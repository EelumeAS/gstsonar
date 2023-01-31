
#ifndef __GST_SONARPARSE_H__
#define __GST_SONARPARSE_H__

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>

#include "navi.h"
#include "nmeaparse.h"

G_BEGIN_DECLS

// sonar meta
GType gst_sonar_meta_api_get_type (void);
const GstMetaInfo * gst_sonar_meta_get_info (void);
#define GST_SONAR_META_GET(buf) ((GstSonarMeta *)gst_buffer_get_meta(buf,gst_sonar_meta_api_get_type()))
#define GST_SONAR_META_ADD(buf) ((GstSonarMeta *)gst_buffer_add_meta(buf,gst_sonar_meta_get_info(),NULL))

typedef struct
{
  float sound_speed; // Filtered sanitized sound speed in m/s
  float sample_rate; // Sample rate in reported range sample index, in Hz
  int t0; // Sample index of first sample in each beam
  float gain; // Intensity value gain

} GstSonarMetaData;

typedef struct
{
  GstMeta meta;
  GstSonarMetaData data;

} GstSonarMeta;

// sonarparse
#define GST_TYPE_SONARPARSE \
  (gst_sonarparse_get_type())
#define GST_SONARPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SONARPARSE,GstSonarparse))
#define GST_SONARPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SONARPARSE,GstSonarparseClass))
#define GST_IS_SONARPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SONARPARSE))
#define GST_IS_SONARPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SONARPARSE))

typedef struct _GstSonarparse GstSonarparse;
typedef struct _GstSonarparseClass GstSonarparseClass;

struct _GstSonarparse
{
  GstBaseParse baseparse;

  /* < private > */
  wbms_type_t wbms_type;
  guint32 n_beams;
  guint32 resolution;
  guint32 framerate;
  gchar *caps_name;

  guint64 initial_time;

  GstSonarMetaData next_meta_data;
};

struct _GstSonarparseClass
{
  GstBaseParseClass parent_class;
};

GType gst_sonarparse_get_type (void);

G_END_DECLS

#endif /* __GST_SONARPARSE_H__ */
