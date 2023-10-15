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
    guint angle_stride;                          // stride between angle measurements
    guint upper_gate_offset;                     // offset of first upper gate in buffer
    guint lower_gate_offset;                     // offset of first lower gate in buffer
    guint intensity_offset;                      // offset of first intensity in buffer
    guint flags_offset;                          // offset of first status flag in buffer
    guint quality_flags_offset;                  // offset of first quality flag in buffer
    guint quality_val_offset;                    // offset of first quality vlaue in buffer
} GstSonarFormat;

typedef struct
{
    guint64 time;          // TX time, microseconds since Unix epoch
    guint64 network_time;  // Network time for packet, microseconds since Unix epoch 
    guint32 ping_number;   // Ping number
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


// telemetry
#define GST_SONAR_TELEMETRY_PRESENCE_ROLL      (1 << 0)
#define GST_SONAR_TELEMETRY_PRESENCE_PITCH     (1 << 1)
#define GST_SONAR_TELEMETRY_PRESENCE_YAW       (1 << 2)
#define GST_SONAR_TELEMETRY_PRESENCE_LATITUDE  (1 << 3)
#define GST_SONAR_TELEMETRY_PRESENCE_LONGITUDE (1 << 4)
#define GST_SONAR_TELEMETRY_PRESENCE_DEPTH     (1 << 5)
#define GST_SONAR_TELEMETRY_PRESENCE_ALTITUDE  (1 << 6)
#define GST_SONAR_TELEMETRY_PRESENCE_N_FIELDS  (7)
#define GST_SONAR_TELEMETRY_PRESENCE_FULL      ((1 << GST_SONAR_TELEMETRY_PRESENCE_N_FIELDS) - 1)

typedef float GstSonarTelemetryField;

// contains telemetry data. which data is specified in the presence field
typedef struct
{
    GstSonarTelemetryField roll, pitch, yaw;       // in radians
    GstSonarTelemetryField latitude, longitude;    // in degrees
    GstSonarTelemetryField depth;                  // in meters
    GstSonarTelemetryField altitude;               // in meters
    guint8 presence;                               // composed of bitfields GST_SONAR_TELEMETRY_PRESENCE_ROLL etc.

} GstSonarTelemetry;

// telemetry, along with associated timestamps
typedef struct
{
    GstSonarTelemetry tel;

    guint64 attitude_time;
    guint64 position_time;
    guint64 depth_time;
    guint64 altitude_time;

} GstSonarTelemetryTimed;

typedef struct
{
    GstMeta meta;
    GstSonarTelemetry tel;    // interpolated telemetry

} GstTelemetryMeta;

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
    float gst_sonar_format_get_intensity(const GstSonarFormat* format, const char* buffer, int beam_index);
    guint8 gst_sonar_format_get_quality_flags(const GstSonarFormat* format, const char* buffer, int beam_index);
    guint8 gst_sonar_format_get_quality_val(const GstSonarFormat* format, const char* buffer, int beam_index);
    

    // telemetry meta
    GType gst_telemetry_meta_api_get_type(void);
    const GstMetaInfo* gst_telemetry_meta_get_info(void);
    #define GST_TELEMETRY_META_GET(buf) ((GstTelemetryMeta*)gst_buffer_get_meta(buf, gst_telemetry_meta_api_get_type()))
    #define GST_TELEMETRY_META_ADD(buf) ((GstTelemetryMeta*)gst_buffer_add_meta(buf, gst_telemetry_meta_get_info(), NULL))



#ifdef __cplusplus
}    // extern "C"
#endif

#endif /* __GST_SONARMETA_H__ */
