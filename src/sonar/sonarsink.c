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
#include "openglWp.h"
#include "sonarparse.h"

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
    GST_STATIC_CAPS ("sonar/multibeam; sonar/bathymetry")
    );

static GstFlowReturn
gst_sonarsink_render (GstBaseSink * basesink, GstBuffer * buf)
{
  GstSonarsink *sonarsink = GST_SONARSINK (basesink);

  GstSonarMeta *meta = GST_SONAR_META_GET(buf);

  GST_DEBUG_OBJECT(sonarsink, "%lu: rendering buffer: %p, width n_beams = %d, resolution = %d, sound_speed = %f, sample_rate = %f, t0 = %d"
    , buf->pts, buf, sonarsink->n_beams, sonarsink->resolution, meta->sound_speed, meta->sample_rate, meta->t0);

  GstMapInfo mapinfo;
  if (!gst_buffer_map (buf, &mapinfo, GST_MAP_READ))
  {
    return GST_FLOW_ERROR;
  }

  switch(sonarsink->wbms_type)
  {
    case WBMS_FLS:
    {
      const uint16_t* beam_intensities = (const uint16_t*)(mapinfo.data + sizeof(packet_header_t) + sizeof(fls_data_header_t));
      const float* beam_angles = (const float*)(beam_intensities + sonarsink->n_beams * sonarsink->resolution);

      for (int range_index=0; range_index < sonarsink->resolution; ++range_index)
      {
        for (int beam_index=0; beam_index < sonarsink->n_beams; ++beam_index)
        {
          uint16_t beam_intensity = beam_intensities[range_index * sonarsink->n_beams + beam_index];
          float beam_angle = beam_angles[beam_index];
          //float range = ((meta->t0 + range_index) * meta->sound_speed) / (2 * meta->sample_rate);

          int vertex_index = 3 * (beam_index * sonarsink->resolution + range_index);
          float* vertex = sonarsink->vertices + vertex_index;

          float range_norm = (float)range_index / (float)sonarsink->resolution;

          vertex[0] = -sin(beam_angle) * range_norm * sonarsink->zoom;
          vertex[1] = -1 + cos(beam_angle) * range_norm * sonarsink->zoom;
          vertex[2] = -1;


          const float iscale = 1.f/3e3;
          float I = beam_intensity * iscale;
          I = I > 1 ? 1 : I;
          float* color = sonarsink->colors + vertex_index;
          color[0] = I;
          color[1] = I;
          color[2] = I;
        }
      }
      break;
    }
    case WBMS_BATH:
    {
      g_assert(sonarsink->resolution == 1);

      const detectionpoint_t* detection_points = (const detectionpoint_t*)(mapinfo.data + sizeof(packet_header_t) + sizeof(bath_data_header_t));

      for (int beam_index=0; beam_index < sonarsink->n_beams; ++beam_index)
      {
        const detectionpoint_t* dp = detection_points + beam_index;

        float range = (dp->sample_number * meta->sound_speed) / (2 * meta->sample_rate);

        int vertex_index = 3 * beam_index;
        float* vertex = sonarsink->vertices + vertex_index;

        vertex[0] = sin(dp->angle) * range * sonarsink->zoom;
        vertex[1] = -cos(dp->angle) * range * sonarsink->zoom;
        vertex[2] = -1;


        float* color = sonarsink->colors + vertex_index;
        color[0] = 1;
        color[1] = 1;
        color[2] = 1;
      }
      break;
    }
  }

  gst_buffer_unmap (buf, &mapinfo);

  // update graphic
  updateWp(sonarsink->vertices, sonarsink->colors, sonarsink->n_beams * sonarsink->resolution);

  // take input from window
  SDL_Event e;
  while ( SDL_PollEvent(&e) ) {
    switch (e.type) {
      case SDL_QUIT:
        return GST_FLOW_EOS;
      break;
      case SDL_KEYDOWN:
      {
        switch (e.key.keysym.sym) {
          case SDLK_ESCAPE:
            return GST_FLOW_EOS;
          case SDLK_SPACE:
          {
            //GstStructure *s = gst_structure_new ("GstMultiFileSink",
            //  "filename", G_TYPE_STRING, filename,
            //  "index", G_TYPE_INT, multifilesink->index,
            //  "timestamp", G_TYPE_UINT64, timestamp,
            //  "stream-time", G_TYPE_UINT64, stream_time,
            //  "running-time", G_TYPE_UINT64, running_time,
            //  "duration", G_TYPE_UINT64, duration,
            //  "offset", G_TYPE_UINT64, offset,
            //  "offset-end", G_TYPE_UINT64, offset_end, NULL);

            GstMessage *msg = gst_message_new_request_state(GST_OBJECT_CAST(sonarsink), sonarsink->playpause);
            gst_element_post_message(GST_ELEMENT_CAST(sonarsink), msg);
            sonarsink->playpause = sonarsink->playpause == GST_STATE_PAUSED ? GST_STATE_PLAYING : GST_STATE_PAUSED;
          }
          break;
        }
      break;
      }
    }
  }

  return GST_FLOW_OK;
}

static gboolean
gst_sonarsink_set_caps (GstBaseSink * basesink, GstCaps * caps)
{
  GstSonarsink *sonarsink = GST_SONARSINK (basesink);

  GstStructure *s = gst_caps_get_structure (caps, 0);
  const GValue *v_framerate, *v_n_beams, *v_resolution;

  GST_DEBUG_OBJECT(sonarsink, "caps structure: %s\n", gst_structure_to_string(s));

  gint n_beams, resolution;
  const gchar *caps_name;

  if ((caps_name = gst_structure_get_name(s))
    && gst_structure_get_int(s, "n_beams", &n_beams)
    && gst_structure_get_int(s, "resolution", &resolution))
  {
    GST_OBJECT_LOCK (sonarsink);
    guint32 old_n_beams = sonarsink->n_beams;
    guint32 old_resolution = sonarsink->resolution;

    GST_DEBUG_OBJECT(sonarsink, "got caps details caps_name: %s, n_beams: %d, resolution: %d", caps_name, n_beams, resolution);
    sonarsink->n_beams = (guint32)n_beams;
    sonarsink->resolution = (guint32)resolution;

    if ((sonarsink->n_beams != old_n_beams)
      || (sonarsink->resolution != old_resolution))
    {
      const int size = sonarsink->n_beams * sonarsink->resolution * 3 * sizeof(sonarsink->vertices[0]);

      free(sonarsink->vertices);
      free(sonarsink->colors);
      sonarsink->vertices = (float*)malloc(size);
      sonarsink->colors = (float*)malloc(size);
    }

    if (strcmp(caps_name, "sonar/multibeam") == 0)
      sonarsink->wbms_type = WBMS_FLS;
    else if (strcmp(caps_name, "sonar/bathymetry") == 0)
      sonarsink->wbms_type = WBMS_BATH;
    else
      g_assert_not_reached();

    GST_OBJECT_UNLOCK (sonarsink);

    // initialize visualization once
    if (sonarsink->init_wp)
    {
      sonarsink->init_wp = 0;
      int rc = initWp();
      g_assert(rc == 0);
    }

    return TRUE;
  }
  else
  {
    GST_DEBUG_OBJECT(sonarsink, "no details in caps\n");

    return FALSE;
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
  sonarsink->playpause = GST_STATE_PAUSED;

  sonarsink->zoom = DEFAULT_PROP_ZOOM;
}
