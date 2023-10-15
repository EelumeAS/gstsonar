// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
/**
 * SECTION:element-gst_sonarmux
 *
 * Muxer for interpolating telemetry data over sonar data
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  SBD=../samples/in.sbd && GST_PLUGIN_PATH=. GST_DEBUG=2,sonarmux:6 gst-launch-1.0 filesrc location=$SBD ! sonarparse ! sonarmux name=mux ! sonardetect ! sonarsink filesrc location=$SBD ! nmeaparse !
 mux.
 * </refsect2>
 */

#include "common/linalg.h"
#include "nmeaparse/nmeaparse.h"
#include "sonarmux/sonarmux.h"

#include <stdio.h>

#define SONAR_CAPS                "sonar/multibeam; sonar/bathymetry"
#define TELEMETRY_CAPS            "application/telemetry"
#define SONAR_WITH_TELEMETRY_CAPS "sonar/multibeam, has_telemetry = (boolean) true ; sonar/bathymetry, has_telemetry = (boolean) true"

GST_DEBUG_CATEGORY_STATIC(sonarmux_debug);
#define GST_CAT_DEFAULT sonarmux_debug

#define gst_sonarmux_parent_class parent_class
G_DEFINE_TYPE(GstSonarmux, gst_sonarmux, GST_TYPE_AGGREGATOR);

enum
{
    PROP_0,
};

static GstStaticPadTemplate gst_sonarmux_sonar_sink_template = GST_STATIC_PAD_TEMPLATE("sonar", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS(SONAR_CAPS));

static GstStaticPadTemplate gst_sonarmux_telemetry_sink_template = GST_STATIC_PAD_TEMPLATE("tel", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS(TELEMETRY_CAPS));

static GstStaticPadTemplate gst_sonarmux_src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(SONAR_WITH_TELEMETRY_CAPS));

static void gst_sonarmux_free_buf(gpointer data)
{
    GstBuffer* buf = (GstBuffer*)data;
    gst_buffer_unref(buf);
}


// update timed_tel from tel
static void gst_sonar_telemetry_timed_set(GstSonarTelemetryTimed* timed_tel, const GstSonarTelemetry* tel, guint64 tel_time)
{
    // update timestamp(s)
    if (tel->presence & (GST_SONAR_TELEMETRY_PRESENCE_ROLL | GST_SONAR_TELEMETRY_PRESENCE_PITCH | GST_SONAR_TELEMETRY_PRESENCE_YAW))
        timed_tel->attitude_time = tel_time;

    if (tel->presence & (GST_SONAR_TELEMETRY_PRESENCE_LATITUDE | GST_SONAR_TELEMETRY_PRESENCE_LONGITUDE))
        timed_tel->position_time = tel_time;

    if (tel->presence & (GST_SONAR_TELEMETRY_PRESENCE_DEPTH))
        timed_tel->depth_time = tel_time;

    if (tel->presence & (GST_SONAR_TELEMETRY_PRESENCE_ALTITUDE))
        timed_tel->altitude_time = tel_time;

    // update telemetry
    for (int i = 0; i < GST_SONAR_TELEMETRY_PRESENCE_N_FIELDS; ++i)
        if (tel->presence & (1 << i))
            ((GstSonarTelemetryField*)&timed_tel->tel)[i] = ((const GstSonarTelemetryField*)tel)[i];

    // update presence
    timed_tel->tel.presence |= tel->presence;
}

gboolean gst_sonar_telemetry_has_full_presence(const GstSonarTelemetry* tel)
{
    return (tel->presence ^ GST_SONAR_TELEMETRY_PRESENCE_FULL) == 0;
}

