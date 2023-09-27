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
 * SECTION:element-gst_nmeaparse
 *
 * Nmeaparse parses telemetry data from nmea strings
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=nmeaparse:5,nmeaconvert:5 gst-launch-1.0 filesrc location=../in.nmea ! nmeaparse ! fakesink
 * </refsect2>
 */

#include "common/sonarshared.h"
#include "nmeaparse/nmeaparse.h"

#include <stdio.h>
#include <ctype.h>

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(nmeaparse_debug);
#define GST_CAT_DEFAULT nmeaparse_debug

#define gst_nmeaparse_parent_class parent_class
G_DEFINE_TYPE(GstNmeaparse, gst_nmeaparse, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_nmeaparse_sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_nmeaparse_telemetry_src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("application/nmea"));

static GstFlowReturn gst_nmeaparse_handle_frame(GstBaseParse* baseparse, GstBaseParseFrame* frame, gint* skipsize)
{
    GstNmeaparse* nmeaparse = GST_NMEAPARSE(baseparse);

    GstMapInfo mapinfo;
    if (!gst_buffer_map(frame->buffer, &mapinfo, GST_MAP_READ))
        return GST_FLOW_ERROR;

#define exit(value)                                                                                                                                                                                    \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        gst_buffer_unmap(frame->buffer, &mapinfo);                                                                                                                                                     \
        return (value);                                                                                                                                                                                \
    } while (0)

    GstByteReader reader;
    gst_byte_reader_init(&reader, mapinfo.data, mapinfo.size);

    // find nmea message start
    *skipsize = gst_byte_reader_masked_scan_uint32(&reader, 0xff000000, '$' << 24, 0, mapinfo.size);

    if (*skipsize == -1)
    {
        *skipsize = mapinfo.size;

        exit(GST_FLOW_OK);
    }
    else if (*skipsize != 0)
        exit(GST_FLOW_OK);    // make sure we start on the beginning of the nmea message before proceeding

    // verify nmea message
    g_assert(mapinfo.size > 6);    // we set initial min frame size larger than this in gst_nmeaparse_start
    if (mapinfo.data[6] != ',')    // check for comma
    {
        *skipsize = 1;
        exit(GST_FLOW_OK);
    }
    for (const char* c = mapinfo.data + 1; c != (const char*)mapinfo.data + 6; ++c)    // check for the 5 letters between dollar sign and comma
    {
        if ((*c < 'A') || (*c > 'Z'))
        {
            *skipsize = 1;
            exit(GST_FLOW_OK);
        }
    }

    // find nmea message end
    guint32 nmea_size = gst_byte_reader_masked_scan_uint32(&reader, 0xffff0000, '\r' << 24 | '\n' << 16, 0, mapinfo.size);

    if (nmea_size == -1)
    {
        // when we can't find the end, we increase the minimum frame size
        gst_base_parse_set_min_frame_size(baseparse, mapinfo.size + 1);
        exit(GST_FLOW_OK);
    }

    GST_LOG_OBJECT(nmeaparse, "nmea entry of size %d: %.*s", nmea_size, nmea_size, mapinfo.data);

    guint64 timestamp        = 0;
    gboolean found_timestamp = FALSE;

    // try to find unix epoch
    if (nmeaparse->timestamp_offset && (nmeaparse->timestamp_offset < nmea_size))
    {
        timestamp = strtol(mapinfo.data + nmeaparse->timestamp_offset, NULL, 10);

        if ((timestamp < (long)1e13)        // 20.11.2286
            && (timestamp > (long)1e12))    // 09.09.2001
        {
            found_timestamp = TRUE;
        }
    }

    if (!found_timestamp)
    {
        for (char* c = (char*)mapinfo.data; c != (char*)mapinfo.data + nmea_size;)
        {
            if (isdigit(*c))
            {
                // check for sane date
                int timestamp_offset = c - (char*)mapinfo.data;
                long val             = strtol(c, &c, 10);
                if ((val < (long)1e13)        // 20.11.2286
                    && (val > (long)1e12))    // 09.09.2001
                {
                    found_timestamp             = TRUE;
                    timestamp                   = val;
                    nmeaparse->timestamp_offset = timestamp_offset;
                    break;
                }
            }
            else
                ++c;
        }
    }

    if (found_timestamp)
    {
        // set timestamp
        timestamp *= (guint64)1e6;    // ms to ns
        if (nmeaparse->initial_time == 0)
            nmeaparse->initial_time = gst_sonarshared_set_initial_time(timestamp);

        if (timestamp < nmeaparse->initial_time)
        {
            GST_WARNING_OBJECT(nmeaparse, "timestamp would be negative: %llu < %llu, reset to zero", timestamp, nmeaparse->initial_time);
            GST_BUFFER_PTS(frame->buffer) = GST_BUFFER_DTS(frame->buffer) = 0;
        }
        else
            GST_BUFFER_PTS(frame->buffer) = GST_BUFFER_DTS(frame->buffer) = timestamp - nmeaparse->initial_time;
    }
    else
    {
        GST_WARNING_OBJECT(nmeaparse, "could not find a suitable unix epoch timestamp");
    }


    exit(gst_base_parse_finish_frame(baseparse, frame, nmea_size + 2));

