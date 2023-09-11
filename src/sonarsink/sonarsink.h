// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_SONARSINK_H__
#define __GST_SONARSINK_H__

#include <gst/base/gstbasesink.h>
#include <gst/gst.h>

#include "common/sonarmeta.h"

G_BEGIN_DECLS

#define GST_TYPE_SONARSINK            (gst_sonarsink_get_type())
#define GST_SONARSINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SONARSINK, GstSonarsink))
#define GST_SONARSINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SONARSINK, GstSonarsinkClass))
#define GST_IS_SONARSINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SONARSINK))
#define GST_IS_SONARSINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SONARSINK))

typedef struct _GstSonarsink GstSonarsink;
typedef struct _GstSonarsinkClass GstSonarsinkClass;

struct _GstSonarsink
{
    GstBaseSink basesink;

    /* < private > */
    GstSonarType sonar_type;
    guint32 n_beams;
    guint32 resolution;
    gboolean detected;    // if detection was run on the data

    float* vertices;
    float* colors;

    // initializer flag for openglWp
    int init_wp;

    GstState playpause;    // which state to set on next space press

    float zoom;    // factor to scale ranges with
    float gain;    // factor to scale intensities with
};

struct _GstSonarsinkClass
{
    GstBaseSinkClass parent_class;
};

GType gst_sonarsink_get_type(void);

G_END_DECLS

#endif /* __GST_SONARSINK_H__ */