// update the borders of the telemetry interpolation interval
static void gst_sonarmux_update_pretel_posttel(gpointer data, gpointer user_data)
{
    GstBuffer* telbuf     = (GstBuffer*)data;
    GstSonarmux* sonarmux = (GstSonarmux*)user_data;


    GstMapInfo mapinfo;
    if (!gst_buffer_map(telbuf, &mapinfo, GST_MAP_READ))
    {
        GST_WARNING_OBJECT(sonarmux, "couldn't map telemetry buffer at %p", telbuf);
    }
    else if (mapinfo.size != sizeof(GstSonarTelemetry))
    {
        GST_WARNING_OBJECT(sonarmux, "telemetry buffer at %p had wrong size %lu != %lu", telbuf, mapinfo.size, sizeof(GstSonarTelemetry));
        gst_buffer_unmap(telbuf, &mapinfo);
    }
    else
    {
        GstSonarTelemetry* tel = (GstSonarTelemetry*)mapinfo.data;

        GST_TRACE_OBJECT(sonarmux, "%lu:\tgot telemetry buf %p: pitch=%f, roll=%f, yaw=%f, latitude=%f, longitude=%f, depth=%f, altitude=%f, presence=%02x", telbuf->pts, telbuf, tel->pitch,
            tel->roll, tel->yaw, tel->latitude, tel->longitude, tel->depth, tel->altitude, tel->presence);

        if (telbuf->pts > sonarmux->sonarbuf->pts)
        {
            gst_sonar_telemetry_timed_set(&sonarmux->posttel, tel, telbuf->pts);

            gst_buffer_unmap(telbuf, &mapinfo);
        }
        else if (tel->presence & sonarmux->pretel.tel.presence)
        {
            // remove outdated buffers
            gst_buffer_unmap(telbuf, &mapinfo);
            gst_buffer_unref(telbuf);
            g_queue_remove(&sonarmux->telbufs, data);
            GST_TRACE_OBJECT(sonarmux, "Queue length after remove: %u", g_queue_get_length(&sonarmux->telbufs));
        }
        else
        {
            gst_sonar_telemetry_timed_set(&sonarmux->pretel, tel, telbuf->pts);
        }
    }
}

GstSonarTelemetry gst_sonar_telemetry_timed_interpolate(GstSonarTelemetryTimed* first, GstSonarTelemetryTimed* second, guint64 interpolation_time)
{
    const linalg_euler_angles_t first_angles = {
        .roll  = first->tel.roll,
        .pitch = first->tel.pitch,
        .yaw   = first->tel.yaw,
        .time  = first->attitude_time,
    };

    const linalg_euler_angles_t second_angles = {
        .roll  = second->tel.roll,
        .pitch = second->tel.pitch,
        .yaw   = second->tel.yaw,
        .time  = second->attitude_time,
    };

    linalg_euler_angles_t euler_angles;
    linalg_interpolate_euler_angles(&euler_angles, &first_angles, &second_angles, interpolation_time);

    GstSonarTelemetry ret;
    ret.roll  = euler_angles.roll;
    ret.pitch = euler_angles.pitch;
    ret.yaw   = euler_angles.yaw;

    ret.latitude  = linalg_interpolate_scalar(first->tel.latitude, first->position_time, second->tel.latitude, second->position_time, interpolation_time);
    ret.longitude = linalg_interpolate_scalar(first->tel.longitude, first->position_time, second->tel.longitude, second->position_time, interpolation_time);
    ret.depth     = linalg_interpolate_scalar(first->tel.depth, first->position_time, second->tel.depth, second->depth_time, interpolation_time);
    ret.altitude  = linalg_interpolate_scalar(first->tel.altitude, first->position_time, second->tel.altitude, second->altitude_time, interpolation_time);
    ret.presence  = GST_SONAR_TELEMETRY_PRESENCE_FULL;

    return ret;
}

