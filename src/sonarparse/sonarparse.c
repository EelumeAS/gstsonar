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
 * SECTION:element-gst_sonarparse
 *
 * Sonarparse parses Norbit WBMS sonar data
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=sonarparse:5,sonarconvert:5 gst-launch-1.0 filesrc location=../in.sbd ! sonarparse ! fakesink
 * </refsect2>
 */

#include "sonarparse.h"
#include "common/sonarshared.h"

#include <stdio.h>

#include <gst/base/gstbytereader.h>

#define NORBIT_SONAR_PREFIX 0xefbeadde    // deadbeef backwards

GST_DEBUG_CATEGORY_STATIC(sonarparse_debug);
#define GST_CAT_DEFAULT sonarparse_debug

#define gst_sonarparse_parent_class parent_class
G_DEFINE_TYPE(GstSonarparse, gst_sonarparse, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_sonarparse_src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("sonar/multibeam, "
                    "n_beams = (int) [ 0, MAX ],"
                    "resolution = (int) [ 0, MAX ], "
                    "framerate = (fraction) [ 0/1, MAX ], "
                    "parsed = (boolean) true ;"

                    "sonar/bathymetry, "
                    "n_beams = (int) [ 0, MAX ],"
                    "resolution = (int) 1, "
                    "framerate = (fraction) [ 0/1, MAX ], "
                    "parsed = (boolean) true ;"));

static GstStaticPadTemplate gst_sonarparse_sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("sonar/multibeam; sonar/bathymetry"));

static GstFlowReturn gst_sonarparse_handle_frame(GstBaseParse* baseparse, GstBaseParseFrame* frame, gint* skipsize)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(baseparse);

    GstMapInfo mapinfo;
    if (!gst_buffer_map(frame->buffer, &mapinfo, GST_MAP_READ))
    {
        return GST_FLOW_ERROR;
    }

