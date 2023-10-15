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

#include "common/sonarmeta.h"

#include <gst/gst.h>
#include <glib/gqueue.h>
#include <gst/base/gstaggregator.h>

G_BEGIN_DECLS

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