// called when exactly one buffer is queued on both sinks
// (see https://gstreamer.freedesktop.org/documentation/base/gstaggregator.html?gi-language=c)
static GstFlowReturn gst_sonarmux_aggregate(GstAggregator* aggregator, gboolean timeout)
{
    GstSonarmux* sonarmux = GST_SONARMUX(aggregator);

    GST_TRACE_OBJECT(sonarmux, "aggregate");

    if (!sonarmux->sonarbuf)
    {
        sonarmux->sonarbuf = gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sonarmux->sonarsink);
        if (!sonarmux->sonarbuf)
        {
            GST_WARNING_OBJECT(sonarmux, "no more sonar buffers");
            return GST_FLOW_EOS;
        }
    }

    // gst_aggregator_pad_drop_buffer((GstAggregatorPad*)sonarmux->telsink);
    GstBuffer* telbuf = gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sonarmux->telsink);
    if (!telbuf)
    {
        GST_WARNING_OBJECT(sonarmux, "no more telemetry buffers");
        return GST_FLOW_EOS;
    }
    else
    {
        g_queue_push_head(&sonarmux->telbufs, telbuf);
        GST_TRACE_OBJECT(sonarmux, "Queue length after add: %u", g_queue_get_length(&sonarmux->telbufs));

        sonarmux->pretel  = (GstSonarTelemetryTimed){0};
        sonarmux->posttel = (GstSonarTelemetryTimed){0};

        if (telbuf->pts > sonarmux->sonarbuf->pts)
            g_queue_foreach(&sonarmux->telbufs, (GFunc)gst_sonarmux_update_pretel_posttel, sonarmux);

        if (gst_sonar_telemetry_has_full_presence(&sonarmux->posttel.tel))
        {
            if (gst_sonar_telemetry_has_full_presence(&sonarmux->pretel.tel))
            {
                const GstSonarTelemetry* tel = &sonarmux->posttel.tel;
                GST_LOG_OBJECT(sonarmux, "%lu:\tposttel: pitch=%f, roll=%f, yaw=%f, latitude=%f, longitude=%f, depth=%f, altitude=%f, presence: %#02x", telbuf->pts, tel->pitch, tel->roll, tel->yaw,
                    tel->latitude, tel->longitude, tel->depth, tel->altitude, tel->presence);

                tel = &sonarmux->pretel.tel;
                GST_LOG_OBJECT(sonarmux, "%lu:\tpretel: pitch=%f, roll=%f, yaw=%f, latitude=%f, longitude=%f, depth=%f, altitude=%f, presence: %#02x", telbuf->pts, tel->pitch, tel->roll, tel->yaw,
                    tel->latitude, tel->longitude, tel->depth, tel->altitude, tel->presence);

                // prepare and release sonar buffer
                GstBuffer* sonarbuf = sonarmux->sonarbuf;
                sonarmux->sonarbuf  = NULL;

                GstTelemetryMeta* meta = GST_TELEMETRY_META_ADD(sonarbuf);
                meta->tel              = gst_sonar_telemetry_timed_interpolate(&sonarmux->pretel, &sonarmux->posttel, sonarbuf->pts);

                return gst_aggregator_finish_buffer(aggregator, sonarbuf);
            }
            else
            {
                GST_WARNING_OBJECT(sonarmux, "not enough initial telemetry to interpolate");
                gst_buffer_unref(sonarmux->sonarbuf);
                sonarmux->sonarbuf = NULL;
                return GST_AGGREGATOR_FLOW_NEED_DATA;
            }
        }
        else
            return GST_AGGREGATOR_FLOW_NEED_DATA;
    }
}

static GstAggregatorPad* gst_sonarmux_create_new_pad(GstAggregator* aggregator, GstPadTemplate* templ, const gchar* req_name, const GstCaps* caps)
{
    GstSonarmux* sonarmux = GST_SONARMUX(aggregator);

    const gchar* templ_caps = gst_caps_to_string(templ->caps);

    GST_DEBUG_OBJECT(sonarmux, "create_new_pad: req_name: %s, template: %s, direction: %d", req_name, templ_caps, templ->direction);

    GstAggregatorPad* pad = g_object_new(GST_TYPE_SONARMUX_PAD, "name", req_name, "direction", templ->direction, "template", templ, NULL);

    if (strcmp(templ_caps, SONAR_CAPS) == 0)
        sonarmux->sonarsink = (GstPad*)pad;
    else if (strcmp(templ_caps, TELEMETRY_CAPS) == 0)
        sonarmux->telsink = (GstPad*)pad;
    else
        g_assert_not_reached();

    return pad;
}