#define exit                                                                                                                                                                                           \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        gst_buffer_unmap(frame->buffer, &mapinfo);                                                                                                                                                     \
        return GST_FLOW_OK;                                                                                                                                                                            \
    } while (0)

    if (mapinfo.size < 4)
        exit;

    GstByteReader reader;
    gst_byte_reader_init(&reader, mapinfo.data, mapinfo.size);

    *skipsize = gst_byte_reader_masked_scan_uint32(&reader, 0xffffffff, NORBIT_SONAR_PREFIX, 0, mapinfo.size);
    if (*skipsize == -1)
    {
        *skipsize = mapinfo.size - 3; /* Last 3 bytes + 1 more may match header. */

        exit;
    }
    else if (*skipsize != 0)
        exit;

    GST_TRACE_OBJECT(sonarparse, "found preamble at %d", *skipsize);

    // skip over preamble
    gst_byte_reader_skip(&reader, *skipsize);

    const guint8* header_data          = NULL;
    const wbms_packet_header_t* header = NULL;
    if (!gst_byte_reader_get_data(&reader, sizeof(*header), &header_data))
        exit;
    header = (const wbms_packet_header_t*)header_data;

    if (header->size < sizeof(wbms_packet_header_t))
    {
        GST_ERROR_OBJECT(sonarparse, "size specified in header is too small: %d", header->size);
        gst_buffer_unmap(frame->buffer, &mapinfo);
        return GST_FLOW_ERROR;
    }

    if (mapinfo.size < header->size)
        exit;

    gst_base_parse_set_min_frame_size(baseparse, header->size);

    // parse sub_header
    const guint8* sub_header_data = NULL;
    double sub_header_time;
    guint32 old_n_beams    = sonarparse->n_beams;
    guint32 old_resolution = sonarparse->resolution;
    guint32 old_framerate  = sonarparse->framerate;

    switch (header->type)
    {
        case WBMS_BATH:
        {
            GST_DEBUG_OBJECT(sonarparse, "sonar type is bathymetry");
            const wbms_bath_data_header_t* sub_header;
            if (!gst_byte_reader_get_data(&reader, sizeof(*sub_header), &sub_header_data))
                exit;
            sub_header = (wbms_bath_data_header_t*)sub_header_data;

            sub_header_time        = sub_header->time;
            sonarparse->caps_name  = "sonar/bathymetry";
            sonarparse->n_beams    = sub_header->N;
            sonarparse->resolution = 1;
            sonarparse->framerate  = sub_header->ping_rate;

            sonarparse->next_meta = (GstSonarMeta){
                .format =
                    (GstSonarFormat){
                        .measurement_type   = GST_SONAR_MEASUREMENT_TYPE_INT32,
                        .measurement_stride = sizeof(wbms_detectionpoint_t),
                        .stride             = sizeof(wbms_detectionpoint_t),
                        .measurement_offset = sizeof(wbms_packet_header_t) + sizeof(wbms_bath_data_header_t),
                        .angle_offset       = sizeof(wbms_packet_header_t) + sizeof(wbms_fls_data_header_t) + sizeof(((wbms_detectionpoint_t*)NULL)->sample_number),
                        .angle_type         = GST_SONAR_MEASUREMENT_TYPE_FLOAT32,
                        .angle_stride       = sizeof(wbms_detectionpoint_t),
                    },
                .params =
                    (GstSonarParams){
                        .sound_speed = sub_header->snd_velocity,
                        .sample_rate = sub_header->sample_rate,
                        .t0          = 0,    // t0 doesn't apply
                        .gain        = sub_header->gain,
                    },
            };
            break;
        }
        case WBMS_FLS:    // fls
        {
            GST_DEBUG_OBJECT(sonarparse, "sonar type is fls");
            const wbms_fls_data_header_t* sub_header;
            if (!gst_byte_reader_get_data(&reader, sizeof(*sub_header), &sub_header_data))
                exit;
            sub_header = (wbms_fls_data_header_t*)sub_header_data;

            g_assert(sub_header->dtype == 3);    // int16
            sub_header_time        = sub_header->time;
            sonarparse->caps_name  = "sonar/multibeam";
            sonarparse->n_beams    = sub_header->N;
            sonarparse->resolution = sub_header->M;
            sonarparse->framerate  = sub_header->ping_rate;

            GstSonarMeasurementType intensity_type = wbms_get_intensity_type(sub_header->dtype);
            if (intensity_type == GST_SONAR_MEASUREMENT_TYPE_INVALID)
            {
                GST_ERROR_OBJECT(sonarparse, "dtype invalid / not implemented: %d\n", sub_header->dtype);
                return GST_FLOW_ERROR;
            }
            guint intensity_stride = gst_sonar_measurement_type_get_size(intensity_type);

            sonarparse->next_meta = (GstSonarMeta){
                .format =
                    (GstSonarFormat){
                        .measurement_type   = intensity_type,
                        .measurement_stride = intensity_stride,
                        .stride             = sub_header->N * intensity_stride,
                        .measurement_offset = sizeof(wbms_packet_header_t) + sizeof(wbms_fls_data_header_t),
                        .angle_offset       = sizeof(wbms_packet_header_t) + sizeof(wbms_fls_data_header_t) + sub_header->N * sub_header->M * intensity_stride,
                        .angle_type         = GST_SONAR_MEASUREMENT_TYPE_FLOAT32,
                        .angle_stride       = 4,
                    },
                .params =
                    (GstSonarParams){
                        .sound_speed = sub_header->snd_velocity,
                        .sample_rate = sub_header->sample_rate,
                        .t0          = sub_header->t0,
                        .gain        = sub_header->gain,
                    },
            };
            break;
        }
        default:
            GST_ERROR_OBJECT(sonarparse, "sonar type not recognized: %d", header->type);
            gst_buffer_unmap(frame->buffer, &mapinfo);
            return GST_FLOW_ERROR;
    }

    // set caps
    if ((sonarparse->n_beams != old_n_beams) || (sonarparse->resolution != old_resolution) || (sonarparse->framerate != old_framerate))
    {
        GstCaps* caps = gst_caps_new_simple(sonarparse->caps_name, "n_beams", G_TYPE_INT, sonarparse->n_beams, "resolution", G_TYPE_INT, sonarparse->resolution, "framerate", GST_TYPE_FRACTION,
            sonarparse->framerate, 1, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);

        GST_DEBUG_OBJECT(sonarparse, "setting downstream caps on %s:%s to %" GST_PTR_FORMAT, GST_DEBUG_PAD_NAME(GST_BASE_PARSE_SRC_PAD(sonarparse)), caps);

        if (!gst_pad_set_caps(GST_BASE_PARSE_SRC_PAD(sonarparse), caps))
        {
            GST_ERROR_OBJECT(sonarparse, "couldn't set src caps");
            gst_caps_unref(caps);
            gst_buffer_unmap(frame->buffer, &mapinfo);
            return GST_FLOW_ERROR;
        }

        gst_caps_unref(caps);
    }

    guint64 timestamp = (guint64)(sub_header_time * 1e9);
    if (sonarparse->initial_time == 0)
        sonarparse->initial_time = gst_sonarshared_set_initial_time(timestamp);

    // set timestamp
    if (timestamp < sonarparse->initial_time)
    {
        GST_WARNING_OBJECT(sonarparse, "timestamp would be negative: %llu < %llu, reset to zero", timestamp, gst_sonar_shared_data.initial_time);
        GST_BUFFER_PTS(frame->buffer) = GST_BUFFER_DTS(frame->buffer) = 0;
    }
    else
        GST_BUFFER_PTS(frame->buffer) = GST_BUFFER_DTS(frame->buffer) = timestamp - sonarparse->initial_time;

    GST_BUFFER_DURATION(frame->buffer) = (guint64)(1e9 / sonarparse->framerate);

    // debug framerate
    {
        GST_LOG_OBJECT(sonarparse, "time: %f %llu", sub_header_time, GST_BUFFER_PTS(frame->buffer));
        static double prev_pts = 0;
        double pts             = sub_header_time;
        static double fps      = 0;
        if (prev_pts != 0)
        {
            double new_fps = 1. / (pts - prev_pts);
            if (fps == 0)
                fps = new_fps;
            else
                fps = 0.3 * fps + .7 * new_fps;
            GST_LOG_OBJECT(sonarparse, "fps: %f, %f", new_fps, fps);
        }
        prev_pts = pts;
    }

