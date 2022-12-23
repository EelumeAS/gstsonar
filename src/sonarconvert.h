
#ifndef __GST_SONARCONVERT_H__
#define __GST_SONARCONVERT_H__

#include <gst/gst.h>

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_SONARCONVERT \
  (gst_sonarconvert_get_type())
#define GST_SONARCONVERT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SONARCONVERT,GstSonarconvert))
#define GST_SONARCONVERT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SONARCONVERT,GstSonarconvertClass))
#define GST_IS_SONARCONVERT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SONARCONVERT))
#define GST_IS_SONARCONVERT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SONARCONVERT))

typedef struct _GstSonarconvert GstSonarconvert;
typedef struct _GstSonarconvertClass GstSonarconvertClass;

struct _GstSonarconvert
{
  GstBaseTransform basetransform;

  /* < private > */
};

struct _GstSonarconvertClass
{
  GstBaseTransformClass parent_class;
};

GType gst_sonarconvert_get_type (void);

G_END_DECLS

#endif /* __GST_SONARCONVERT_H__ */
