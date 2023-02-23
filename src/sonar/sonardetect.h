#ifndef __GST_SONARDETECT_H__
#define __GST_SONARDETECT_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

#include "sonarparse.h"
#include "sonarmux.h"
#include "navi.h"

#ifdef __cplusplus
extern "C" {
#endif

G_BEGIN_DECLS

// sonardetect
#define GST_TYPE_SONARDETECT \
  (gst_sonardetect_get_type())
#define GST_SONARDETECT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SONARDETECT,GstSonardetect))
#define GST_SONARDETECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SONARDETECT,GstSonardetectClass))
#define GST_IS_SONARDETECT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SONARDETECT))
#define GST_IS_SONARDETECT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SONARDETECT))

typedef struct _GstSonardetect GstSonardetect;
typedef struct _GstSonardetectClass GstSonardetectClass;

struct _GstSonardetect
{
  GstBaseTransform basetransform;

  wbms_type_t wbms_type;
  guint32 n_beams;
  guint32 resolution;
  gboolean has_telemetry;
};

struct _GstSonardetectClass
{
  GstBaseTransformClass parent_class;
};

GType gst_sonardetect_get_type (void);

G_END_DECLS


// sonardetect.cpp functions

// stores the index of the first detected point in the first range index for each beam
void sonardetect_detect(uint64_t timestamp, uint8_t* sonar_data, int n_beams, int resolution, const GstSonarMetaData *meta_data, const GstSonarTelemetry* tel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __GST_SONARDETECT_H__ */
