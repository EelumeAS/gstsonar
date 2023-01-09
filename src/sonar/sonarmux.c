/**
 * SECTION:element-gst_sonarmux
 *
 * Sonarmux is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  TODO
 * </refsect2>
 */

#include "sonarmux.h"
#include "navi.h"
#include "sonarparse.h"

#include <stdio.h>
#include <sys/time.h>

GST_DEBUG_CATEGORY_STATIC(sonarmux_debug);
#define GST_CAT_DEFAULT sonarmux_debug

#define gst_sonarmux_parent_class parent_class
G_DEFINE_TYPE (GstSonarmux, gst_sonarmux, GST_TYPE_AGGREGATOR);

enum
{
  PROP_0,
};

static GstStaticPadTemplate gst_sonarmux_sonar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sonar",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("sonar/multibeam")
    );

static GstStaticPadTemplate gst_sonarmux_telemetry_sink_template =
GST_STATIC_PAD_TEMPLATE ("tel",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("application/telemetry")
    );

static GstStaticPadTemplate gst_sonarmux_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("sonar/multibeam")
    );


static GstFlowReturn
gst_sonarmux_aggregate (GstAggregator * aggregator, gboolean timeout)
{
  GstSonarmux *sonarmux = GST_SONARMUX (aggregator);
  GST_DEBUG_OBJECT(sonarmux, "aggregate");

  //for (GList *l = GST_ELEMENT_CAST(sonarmux)->sinkpads; l; l = l->next)
  //{
  //  GstPad *pad = (GstPad*)l->data;
  //  GstCaps *caps = gst_pad_get_current_caps(pad);
  //  GstStructure *s = gst_caps_get_structure (caps, 0);
  //  GST_DEBUG_OBJECT(sonarmux, "caps structure: %s", gst_structure_to_string(s));
  //}

  while (gst_aggregator_pad_has_buffer((GstAggregatorPad*)sonarmux->telsink))
  {
    GST_DEBUG_OBJECT(sonarmux, "dropping buffer");
    gst_aggregator_pad_drop_buffer((GstAggregatorPad*)sonarmux->telsink);
    //GstBuffer *buf = gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sonarmux->telsink);
    //if (buf)
    //{
    //  GST_DEBUG_OBJECT(sonarmux, "tel time of buf %p: %llu", buf, buf->pts);
    //  gst_buffer_unref(buf);
    //}
  }

  GstBuffer *buf = gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sonarmux->sonarsink);
  if (buf)
  {
    //GstSonarMeta *meta = GST_SONAR_META_GET(buf);
    //GST_AGGREGATOR_PAD(aggregator->srcpad)->segment.position = GST_BUFFER_PTS (buf);
    GST_DEBUG_OBJECT(sonarmux, "got buffer %p with time %llu", buf, buf->pts);
    return gst_aggregator_finish_buffer(aggregator, buf);
  }
  else
    //return GST_FLOW_OK;
    return GST_AGGREGATOR_FLOW_NEED_DATA;
}

static GstAggregatorPad *
gst_sonarmux_create_new_pad (GstAggregator * aggregator, GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstSonarmux *sonarmux = GST_SONARMUX (aggregator);

  GST_DEBUG_OBJECT(sonarmux, "create_new_pad: req_name: %s, template: %s, direction: %d", req_name, gst_caps_to_string(templ->caps), templ->direction);

  GstAggregatorPad * pad = g_object_new (GST_TYPE_SONARMUX_PAD, "name", req_name, "direction", templ->direction, "template", templ, NULL);

  if (strcmp(req_name, "sonar") == 0)
    sonarmux->sonarsink = (GstPad*)pad;
  else if (strcmp(req_name, "tel") == 0)
    sonarmux->telsink = (GstPad*)pad;
  else
    g_assert_not_reached();

  return pad;
}

static gboolean gst_sonarmux_update_src_caps (GstAggregator * aggregator, GstCaps *caps, GstCaps **ret)
{
  GstSonarmux *sonarmux = GST_SONARMUX (aggregator);

  GstCaps *sonarcaps = gst_pad_get_current_caps(sonarmux->sonarsink);
  GST_DEBUG_OBJECT(sonarmux, "sonarsink caps: %s\nother caps: %s", gst_caps_to_string(sonarcaps), gst_caps_to_string(caps));

  *ret = gst_caps_copy(sonarcaps);
  return TRUE;
}

static void
gst_sonarmux_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonarmux *sonarmux = GST_SONARMUX (object);

  GST_OBJECT_LOCK (sonarmux);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonarmux);
}

static void
gst_sonarmux_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSonarmux *sonarmux = GST_SONARMUX (object);

  GST_OBJECT_LOCK (sonarmux);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonarmux);
}

static void
gst_sonarmux_finalize (GObject * object)
{
  GstSonarmux *sonarmux = GST_SONARMUX (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonarmux_class_init (GstSonarmuxClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAggregatorClass *aggregator_class = (GstAggregatorClass *) klass;

  gobject_class->finalize     = gst_sonarmux_finalize;
  gobject_class->set_property = gst_sonarmux_set_property;
  gobject_class->get_property = gst_sonarmux_get_property;

  aggregator_class->aggregate = GST_DEBUG_FUNCPTR (gst_sonarmux_aggregate);
  aggregator_class->create_new_pad = GST_DEBUG_FUNCPTR (gst_sonarmux_create_new_pad);
  aggregator_class->update_src_caps = GST_DEBUG_FUNCPTR (gst_sonarmux_update_src_caps);

  GST_DEBUG_CATEGORY_INIT(sonarmux_debug, "sonarmux", 0, "TODO");

  gst_element_class_set_static_metadata (gstelement_class, "Sonarmux",
      "Sink",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sonarmux_sonar_sink_template, GST_TYPE_SONARMUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sonarmux_telemetry_sink_template, GST_TYPE_SONARMUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sonarmux_src_template, GST_TYPE_SONARMUX_PAD);
}

static void
gst_sonarmux_init (GstSonarmux * sonarmux)
{
}

static void
gst_sonarmux_pad_class_init (GstSonarmuxPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
}

static void
gst_sonarmux_pad_init (GstSonarmuxPad * pad)
{
  //pad->configured_caps = NULL;
}

G_DEFINE_TYPE (GstSonarmuxPad, gst_sonarmux_pad, GST_TYPE_AGGREGATOR_PAD);
