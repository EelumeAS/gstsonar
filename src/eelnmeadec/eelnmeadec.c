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
 * SECTION:element-gst_eelnmeadec
 *
 * EelNmeaDec parses telemetry data from custom nmea messages
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=sonarparse:5,eelnmeadec:5 gst-launch-1.0 filesrc location=../in.sbd ! sonarparse ! eelnmeadec ! fakesink
 * </refsect2>
 */

#include "common/sonarshared.h"
#include "eelnmeadec/eelnmeadec.h"
#include "sonarmux/sonarmux.h"

#include <math.h>
#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC(eelnmeadec_debug);
#define GST_CAT_DEFAULT eelnmeadec_debug

#define gst_eelnmeadec_parent_class parent_class
G_DEFINE_TYPE(GstEelNmeaDec, gst_eelnmeadec, GST_TYPE_BASE_TRANSFORM);

static GstStaticPadTemplate gst_eelnmeadec_src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/telemetry"));

static GstStaticPadTemplate gst_eelnmeadec_sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/nmea"));

static GstFlowReturn gst_eelnmeadec_transform(GstBaseTransform* basetransform, GstBuffer* in, GstBuffer* out)
{
    GstEelNmeaDec* eelnmeadec = GST_EEL_NMEA_DEC(basetransform);

    GstMapInfo mapinfo;
    if (!gst_buffer_map(in, &mapinfo, GST_MAP_READ))
        return GST_FLOW_ERROR;

    // allocate and parse telemetry
    GstSonarTelemetry* telemetry = g_malloc(sizeof(*telemetry));
    guint64 timestamp            = 0;
    gdouble timeUTC;
    guint32 len;

    gdouble heading;

    gdouble longitude;
    gdouble latitude;
    gchar north;
    gchar east;

    gdouble roll;
    gdouble pitch;

    gdouble depth;
    gdouble altitude;

    if ((sscanf(mapinfo.data, "$EIHEA,%u,%lf,%lu,%lf*", &len, &timeUTC, &timestamp, &heading) == 4) && (heading != -1))
        *telemetry = (GstSonarTelemetry){
            .yaw      = heading * M_PI / 180.,
            .presence = GST_SONAR_TELEMETRY_PRESENCE_YAW,
        };
    else if ((sscanf(mapinfo.data, "$EIPOS,%u,%lf,%lu,%lf,%c,%lf,%c*", &len, &timeUTC, &timestamp, &latitude, &north, &longitude, &east) == 7) && (latitude != -1) && (longitude != -1))
        *telemetry = (GstSonarTelemetry){
            .latitude  = latitude * (north == 'N' ? 1 : -1),
            .longitude = longitude * (east == 'E' ? 1 : -1),
            .presence  = GST_SONAR_TELEMETRY_PRESENCE_LATITUDE | GST_SONAR_TELEMETRY_PRESENCE_LONGITUDE,
        };
    else if ((sscanf(mapinfo.data, "$EIORI,%u,%lf,%lu,%lf,%lf*", &len, &timeUTC, &timestamp, &roll, &pitch) == 5) && (roll != -1) && (pitch != -1))
        *telemetry = (GstSonarTelemetry){
            .roll     = roll * M_PI / 180.,
            .pitch    = pitch * M_PI / 180.,
            .presence = GST_SONAR_TELEMETRY_PRESENCE_ROLL | GST_SONAR_TELEMETRY_PRESENCE_PITCH,
        };
    else if ((sscanf(mapinfo.data, "$EIDEP,%u,%lf,%lu,%lf,m,%lf,m*", &len, &timeUTC, &timestamp, &depth, &altitude) == 5) && (depth != -1) && (altitude != -1))
        *telemetry = (GstSonarTelemetry){
            .depth    = depth,
            .altitude = altitude,
            .presence = GST_SONAR_TELEMETRY_PRESENCE_ALTITUDE | GST_SONAR_TELEMETRY_PRESENCE_DEPTH,
        };
    else
    {
        GST_WARNING_OBJECT(eelnmeadec, "invalid nmea: %.*s\n", (int)(mapinfo.size - 2), mapinfo.data);

        *telemetry = (GstSonarTelemetry){0};
    }

    gst_buffer_unmap(in, &mapinfo);

    // set out buffer
    GstMemory* mem = gst_memory_new_wrapped(0,    // flags (GstMemoryFlags)
        telemetry,                                // data
        sizeof(*telemetry),                       // maxsize
        0,                                        // offset
        sizeof(*telemetry),                       // size
        NULL,                                     // user_data
        NULL);                                    // notify (GDestroyNotify)
    gst_buffer_replace_all_memory(out, mem);

    // set timestamp
    timestamp *= (guint64)1e6;    // ms to ns
    if (eelnmeadec->initial_time == 0)
        eelnmeadec->initial_time = gst_sonarshared_set_initial_time(timestamp);

    if (timestamp < eelnmeadec->initial_time)
    {
        GST_WARNING_OBJECT(eelnmeadec, "timestamp would be negative: %lu < %lu, reset to zero", timestamp, eelnmeadec->initial_time);
        GST_BUFFER_PTS(out) = GST_BUFFER_DTS(out) = 0;
    }
    else
        GST_BUFFER_PTS(out) = GST_BUFFER_DTS(out) = timestamp - eelnmeadec->initial_time;

    GST_TRACE_OBJECT(eelnmeadec, "created telemetry buffer %p, timestamp: %lu, pts: %lu", out, timestamp, GST_BUFFER_PTS(out));

    return GST_FLOW_OK;
}

