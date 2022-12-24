#ifndef __GST_SONARSINK_H__
#define __GST_SONARSINK_H__

#include <gst/gst.h>

#include <gst/video/gstvideosink.h>

G_BEGIN_DECLS

#define GST_TYPE_SONARSINK \
  (gst_sonarsink_get_type())
#define GST_SONARSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SONARSINK,GstSonarsink))
#define GST_SONARSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SONARSINK,GstSonarsinkClass))
#define GST_IS_SONARSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SONARSINK))
#define GST_IS_SONARSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SONARSINK))

typedef struct _GstSonarsink GstSonarsink;
typedef struct _GstSonarsinkClass GstSonarsinkClass;

struct _GstSonarsink
{
  GstVideoSink videosink;

  /* < private > */
  guint32 n_beams;
  guint32 resolution;
};

struct _GstSonarsinkClass
{
  GstVideoSinkClass parent_class;
};

GType gst_sonarsink_get_type (void);

G_END_DECLS

#endif /* __GST_SONARSINK_H__ */
