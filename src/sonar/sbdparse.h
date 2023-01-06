#ifndef __GST_SBDPARSE_H__
#define __GST_SBDPARSE_H__

#include <gst/gst.h>

#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS

#define GST_TYPE_SBDPARSE \
  (gst_sbdparse_get_type())
#define GST_SBDPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SBDPARSE,GstSbdparse))
#define GST_SBDPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SBDPARSE,GstSbdparseClass))
#define GST_IS_SBDPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SBDPARSE))
#define GST_IS_SBDPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SBDPARSE))

typedef struct _GstSbdparse GstSbdparse;
typedef struct _GstSbdparseClass GstSbdparseClass;

typedef struct
{
  float pitch;
  float roll;
  float yaw;
  float latitude;
  float longitude;
  float depth;
  float altitude;
} GstSbdparseTelemetry;

struct _GstSbdparse
{
  GstBaseParse baseparse;

  /* < private > */

  GstPad *telsrc; // telemetry source
  guint64 initial_time;
};

struct _GstSbdparseClass
{
  GstBaseParseClass parent_class;
};

GType gst_sbdparse_get_type (void);

G_END_DECLS

#endif /* __GST_SBDPARSE_H__ */