static GstCaps* gst_eelnmeadec_transform_caps(GstBaseTransform* basetransform, GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    GstEelNmeaDec* eelnmeadec = GST_EEL_NMEA_DEC(basetransform);

    if (direction == GST_PAD_SRC)
    {
        GstStructure* s = gst_caps_get_structure(caps, 0);
        GST_DEBUG_OBJECT(eelnmeadec, "src structure: %s", gst_structure_to_string(s));

        GstCaps* sink_caps = gst_caps_new_simple("application/nmea", NULL);

        return sink_caps;
    }
    else if (direction == GST_PAD_SINK)
    {
        return gst_caps_new_simple("application/telemetry", NULL);
    }
    else
    {
        g_assert_not_reached();
    }
}

static void gst_eelnmeadec_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstEelNmeaDec* eelnmeadec = GST_EEL_NMEA_DEC(object);

    GST_OBJECT_LOCK(eelnmeadec);
    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(eelnmeadec);
}

static void gst_eelnmeadec_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstEelNmeaDec* eelnmeadec = GST_EEL_NMEA_DEC(object);

    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_eelnmeadec_finalize(GObject* object)
{
    GstEelNmeaDec* eelnmeadec = GST_EEL_NMEA_DEC(object);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_eelnmeadec_class_init(GstEelNmeaDecClass* klass)
{
    GObjectClass* gobject_class                = (GObjectClass*)klass;
    GstElementClass* gstelement_class          = (GstElementClass*)klass;
    GstBaseTransformClass* basetransform_class = (GstBaseTransformClass*)klass;

    gobject_class->finalize     = gst_eelnmeadec_finalize;
    gobject_class->set_property = gst_eelnmeadec_set_property;
    gobject_class->get_property = gst_eelnmeadec_get_property;

    GST_DEBUG_CATEGORY_INIT(eelnmeadec_debug, "eelnmeadec", 0, "eelnmeadec");


    gst_element_class_set_static_metadata(gstelement_class, "EelNmeaDec", "Transform", "Decodes  telemetry data from Eelume NMEA messages", "Eelume AS <opensource@eelume.com>");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_eelnmeadec_sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &gst_eelnmeadec_src_template);

    basetransform_class->transform      = GST_DEBUG_FUNCPTR(gst_eelnmeadec_transform);
    basetransform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_eelnmeadec_transform_caps);
}

static void gst_eelnmeadec_init(GstEelNmeaDec* eelnmeadec)
{
    eelnmeadec->initial_time = 0;
}
