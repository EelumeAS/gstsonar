/**
 * SECTION:element-gst_sbdmux
 *
 * Muxer for interpolating telemetry data over sonar data
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  SBD=../samples/in.sbd && GST_PLUGIN_PATH=. GST_DEBUG=2,sbdmux:6 gst-launch-1.0 filesrc location=$SBD ! sonarparse ! sbdmux name=mux ! sonardetect ! sonarsink filesrc location=$SBD ! nmeaparse ! mux.
 * </refsect2>
 */

#include "sbdmux.h"
#include "norbit_wbms.h"
#include "nmeaparse.h"
#include "linalg.h"
#include "sbd.h"

#include <stdio.h>

#define SONAR_CAPS "sonar/multibeam; sonar/bathymetry"
#define TELEMETRY_CAPS "application/nmea"
#define SBD_CAPS "application/sbd"

GST_DEBUG_CATEGORY_STATIC(sbdmux_debug);
#define GST_CAT_DEFAULT sbdmux_debug

#define gst_sbdmux_parent_class parent_class
G_DEFINE_TYPE (GstSbdmux, gst_sbdmux, GST_TYPE_AGGREGATOR);

enum
{
  PROP_0,
  PROP_HEADER,
};

#define DEFAULT_PROP_HEADER ""

static GstStaticPadTemplate gst_sbdmux_sonar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sonar",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (SONAR_CAPS)
    );

static GstStaticPadTemplate gst_sbdmux_telemetry_sink_template =
GST_STATIC_PAD_TEMPLATE ("tel",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (TELEMETRY_CAPS)
    );

static GstStaticPadTemplate gst_sbdmux_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SBD_CAPS)
    );

static void gst_sbdmux_free_buf(gpointer data)
{
  GstBuffer *buf = (GstBuffer*) data;
  gst_buffer_unref(buf);
}

// called when exactly one buffer is queued on both sinks
// (see https://gstreamer.freedesktop.org/documentation/base/gstaggregator.html?gi-language=c)
static GstFlowReturn
gst_sbdmux_aggregate (GstAggregator * aggregator, gboolean timeout)
{
  GstSbdmux *sbdmux = GST_SBDMUX (aggregator);

  //GST_TRACE_OBJECT(sbdmux, "aggregate");

  GstFlowReturn ret = GST_FLOW_ERROR;

  if (sbdmux->write_header && sbdmux->header_path && *sbdmux->header_path)
  {
    sbdmux->write_header = FALSE;

    FILE *header_file = fopen(sbdmux->header_path, "rb");
    if (!header_file)
    {
      GST_ERROR_OBJECT(sbdmux, "couldn't open header at %s for reading: %s", sbdmux->header_path, strerror(errno));
      ret = GST_FLOW_ERROR;
    }
    else
    {
      fseek(header_file, 0, SEEK_END);
      int header_size = ftell(header_file);
      fseek(header_file, 0, SEEK_SET);
      char *header = (char*)malloc(header_size);

      if (!header)
        GST_ERROR_OBJECT(sbdmux, "couldn't allocate %lu bytes: %s", header_size, strerror(errno));
      else if (fread(header, 1, header_size, header_file) != header_size)
        GST_ERROR_OBJECT(sbdmux, "couldn't read %lu bytes from %s: %s", header_size, sbdmux->header_path, strerror(errno));
      else
      {
        GstBuffer *header_buf = gst_buffer_new_wrapped(header, header_size);
        header_buf->pts = header_buf->dts = 0;

        GstBuffer *header_entry = sbd_entry(header_buf);
        ret = gst_aggregator_finish_buffer(aggregator, header_entry);
      }
      fclose(header_file);
    }
  }
  else
  {
    if (!sbdmux->sonarbuf)
      sbdmux->sonarbuf = sbd_entry(gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sbdmux->sonarsink));

    if (!sbdmux->telbuf)
      sbdmux->telbuf = sbd_entry(gst_aggregator_pad_pop_buffer((GstAggregatorPad*)sbdmux->telsink));

    if (!sbdmux->sonarbuf && !sbdmux->telbuf)
    {
      GST_WARNING_OBJECT(sbdmux, "no more buffers");
      ret = GST_FLOW_EOS;
    }
    else if (!sbdmux->sonarbuf)
    {
      ret = gst_aggregator_finish_buffer(aggregator, sbdmux->telbuf);
      sbdmux->telbuf = NULL;
    }
    else if (!sbdmux->telbuf || (sbdmux->telbuf->pts > sbdmux->sonarbuf->pts))
    {
      ret = gst_aggregator_finish_buffer(aggregator, sbdmux->sonarbuf);
      sbdmux->sonarbuf = NULL;
    }
    else
    {
      ret = gst_aggregator_finish_buffer(aggregator, sbdmux->telbuf);
      sbdmux->telbuf = NULL;
    }
  }

  return ret;
}