#undef exit

    gst_buffer_unmap(frame->buffer, &mapinfo);

    return gst_base_parse_finish_frame(baseparse, frame, header->size);
}

static GstFlowReturn gst_sonarparse_pre_push_frame(GstBaseParse* baseparse, GstBaseParseFrame* frame)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(baseparse);

    GstSonarMeta* meta = GST_SONAR_META_ADD(frame->buffer);
    meta->format       = sonarparse->next_meta.format;
    meta->params       = sonarparse->next_meta.params;

    return GST_FLOW_OK;
}

static gboolean gst_sonarparse_start(GstBaseParse* baseparse)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(baseparse);

    GST_DEBUG_OBJECT(sonarparse, "start");

    gst_base_parse_set_min_frame_size(baseparse, sizeof(wbms_packet_header_t));

    return TRUE;
}

static void gst_sonarparse_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(object);

    GST_OBJECT_LOCK(sonarparse);
    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(sonarparse);
}

static void gst_sonarparse_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(object);

    switch (prop_id)
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_sonarparse_finalize(GObject* object)
{
    GstSonarparse* sonarparse = GST_SONARPARSE(object);

    gst_sonarshared_finalize();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_sonarparse_class_init(GstSonarparseClass* klass)
{
    GObjectClass* gobject_class        = (GObjectClass*)klass;
    GstElementClass* gstelement_class  = (GstElementClass*)klass;
    GstBaseParseClass* baseparse_class = (GstBaseParseClass*)klass;

    gobject_class->finalize     = gst_sonarparse_finalize;
    gobject_class->set_property = gst_sonarparse_set_property;
    gobject_class->get_property = gst_sonarparse_get_property;

    GST_DEBUG_CATEGORY_INIT(sonarparse_debug, "sonarparse", 0, "sonarparse");


    gst_element_class_set_static_metadata(gstelement_class, "Sonarparse", "Parse", "Sonarparse parses Norbit WBMS sonar data", "Eelume AS <opensource@eelume.com>");

    gst_element_class_add_static_pad_template(gstelement_class, &gst_sonarparse_sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &gst_sonarparse_src_template);

    baseparse_class->handle_frame   = GST_DEBUG_FUNCPTR(gst_sonarparse_handle_frame);
    baseparse_class->pre_push_frame = GST_DEBUG_FUNCPTR(gst_sonarparse_pre_push_frame);
    baseparse_class->start          = GST_DEBUG_FUNCPTR(gst_sonarparse_start);
}

static void gst_sonarparse_init(GstSonarparse* sonarparse)
{
    sonarparse->n_beams    = 0;
    sonarparse->resolution = 0;
    sonarparse->framerate  = 0;
    sonarparse->caps_name  = NULL;

    sonarparse->next_meta = (GstSonarMeta){
        .format = (GstSonarFormat){0},
        .params = (GstSonarParams){0},
    };

    sonarparse->initial_time = 0;

    gst_sonarshared_init();
}
