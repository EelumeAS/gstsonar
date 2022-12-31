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
#include "openglWp.h"

#include <stdio.h>
#include <sys/time.h>

GST_DEBUG_CATEGORY_STATIC(sonarsink_debug);
#define GST_CAT_DEFAULT sonarsink_debug

#define gst_sonarsink_parent_class parent_class
G_DEFINE_TYPE (GstSonarsink, gst_sonarsink, GST_TYPE_BASE_SINK);

enum
{
  PROP_0,
  PROP_ZOOM,
};

#define DEFAULT_PROP_ZOOM 1

static GstStaticPadTemplate gst_sonarsink_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("sonar/multibeam")
    );

static GstFlowReturn
gst_sonarsink_render (GstBaseSink * basesink, GstBuffer * buf)
{
  GstSonarsink *sonarsink = GST_SONARSINK (basesink);

  GST_DEBUG_OBJECT(sonarsink, "%lu: rendering buffer: %p, width n_beams = %d, resolution = %d", buf->pts, buf, sonarsink->n_beams, sonarsink->resolution);

  GstMapInfo mapinfo;
  if (!gst_buffer_map (buf, &mapinfo, GST_MAP_READ))
  {
    return GST_FLOW_ERROR;
  }

  const char* payload = mapinfo.data + sizeof(packet_header_t) + sizeof(fls_data_header_t);

  for (int range_index=0; range_index < sonarsink->resolution; ++range_index)
  {
    for (int beam_index=0; beam_index < sonarsink->n_beams; ++beam_index)
    {
      float beam_angle = ((const float*)((const uint16_t*)payload + sonarsink->n_beams * sonarsink->resolution))[beam_index];

      uint16_t beam_intensity = ((const uint16_t*)payload)[range_index * sonarsink->n_beams + beam_index];

      const int vertex_index = 3 * (beam_index * sonarsink->resolution + range_index);

      const float amplitude = (float)range_index / (float)sonarsink->resolution;

      sonarsink->vertices[vertex_index] = -sin(beam_angle) * amplitude * sonarsink->zoom;
      sonarsink->vertices[vertex_index+1] = cos(beam_angle) * amplitude * sonarsink->zoom;
      sonarsink->vertices[vertex_index+2] = -1;


      const float iscale = 1.f/3e3;
      float I = beam_intensity * iscale;
      I = I > 1 ? 1 : I;
      sonarsink->colors[vertex_index] = I;
      sonarsink->colors[vertex_index+1] = I;
      sonarsink->colors[vertex_index+2] = I;
    }
  }

  gst_buffer_unmap (buf, &mapinfo);

  updateWp(sonarsink->vertices, sonarsink->colors, sonarsink->n_beams * sonarsink->resolution);

  return GST_FLOW_OK;
}

static gboolean
gst_sonarsink_set_caps (GstBaseSink * basesink, GstCaps * caps)
{
  GstSonarsink *sonarsink = GST_SONARSINK (basesink);

  GstStructure *s = gst_caps_get_structure (caps, 0);
  const GValue *v_framerate, *v_n_beams, *v_resolution;

  GST_DEBUG_OBJECT(sonarsink, "caps structure: %s\n", gst_structure_to_string(s));

  if (((v_n_beams = gst_structure_get_value(s, "n_beams")) == NULL)
    || (!G_VALUE_HOLDS_INT (v_n_beams))
    || ((v_resolution = gst_structure_get_value(s, "resolution")) == NULL)
    || (!G_VALUE_HOLDS_INT (v_resolution)))
  {
    GST_DEBUG_OBJECT(sonarsink, "no details in caps\n");

    return FALSE;
  }
  else
  {
    guint32 n_beams = g_value_get_int (v_n_beams);
    guint32 resolution = g_value_get_int (v_resolution);

    GST_OBJECT_LOCK (sonarsink);
    guint32 old_n_beams = sonarsink->n_beams;
    guint32 old_resolution = sonarsink->resolution;

    sonarsink->n_beams = n_beams;
    sonarsink->resolution = resolution;

    if ((sonarsink->n_beams != old_n_beams)
      || (sonarsink->resolution != old_resolution))
    {
      const int size = sonarsink->n_beams * sonarsink->resolution * 3 * sizeof(sonarsink->vertices[0]);

      free(sonarsink->vertices);
      free(sonarsink->colors);
      sonarsink->vertices = (float*)malloc(size);
      sonarsink->colors = (float*)malloc(size);
    }

    GST_OBJECT_UNLOCK (sonarsink);

    GST_DEBUG_OBJECT(sonarsink, "got caps details n_beams: %d, resolution: %d", n_beams, resolution);

    // initialize visualization once
    if (sonarsink->init_wp)
    {
      sonarsink->init_wp = 0;
      int rc = initWp();
      assert(rc == 0);
    }

    return TRUE;
  }
}

static void
gst_sonarsink_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonarsink *sonarsink = GST_SONARSINK (object);

  GST_OBJECT_LOCK (sonarsink);
  switch (prop_id) {
    case PROP_ZOOM:
      sonarsink->zoom = g_value_get_double (value);
      break;
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
    case PROP_ZOOM:
      g_value_set_double (value, sonarsink->zoom);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonarsink_finalize (GObject * object)
{
  GstSonarsink *sonarsink = GST_SONARSINK (object);

  free(sonarsink->vertices);
  free(sonarsink->colors);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonarsink_class_init (GstSonarsinkClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseSinkClass *basesink_class = (GstBaseSinkClass *) klass;

  gobject_class->finalize     = gst_sonarsink_finalize;
  gobject_class->set_property = gst_sonarsink_set_property;
  gobject_class->get_property = gst_sonarsink_get_property;

  basesink_class->render = GST_DEBUG_FUNCPTR (gst_sonarsink_render);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_sonarsink_set_caps);

  GST_DEBUG_CATEGORY_INIT(sonarsink_debug, "sonarsink", 0, "TODO");

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ZOOM,
      g_param_spec_double ("zoom", "zoom",
          "Zoom", G_MINDOUBLE, G_MAXDOUBLE, DEFAULT_PROP_ZOOM,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

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
  sonarsink->vertices = NULL;
  sonarsink->colors = NULL;
  sonarsink->init_wp = 1;

  sonarsink->zoom = DEFAULT_PROP_ZOOM;
}
