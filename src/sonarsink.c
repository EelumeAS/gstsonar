/**
 * SECTION:element-gst_sonarsink
 *
 * Sonarsink is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  TODO
 * </refsect2>
 */

#include "sonarsink.h"
#include "navi.h"

#include <stdio.h>
#include <sys/time.h>

#include <gst/video/video.h>

inline double ms()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);

  double ms = (double)(tp.tv_sec)*1000 + (double)(tp.tv_usec)/1000;
  return ms;
}

GST_DEBUG_CATEGORY_STATIC(sonarsink_debug);
#define GST_CAT_DEFAULT sonarsink_debug

#define gst_sonarsink_parent_class parent_class
G_DEFINE_TYPE (GstSonarsink, gst_sonarsink, GST_TYPE_VIDEO_SINK);

static GstStaticPadTemplate gst_sonarsink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw")
    );

static GstFlowReturn
gst_sonarsink_show_frame (GstVideoSink * videosink, GstBuffer * buf)
{
  GstSonarsink *sonarsink = GST_SONARSINK (videosink);

  GST_DEBUG_OBJECT(sonarsink, "rendering buffer: %p", buf);


  return GST_FLOW_OK;
}

static void
gst_sonarsink_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonarsink *sonarsink = GST_SONARSINK (object);

  GST_OBJECT_LOCK (sonarsink);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonarsink);
}

static void
gst_sonarsink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSonarsink *sonarsink = GST_SONARSINK (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonarsink_finalize (GObject * object)
{
  GstSonarsink *sonarsink = GST_SONARSINK (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonarsink_class_init (GstSonarsinkClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstVideoSinkClass *videosink_class = (GstVideoSinkClass *) klass;

  gobject_class->finalize     = gst_sonarsink_finalize;
  gobject_class->set_property = gst_sonarsink_set_property;
  gobject_class->get_property = gst_sonarsink_get_property;

  videosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_sonarsink_show_frame);

  GST_DEBUG_CATEGORY_INIT(sonarsink_debug, "sonarsink", 0, "TODO");


  gst_element_class_set_static_metadata (gstelement_class, "Sonarsink",
      "Sink",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarsink_sink_template);
}

static void
gst_sonarsink_init (GstSonarsink * sonarsink)
{
  sonarsink->n_beams = 0;
  sonarsink->resolution = 0;
}
