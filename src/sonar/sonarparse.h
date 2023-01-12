
#ifndef __GST_SONARPARSE_H__
#define __GST_SONARPARSE_H__

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>

#include "navi.h"

G_BEGIN_DECLS

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

  gfloat sound_speed;
  gfloat sample_rate;
  guint32 t0;
};

struct _GstSonarparseClass
{
  GstBaseParseClass parent_class;
};

GType gst_sonarparse_get_type (void);

// sonar meta
typedef struct
{
  GstMeta meta;

  float sound_speed; // Filtered sanitized sound speed in m/s
  float sample_rate; // Sample rate in reported range sample index, in Hz
  int t0; // Sample index of first sample in each beam
} GstSonarMeta;

GType gst_sonar_meta_api_get_type (void);
const GstMetaInfo * gst_sonar_meta_get_info (void);
#define GST_SONAR_META_GET(buf) ((GstSonarMeta *)gst_buffer_get_meta(buf,gst_sonar_meta_api_get_type()))
#define GST_SONAR_META_ADD(buf) ((GstSonarMeta *)gst_buffer_add_meta(buf,gst_sonar_meta_get_info(),NULL))

typedef struct
{
  GMutex m;
  guint64 initial_time;
} GstSonarSharedData;

extern GstSonarSharedData gst_sonar_shared_data; // FIXME: This is a global variable for syncronizing time, limiting the scalability of the sonar elements

G_END_DECLS

#endif /* __GST_SONARPARSE_H__ */
