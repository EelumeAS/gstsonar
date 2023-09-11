// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#include <assert.h>
#include <stdint.h>

#include "sonarmeta.h"

int gst_sonar_measurement_type_get_size(GstSonarMeasurementType measurement_type)
{
    switch (measurement_type)
    {
        case GST_SONAR_MEASUREMENT_TYPE_INT8:
            return sizeof(int8_t);
        case GST_SONAR_MEASUREMENT_TYPE_INT16:
            return sizeof(int16_t);
        case GST_SONAR_MEASUREMENT_TYPE_INT32:
            return sizeof(int32_t);
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT32:
            return sizeof(float);
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT64:
            return sizeof(double);
        default:
            assert(0);
    }
}

float gst_sonar_measurement_type_get_value(GstSonarMeasurementType measurement_type, const char* address)
{
    switch (measurement_type)
    {
        case GST_SONAR_MEASUREMENT_TYPE_INT8:
            return *(const int8_t*)address;
        case GST_SONAR_MEASUREMENT_TYPE_INT16:
            return *(const int16_t*)address;
        case GST_SONAR_MEASUREMENT_TYPE_INT32:
            return *(const int32_t*)address;
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT32:
            return *(const float*)address;
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT64:
            return *(const double*)address;
        default:
            assert(0);
    }
}

void gst_sonar_measurement_type_set_value(GstSonarMeasurementType measurement_type, char* address, float value)
{
    switch (measurement_type)
    {
        case GST_SONAR_MEASUREMENT_TYPE_INT8:
            *(uint8_t*)address = value;
            break;
        case GST_SONAR_MEASUREMENT_TYPE_INT16:
            *(uint16_t*)address = value;
            break;
        case GST_SONAR_MEASUREMENT_TYPE_INT32:
            *(uint32_t*)address = value;
            break;
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT32:
            *(float*)address = value;
            break;
        case GST_SONAR_MEASUREMENT_TYPE_FLOAT64:
            *(double*)address = value;
            break;
        default:
            assert(0);
    }
}


float gst_sonar_format_get_measurement(const GstSonarFormat* format, const char* buffer, int beam_index, int range_index)
{
    return gst_sonar_measurement_type_get_value(format->measurement_type, buffer + format->measurement_offset + range_index * format->stride + beam_index * format->measurement_stride);
}

void gst_sonar_format_set_measurement(const GstSonarFormat* format, char* buffer, int beam_index, int range_index, float value)
{
    return gst_sonar_measurement_type_set_value(format->measurement_type, buffer + format->measurement_offset + range_index * format->stride + beam_index * format->measurement_stride, value);
}

float gst_sonar_format_get_angle(const GstSonarFormat* format, const char* buffer, int beam_index)
{
    return gst_sonar_measurement_type_get_value(format->angle_type, buffer + format->angle_offset + beam_index * format->angle_stride);
}

void gst_sonar_format_set_angle(const GstSonarFormat* format, char* buffer, int beam_index, float value)
{
    return gst_sonar_measurement_type_set_value(format->angle_type, buffer + format->angle_offset + beam_index * format->angle_stride, value);
}

GType gst_sonar_meta_api_get_type(void)
{
    static GType type;
    static const gchar* tags[] = {GST_META_TAG_MEMORY_STR, NULL};

    if (g_once_init_enter(&type))
    {
        GType _type = gst_meta_api_type_register("GstSonarMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

static gboolean gst_sonar_meta_init(GstMeta* meta, G_GNUC_UNUSED gpointer params, G_GNUC_UNUSED GstBuffer* buffer)
{
    GstSonarMeta* sonarmeta = (GstSonarMeta*)meta;

    sonarmeta->format = (GstSonarFormat){0};
    sonarmeta->params = (GstSonarParams){0};

    return TRUE;
}

const GstMetaInfo* gst_sonar_meta_get_info(void)
{
    static const GstMetaInfo* meta_info = NULL;

    if (g_once_init_enter(&meta_info))
    {
        const GstMetaInfo* meta =
            gst_meta_register(gst_sonar_meta_api_get_type(), "GstSonarMeta", sizeof(GstSonarMeta), (GstMetaInitFunction)gst_sonar_meta_init, (GstMetaFreeFunction)NULL, (GstMetaTransformFunction)NULL);
        g_once_init_leave(&meta_info, meta);
    }
    return meta_info;
}
