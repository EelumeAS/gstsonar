// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_SONARMETA_H__
#define __GST_SONARMETA_H__

#include <gst/gst.h>

G_BEGIN_DECLS

GType gst_sonar_meta_api_get_type(void);
const GstMetaInfo* gst_sonar_meta_get_info(void);
#define GST_SONAR_META_GET(buf) ((GstSonarMeta*)gst_buffer_get_meta(buf, gst_sonar_meta_api_get_type()))
#define GST_SONAR_META_ADD(buf) ((GstSonarMeta*)gst_buffer_add_meta(buf, gst_sonar_meta_get_info(), NULL))

typedef enum
{
    GST_SONAR_TYPE_BATHYMETRY,
    GST_SONAR_TYPE_FLS,
} GstSonarType;

typedef enum
{
    GST_SONAR_MEASUREMENT_TYPE_INVALID,
    GST_SONAR_MEASUREMENT_TYPE_INT8,
    GST_SONAR_MEASUREMENT_TYPE_INT16,
    GST_SONAR_MEASUREMENT_TYPE_INT32,
    GST_SONAR_MEASUREMENT_TYPE_FLOAT32,
    GST_SONAR_MEASUREMENT_TYPE_FLOAT64,
} GstSonarMeasurementType;

typedef struct
{
    GstSonarMeasurementType measurement_type;    // type of each measurement
    guint measurement_stride;                    // bytes between each measurement
    guint stride;                                // bytes between each beam of measurements
    guint measurement_offset;                    // offset of first measurement in buffer
    guint angle_offset;                          // offset of first angle
    GstSonarMeasurementType angle_type;          // type of each angle
    guint angle_stride;                          // bytes between each angle
} GstSonarFormat;

typedef struct
{
    gfloat sound_speed;    // Filtered sanitized sound speed in m/s
    gfloat sample_rate;    // Sample rate in reported range sample index, in Hz
    guint t0;              // Sample index of first sample in each beam
    gfloat gain;           // Intensity value gain

} GstSonarParams;

typedef struct
{
    GstMeta meta;
    GstSonarFormat format;
    GstSonarParams params;

} GstSonarMeta;

G_END_DECLS

#ifdef __cplusplus
extern "C"
{
#endif

    int gst_sonar_measurement_type_get_size(GstSonarMeasurementType measurement_type);

    float gst_sonar_format_get_measurement(const GstSonarFormat* format, const char* buffer, int beam_index, int range_index);
    void gst_sonar_format_set_measurement(const GstSonarFormat* format, char* buffer, int beam_index, int range_index, float value);

    float gst_sonar_format_get_angle(const GstSonarFormat* format, const char* buffer, int beam_index);
    void gst_sonar_format_set_angle(const GstSonarFormat* format, char* buffer, int beam_index, float value);

#ifdef __cplusplus
}    // extern "C"
#endif

#endif /* __GST_SONARMETA_H__ */
