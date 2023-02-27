/**
 * SECTION:element-gst_sonarconvert
 *
 * Sonarconvert is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=sonarparse:5,sonarconvert:5 gst-launch-1.0 filesrc location=../in.sbd ! sonarparse ! sonarconvert ! fakesink
 * </refsect2>
 */

#include "sonarconvert.h"
#include "norbit_wbms.h"

#include <stdio.h>

#include <gst/video/video.h>

GST_DEBUG_CATEGORY_STATIC(sonarconvert_debug);
#define GST_CAT_DEFAULT sonarconvert_debug

#define gst_sonarconvert_parent_class parent_class
G_DEFINE_TYPE (GstSonarconvert, gst_sonarconvert, GST_TYPE_BASE_TRANSFORM);

static GstStaticPadTemplate gst_sonarconvert_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) { GRAY8 }, "
        "width = (int) [ 0, MAX ],"
        "height = (int) [ 0, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ], ")
    );

static GstStaticPadTemplate gst_sonarconvert_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("sonar/multibeam")
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

  GST_DEBUG_OBJECT(sonarconvert, "n_beams: %d, resolution: %d", sonarconvert->n_beams, sonarconvert->resolution);

  const gssize offset = sizeof(wbms_packet_header_t) + sizeof(wbms_fls_data_header_t);
  const gssize size = sonarconvert->n_beams * sonarconvert->resolution * sizeof(guint16);// + sonarconvert->n_beams * sizeof(float);
  gst_buffer_resize(buf, offset, size);

  //GstVideoFormat fmt = gst_video_format_from_string("GRAY8");
  //gst_buffer_add_video_meta(buf, GST_VIDEO_FRAME_FLAG_NONE, fmt, sonarconvert->n_beams, sonarconvert->resolution);
  //GST_DEBUG_OBJECT(sonarconvert, "fmt: %s", gst_video_format_to_string(fmt));

  GST_OBJECT_UNLOCK (sonarconvert);

  return GST_FLOW_OK;
}

static GstCaps *
gst_sonarconvert_transform_caps (GstBaseTransform * basetransform, GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstSonarconvert *sonarconvert = GST_SONARCONVERT (basetransform);

  if (filter)
  {
    GstStructure *tmps = gst_caps_get_structure (filter, 0);
    GST_DEBUG_OBJECT(sonarconvert, "filter structure: %s", gst_structure_to_string(tmps));
  }
  else
  {
    GST_DEBUG_OBJECT(sonarconvert, "filter structure: <none>");
  }

  if (direction == GST_PAD_SRC)
  {
    GstStructure *s = gst_caps_get_structure (caps, 0);
    GST_DEBUG_OBJECT(sonarconvert, "src structure: %s", gst_structure_to_string(s));

    GstCaps *sink_caps = gst_caps_new_simple ("sonar/multibeam", NULL);
    //GstCaps *sink_caps = gst_caps_new_simple ("sonar/multibeam", "n_beams", G_TYPE_INT, 0, "resolution", G_TYPE_INT, 0, "framerate", GST_TYPE_FRACTION, 0, 1, NULL);

    return sink_caps;
  }
  else if (direction == GST_PAD_SINK)
  {
    GstStructure *s = gst_caps_get_structure (caps, 0);

    GST_DEBUG_OBJECT(sonarconvert, "sink structure: %s", gst_structure_to_string(s));
    guint32 framerate_n, framerate_d, n_beams, resolution;

    if (gst_structure_get_fraction(s, "framerate", &framerate_n, &framerate_d)
      && gst_structure_get_int(s, "n_beams", &n_beams)
      && gst_structure_get_int(s, "resolution", &resolution))
    {
      GST_OBJECT_LOCK (sonarconvert);
      sonarconvert->n_beams = n_beams;
      sonarconvert->resolution = resolution;
      GST_OBJECT_UNLOCK (sonarconvert);

      GST_DEBUG_OBJECT(sonarconvert, "got caps details framerate: %d/%d, n_beams: %d, resolution: %d", framerate_n, framerate_d, n_beams, resolution);

      return gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "GRAY8", "width", G_TYPE_INT, n_beams, "height", G_TYPE_INT, resolution, "framerate", GST_TYPE_FRACTION, framerate_n, framerate_d, NULL);
    }
    else
    {
      GST_DEBUG_OBJECT(sonarconvert, "no details in caps\n");

      return gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "GRAY8", NULL);
    }
  }
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

  gobject_class->finalize     = gst_sonarconvert_finalize;
  gobject_class->set_property = gst_sonarconvert_set_property;
  gobject_class->get_property = gst_sonarconvert_get_property;

  GST_DEBUG_CATEGORY_INIT(sonarconvert_debug, "sonarconvert", 0, "TODO");


  gst_element_class_set_static_metadata (gstelement_class, "Sonarconvert",
      "Transform",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarconvert_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarconvert_src_template);

  basetransform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_sonarconvert_transform_ip);
  basetransform_class->transform_caps = GST_DEBUG_FUNCPTR (gst_sonarconvert_transform_caps);
}

static void
gst_sonarconvert_init (GstSonarconvert * sonarconvert)
{
  sonarconvert->n_beams = 0;
  sonarconvert->resolution = 0;
}