#undef exit
}

static gboolean gst_nmeaparse_start(GstBaseParse* baseparse)
{
    GstNmeaparse* nmeaparse = GST_NMEAPARSE(baseparse);

    GST_DEBUG_OBJECT(nmeaparse, "start");

    gst_base_parse_set_min_frame_size(baseparse, strlen("$XXXXX,X"));

    // set constant src caps
    GstCaps* srccaps = gst_caps_new_simple("application/nmea", NULL);
    GST_DEBUG_OBJECT(nmeaparse, "setting downstream caps on %s:%s to %" GST_PTR_FORMAT, GST_DEBUG_PAD_NAME(GST_BASE_PARSE_SRC_PAD(nmeaparse)), srccaps);

    if (!gst_pad_set_caps(GST_BASE_PARSE_SRC_PAD(baseparse), srccaps))
    {
        GST_ERROR_OBJECT(nmeaparse, "couldn't set src caps");
        gst_caps_unref(srccaps);
        return FALSE;
    }
    else
    {
        gst_caps_unref(srccaps);
        return TRUE;
    }
}

static void gst_nmeaparse_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstNmeaparse* nmeaparse = GST_NMEAPARSE(object);

    GST_OBJECT_LOCK(nmeaparse);
    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(nmeaparse);
}

static void gst_nmeaparse_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstNmeaparse* nmeaparse = GST_NMEAPARSE(object);

    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_nmeaparse_finalize(GObject* object)
{
    GstNmeaparse* nmeaparse = GST_NMEAPARSE(object);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_nmeaparse_class_init(GstNmeaparseClass* klass)
{
    GObjectClass* gobject_class        = (GObjectClass*)klass;
    GstElementClass* gstelement_class  = (GstElementClass*)klass;
    GstBaseParseClass* baseparse_class = (GstBaseParseClass*)klass;

    gobject_class->finalize     = gst_nmeaparse_finalize;
    gobject_class->set_property = gst_nmeaparse_set_property;
    gobject_class->get_property = gst_nmeaparse_get_property;

    GST_DEBUG_CATEGORY_INIT(nmeaparse_debug, "nmeaparse", 0, "nmeaparse");


    gst_element_class_set_static_metadata(gstelement_class, "Nmeaparse", "Parse", "Nmeaparse parses telemetry data from NMEA strings", "Eelume AS <opensource@eelume.com>");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_nmeaparse_telemetry_src_template);
    gst_element_class_add_static_pad_template(gstelement_class, &gst_nmeaparse_sink_template);

    baseparse_class->handle_frame = GST_DEBUG_FUNCPTR(gst_nmeaparse_handle_frame);
    baseparse_class->start        = GST_DEBUG_FUNCPTR(gst_nmeaparse_start);
}

static void gst_nmeaparse_init(GstNmeaparse* nmeaparse)
{
    nmeaparse->initial_time     = 0;
    nmeaparse->timestamp_offset = 0;
}
