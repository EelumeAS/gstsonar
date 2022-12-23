/**
 * SECTION:element-gst_sonarconvert
 *
 * Sonarconvert is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  gst-launch-1.0 -v -e TODO
 * </refsect2>
 */

#include "sonarconvert.h"
#include "navi.h"

#include <stdio.h>
#include <sys/time.h>

#include <gst/base/gstbytereader.h>

inline double ms()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);

  double ms = (double)(tp.tv_sec)*1000 + (double)(tp.tv_usec)/1000;
  return ms;
}

GST_DEBUG_CATEGORY_STATIC(sonarconvert_debug);
#define GST_CAT_DEFAULT sonarconvert_debug

#define gst_sonarconvert_parent_class parent_class
G_DEFINE_TYPE (GstSonarconvert, gst_sonarconvert, GST_TYPE_BASE_TRANSFORM);

static GstStaticPadTemplate gst_sonarconvert_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/x-raw, "
        "format = (string) { GRAY8 }, "
        "width = (int) [ 0, MAX ],"
        "height = (int) [ 0, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ], ")
    );

static GstStaticPadTemplate gst_sonarconvert_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("sonar/multibeam, "
        "format = (string) { norbit }, "
        "n_beams = (int) [ 0, MAX ],"
        "resolution = (int) [ 0, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ], "
        "parsed = (boolean) true")
    );

static GstFlowReturn
gst_sonarconvert_transform_ip (GstBaseTransform * basetransform, GstBuffer * buf)
{
  GstSonarconvert *sonarconvert = GST_SONARCONVERT (basetransform);

  GST_DEBUG_OBJECT(sonarconvert, "dts: %llu, pts: %llu", buf->dts, buf->pts);

  // profile time:
  //static double start = 0;
  //GST_WARNING_OBJECT(dragon, "%f", ms() - start);
  //start = ms();

  GST_OBJECT_LOCK (sonarconvert);


  GST_OBJECT_UNLOCK (sonarconvert);

  return GST_FLOW_OK;
}

static void
gst_sonarconvert_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonarconvert *sonarconvert = GST_SONARCONVERT (object);

  GST_OBJECT_LOCK (sonarconvert);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonarconvert);
}

static void
gst_sonarconvert_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSonarconvert *sonarconvert = GST_SONARCONVERT (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonarconvert_finalize (GObject * object)
{
  GstSonarconvert *sonarconvert = GST_SONARCONVERT (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonarconvert_class_init (GstSonarconvertClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;
  GstBaseTransformClass *base_class = (GstBaseTransformClass *) klass;

  gobject_class->finalize     = gst_sonarconvert_finalize;
  gobject_class->set_property = gst_sonarconvert_set_property;
  gobject_class->get_property = gst_sonarconvert_get_property;

  GST_DEBUG_CATEGORY_INIT(sonarconvert_debug, "sonarconvert", 0, "Fix for double buffer bug in sonix c1/c1-pro cameras");


  gst_element_class_set_static_metadata (gstelement_class, "Sonarconvert",
      "Transform",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarconvert_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarconvert_src_template);

  basetransform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_sonarconvert_transform_ip);
}

static void
gst_sonarconvert_init (GstSonarconvert * sonarconvert)
{
}
