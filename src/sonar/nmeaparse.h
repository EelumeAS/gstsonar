#ifndef __GST_NMEAPARSE_H__
#define __GST_NMEAPARSE_H__

#include <gst/gst.h>

#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS

#define GST_TYPE_NMEAPARSE \
  (gst_nmeaparse_get_type())
#define GST_NMEAPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_NMEAPARSE,GstNmeaparse))
#define GST_NMEAPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_NMEAPARSE,GstNmeaparseClass))
#define GST_IS_NMEAPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_NMEAPARSE))
#define GST_IS_NMEAPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_NMEAPARSE))

typedef struct _GstNmeaparse GstNmeaparse;
typedef struct _GstNmeaparseClass GstNmeaparseClass;

typedef struct
{
  float pitch;
  float roll;
  float yaw;
  float latitude;
  float longitude;
  float depth;
  float altitude;
} GstNmeaparseTelemetry;

struct _GstNmeaparse
{
  GstBaseParse baseparse;

  /* < private > */

  GstPad *telsrc; // telemetry source
  guint64 initial_time;
};

struct _GstNmeaparseClass
{
  GstBaseParseClass parent_class;
};

GType gst_nmeaparse_get_type (void);

G_END_DECLS

#endif /* __GST_NMEAPARSE_H__ */