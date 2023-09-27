// ***************************************************************************
// Part of Gstsonar - Gstreamer sonar processing plugins
//
// Copyright (c) 2023 Eelume AS <opensource@eelume.com>
// All rights reserved
//
// Licensed under the LGPL v2.1 License.
// See LICENSE file in the project root for full license information.
// ***************************************************************************
#ifndef __GST_EEL_NMEA_DEC_H__
#define __GST_EEL_NMEA_DEC_H__

#include <gst/gst.h>

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_EEL_NMEA_DEC            (gst_eelnmeadec_get_type())
#define GST_EEL_NMEA_DEC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_EEL_NMEA_DEC, GstEelNmeaDec))
#define GST_EEL_NMEA_DEC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_EEL_NMEA_DEC, GstEelNmeaDecClass))
#define GST_IS_EEL_NMEA_DEC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_EEL_NMEA_DEC))
#define GST_IS_EEL_NMEA_DEC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_EEL_NMEA_DEC))

typedef struct _GstEelNmeaDec GstEelNmeaDec;
typedef struct _GstEelNmeaDecClass GstEelNmeaDecClass;

struct _GstEelNmeaDec
{
    GstBaseTransform basetransform;

    guint64 initial_time;
};

struct _GstEelNmeaDecClass
{
    GstBaseTransformClass parent_class;
};

GType gst_eelnmeadec_get_type(void);

G_END_DECLS

#endif /* __GST_EEL_NMEA_DEC_H__ */
