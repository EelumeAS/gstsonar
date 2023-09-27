// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_NMEAPARSE_H__
#define __GST_NMEAPARSE_H__

#include <gst/gst.h>

#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS

#define GST_TYPE_NMEAPARSE            (gst_nmeaparse_get_type())
#define GST_NMEAPARSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_NMEAPARSE, GstNmeaparse))
#define GST_NMEAPARSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_NMEAPARSE, GstNmeaparseClass))
#define GST_IS_NMEAPARSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_NMEAPARSE))
#define GST_IS_NMEAPARSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_NMEAPARSE))

typedef struct _GstNmeaparse GstNmeaparse;
typedef struct _GstNmeaparseClass GstNmeaparseClass;

struct _GstNmeaparse
{
    GstBaseParse baseparse;

    /* < private > */

    guint64 initial_time;
    int timestamp_offset;
};

struct _GstNmeaparseClass
{
    GstBaseParseClass parent_class;
};

GType gst_nmeaparse_get_type(void);

G_END_DECLS

#endif /* __GST_NMEAPARSE_H__ */
