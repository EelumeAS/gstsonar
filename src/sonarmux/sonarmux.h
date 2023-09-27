// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_SONARMUX_H__
#define __GST_SONARMUX_H__

#include <gst/gst.h>
#include <glib/gqueue.h>

#include <gst/base/gstaggregator.h>

G_BEGIN_DECLS

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

// telemetry meta
GType gst_telemetry_meta_api_get_type(void);
const GstMetaInfo* gst_telemetry_meta_get_info(void);
#define GST_TELEMETRY_META_GET(buf) ((GstTelemetryMeta*)gst_buffer_get_meta(buf, gst_telemetry_meta_api_get_type()))
#define GST_TELEMETRY_META_ADD(buf) ((GstTelemetryMeta*)gst_buffer_add_meta(buf, gst_telemetry_meta_get_info(), NULL))

typedef struct
{
    GstMeta meta;
    GstSonarTelemetry tel;    // interpolated telemetry

} GstTelemetryMeta;

// we need to define custom pads for aggregator
GType gst_sonarmux_pad_get_type(void);

#define GST_TYPE_SONARMUX_PAD            (gst_sonarmux_pad_get_type())
#define GST_SONARMUX_PAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SONARMUX_PAD, GstSonarmuxPad))
#define GST_SONARMUX_PAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SONARMUX_PAD, GstSonarmuxPadClass))
#define GST_IS_SONARMUX_PAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SONARMUX_PAD))
#define GST_IS_SONARMUX_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SONARMUX_PAD))
#define GST_SONARMUX_PAD_CAST(obj)       ((GstSonarmuxPad*)(obj))

typedef struct
{
    GstAggregatorPad parent;

    // GstCaps *configured_caps;
} GstSonarmuxPad;

typedef struct
{
    GstAggregatorPadClass parent;
} GstSonarmuxPadClass;

#define GST_TYPE_SONARMUX            (gst_sonarmux_get_type())
#define GST_SONARMUX(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SONARMUX, GstSonarmux))
#define GST_SONARMUX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SONARMUX, GstSonarmuxClass))
#define GST_IS_SONARMUX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SONARMUX))
#define GST_IS_SONARMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SONARMUX))

typedef struct _GstSonarmux GstSonarmux;
typedef struct _GstSonarmuxClass GstSonarmuxClass;

struct _GstSonarmux
{
    GstAggregator aggregator;

    GstPad* sonarsink;
    GstPad* telsink;

    GstBuffer* sonarbuf;    // the sonarbuf currently being interpolated
    GQueue telbufs;         // the telemetry buffers considered for interpolation

    GstSonarTelemetryTimed pretel;    // sonar frames are interpolated between pretel and posttel
    GstSonarTelemetryTimed posttel;
};

struct _GstSonarmuxClass
{
    GstAggregatorClass parent_class;
};

GType gst_sonarmux_get_type(void);

G_END_DECLS

#endif /* __GST_SONARMUX_H__ */