static gboolean gst_sonarmux_update_src_caps(GstAggregator* aggregator, GstCaps* caps, GstCaps** ret)
{
    GstSonarmux* sonarmux = GST_SONARMUX(aggregator);

    GstCaps* sonarcaps = gst_pad_get_current_caps(sonarmux->sonarsink);
    GST_DEBUG_OBJECT(sonarmux, "sonarsink caps: %s\nother caps: %s", gst_caps_to_string(sonarcaps), gst_caps_to_string(caps));

    *ret = gst_caps_copy(sonarcaps);

    // GstStructure *s = gst_caps_get_structure (*ret, 0);
    // const GValue* has_telemetry = gst_structure_get_value(s, "parsed");
    GValue has_telemetry = G_VALUE_INIT;
    g_value_init(&has_telemetry, G_TYPE_BOOLEAN);
    g_value_set_boolean(&has_telemetry, TRUE);
    gst_caps_set_value(*ret, "has_telemetry", &has_telemetry);

    return TRUE;
}

static void gst_sonarmux_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstSonarmux* sonarmux = GST_SONARMUX(object);

    GST_OBJECT_LOCK(sonarmux);
    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(sonarmux);
}

static void gst_sonarmux_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstSonarmux* sonarmux = GST_SONARMUX(object);

    GST_OBJECT_LOCK(sonarmux);
    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(sonarmux);
}

static void gst_sonarmux_finalize(GObject* object)
{
    GstSonarmux* sonarmux = GST_SONARMUX(object);

    g_queue_foreach(&sonarmux->telbufs, (GFunc)gst_sonarmux_free_buf, NULL);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_sonarmux_class_init(GstSonarmuxClass* klass)
{
    GObjectClass* gobject_class          = (GObjectClass*)klass;
    GstElementClass* gstelement_class    = (GstElementClass*)klass;
    GstAggregatorClass* aggregator_class = (GstAggregatorClass*)klass;

    gobject_class->finalize     = gst_sonarmux_finalize;
    gobject_class->set_property = gst_sonarmux_set_property;
    gobject_class->get_property = gst_sonarmux_get_property;

    aggregator_class->aggregate       = GST_DEBUG_FUNCPTR(gst_sonarmux_aggregate);
    aggregator_class->create_new_pad  = GST_DEBUG_FUNCPTR(gst_sonarmux_create_new_pad);
    aggregator_class->update_src_caps = GST_DEBUG_FUNCPTR(gst_sonarmux_update_src_caps);

    GST_DEBUG_CATEGORY_INIT(sonarmux_debug, "sonarmux", 0, "sonarmux");

    gst_element_class_set_static_metadata(gstelement_class, "Sonarmux", "Sink", "Muxer for interpolating telemetry data over sonar data", "Eelume AS <opensource@eelume.com>");

    gst_element_class_add_static_pad_template_with_gtype(gstelement_class, &gst_sonarmux_sonar_sink_template, GST_TYPE_SONARMUX_PAD);
    gst_element_class_add_static_pad_template_with_gtype(gstelement_class, &gst_sonarmux_telemetry_sink_template, GST_TYPE_SONARMUX_PAD);
    gst_element_class_add_static_pad_template_with_gtype(gstelement_class, &gst_sonarmux_src_template, GST_TYPE_SONARMUX_PAD);
}

static void gst_sonarmux_init(GstSonarmux* sonarmux)
{
    g_queue_init(&sonarmux->telbufs);
}

// custom pad
static void gst_sonarmux_pad_class_init(GstSonarmuxPadClass* klass)
{
    GObjectClass* gobject_class = (GObjectClass*)klass;
}

static void gst_sonarmux_pad_init(GstSonarmuxPad* pad)
{
    // pad->configured_caps = NULL;
}

G_DEFINE_TYPE(GstSonarmuxPad, gst_sonarmux_pad, GST_TYPE_AGGREGATOR_PAD);