static GstAggregatorPad *
gst_sbdmux_create_new_pad (GstAggregator * aggregator, GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstSbdmux *sbdmux = GST_SBDMUX (aggregator);

  const gchar *templ_caps = gst_caps_to_string(templ->caps);

  GST_DEBUG_OBJECT(sbdmux, "create_new_pad: req_name: %s, template: %s, direction: %d", req_name, templ_caps, templ->direction);

  GstAggregatorPad * pad = g_object_new (GST_TYPE_SBDMUX_PAD, "name", req_name, "direction", templ->direction, "template", templ, NULL);

  if (strcmp(templ_caps, SONAR_CAPS) == 0)
    sbdmux->sonarsink = (GstPad*)pad;
  else if (strcmp(templ_caps, TELEMETRY_CAPS) == 0)
    sbdmux->telsink = (GstPad*)pad;
  else
    g_assert_not_reached();

  return pad;
}

static gboolean gst_sbdmux_update_src_caps (GstAggregator * aggregator, GstCaps *caps, GstCaps **ret)
{
  GstSbdmux *sbdmux = GST_SBDMUX (aggregator);

  *ret = gst_caps_new_simple ("application/sbd", NULL);

  return TRUE;
}

static void
gst_sbdmux_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSbdmux *sbdmux = GST_SBDMUX (object);

  GST_OBJECT_LOCK (sbdmux);
  switch (prop_id) {
    case PROP_HEADER:
      g_free (sbdmux->header_path);
      sbdmux->header_path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sbdmux);
}

static void
gst_sbdmux_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSbdmux *sbdmux = GST_SBDMUX (object);

  GST_OBJECT_LOCK (sbdmux);
  switch (prop_id) {
    case PROP_HEADER:
      g_value_set_string (value, sbdmux->header_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sbdmux);
}

static void
gst_sbdmux_finalize (GObject * object)
{
  GstSbdmux *sbdmux = GST_SBDMUX (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sbdmux_class_init (GstSbdmuxClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAggregatorClass *aggregator_class = (GstAggregatorClass *) klass;

  gobject_class->finalize     = gst_sbdmux_finalize;
  gobject_class->set_property = gst_sbdmux_set_property;
  gobject_class->get_property = gst_sbdmux_get_property;

  aggregator_class->aggregate = GST_DEBUG_FUNCPTR (gst_sbdmux_aggregate);
  aggregator_class->create_new_pad = GST_DEBUG_FUNCPTR (gst_sbdmux_create_new_pad);
  aggregator_class->update_src_caps = GST_DEBUG_FUNCPTR (gst_sbdmux_update_src_caps);

  GST_DEBUG_CATEGORY_INIT(sbdmux_debug, "sbdmux", 0, "sbdmux");

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_HEADER,
      g_param_spec_string ("header", "header",
          "path to sbd header", DEFAULT_PROP_HEADER,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class, "Sbdmux",
      "Sink",
      "Muxer for splicing telemetry data and sonar data",
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sbdmux_sonar_sink_template, GST_TYPE_SBDMUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sbdmux_telemetry_sink_template, GST_TYPE_SBDMUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class, &gst_sbdmux_src_template, GST_TYPE_SBDMUX_PAD);
}

static void
gst_sbdmux_init (GstSbdmux * sbdmux)
{
  sbdmux->header_path = g_strdup(DEFAULT_PROP_HEADER);
  sbdmux->write_header = TRUE;
  sbdmux->sonarbuf = NULL;
  sbdmux->telbuf = NULL;
}

// custom pad
static void
gst_sbdmux_pad_class_init (GstSbdmuxPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
}

static void
gst_sbdmux_pad_init (GstSbdmuxPad * pad)
{
}

G_DEFINE_TYPE (GstSbdmuxPad, gst_sbdmux_pad, GST_TYPE_AGGREGATOR_PAD);
