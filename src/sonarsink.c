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
//#include "openglWp.h"

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

const GLchar* gst_sonarsink_vertex_source = R"glsl(
#version 300 es
precision mediump float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
smooth out vec4 Color;
void main()
{
    gl_Position = vec4(position, 1.0);
    Color = vec4(color,1);
}
)glsl";

const GLchar* gst_sonarsink_fragment_source = R"glsl(
#version 300 es
precision mediump float;
smooth in vec4 Color;
out vec4 outColor;
void main()
{
	outColor = Color;
}
)glsl";

#define SUPPORTED_GL_APIS GST_GL_API_OPENGL | GST_GL_API_GLES2 | GST_GL_API_OPENGL3

GST_DEBUG_CATEGORY_STATIC(sonarsink_debug);
#define GST_CAT_DEFAULT sonarsink_debug

static void
gst_sonarsink_set_window_handle (GstVideoOverlay * overlay, guintptr id)
{
  GstSonarsink *sonarsink = GST_SONARSINK (overlay);
  guintptr window_id = (guintptr) id;

  g_return_if_fail (GST_IS_SONARSINK (overlay));

  GST_DEBUG ("set_xwindow_id %" G_GUINT64_FORMAT, (guint64) window_id);

  sonarsink->new_window_id = window_id;
}


static void
gst_sonarsink_expose (GstVideoOverlay * overlay)
{
  GstSonarsink *sonarsink = GST_SONARSINK (overlay);

  ///* redisplay opengl scene */
  //if (sonarsink->display) {
  //  if (sonarsink->window_id
  //      && sonarsink->window_id != sonarsink->new_window_id) {
  //    GstGLWindow *window = gst_gl_context_get_window (sonarsink->context);

  //    sonarsink->window_id = sonarsink->new_window_id;
  //    gst_gl_window_set_window_handle (window, sonarsink->window_id);

  //    gst_object_unref (window);
  //  }

  //  gst_sonarsink_redisplay (sonarsink);
  //}
}

static void
gst_sonarsink_handle_events (GstVideoOverlay * overlay, gboolean handle_events)
{
  GstSonarsink *sonarsink = GST_SONARSINK (overlay);

  sonarsink->handle_events = handle_events;
  if (G_LIKELY (sonarsink->context)) {
    GstGLWindow *window;
    window = gst_gl_context_get_window (sonarsink->context);
    gst_gl_window_handle_events (window, handle_events);
    gst_object_unref (window);
  }
}

static void
gst_sonarsink_video_overlay_init (GstVideoOverlayInterface * iface)
{
  iface->set_window_handle = gst_sonarsink_set_window_handle;
  //iface->set_render_rectangle = gst_sonarsink_set_render_rectangle;
  iface->handle_events = gst_sonarsink_handle_events;
  iface->expose = gst_sonarsink_expose;
}


#define gst_sonarsink_parent_class parent_class
//G_DEFINE_TYPE (GstSonarsink, gst_sonarsink, GST_TYPE_BASE_SINK);
G_DEFINE_TYPE_WITH_CODE (GstSonarsink, gst_sonarsink, GST_TYPE_BASE_SINK,
  G_IMPLEMENT_INTERFACE (GST_TYPE_VIDEO_OVERLAY, gst_sonarsink_video_overlay_init)
  );

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

  GST_DEBUG_OBJECT(sonarsink, "rendering buffer: %p, width n_beams = %d, resolution = %d", buf, sonarsink->n_beams, sonarsink->resolution);

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

      const float scale = 5;
      const float amplitude = (float)range_index / (float)sonarsink->resolution;

      sonarsink->vertices[vertex_index] = -sin(beam_angle) * amplitude * scale;
      sonarsink->vertices[vertex_index+1] = cos(beam_angle) * amplitude * scale;
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

  //updateWp(sonarsink->vertices, sonarsink->colors, sonarsink->n_beams * sonarsink->resolution);

  if (g_atomic_int_get (&sonarsink->to_quit) != 0) {
    GST_ELEMENT_ERROR (sonarsink, RESOURCE, NOT_FOUND,
        ("%s", "Quit requested"), (NULL));
    return GST_FLOW_ERROR;
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

    return TRUE;
  }
}

static void
gst_sonarsink_set_gl_context (GstSonarsink * sonarsink, GstGLContext * context)
{
  GST_OBJECT_LOCK (sonarsink);
  if (sonarsink->context)
    gst_object_unref (sonarsink->context);

  sonarsink->context = context;
  GST_OBJECT_UNLOCK (sonarsink);
}

static void
gst_sonarsink_set_other_context (GstSonarsink * sonarsink, GstGLContext * other_context)
{
  GST_OBJECT_LOCK (sonarsink);
  if (sonarsink->other_context)
    gst_object_unref (sonarsink->other_context);

  sonarsink->other_context = other_context;
  GST_OBJECT_UNLOCK (sonarsink);
}

static void
gst_sonarsink_set_display (GstSonarsink * sonarsink, GstGLDisplay * display)
{
  GST_OBJECT_LOCK (sonarsink);
  if (sonarsink->display)
    gst_object_unref (sonarsink->display);

  sonarsink->display = display;
  GST_OBJECT_UNLOCK (sonarsink);
}

static void
gst_sonarsink_set_context (GstElement * element, GstContext * context)
{
  GstSonarsink *sonarsink = GST_SONARSINK (element);
  GstGLContext *other_context = NULL;
  GstGLDisplay *display = NULL;
  GST_DEBUG_OBJECT(sonarsink, "got context");

  gst_gl_handle_set_context (element, context, &display, &other_context);
  gst_sonarsink_set_other_context (sonarsink, other_context);
  gst_sonarsink_set_display (sonarsink, display);

  if (sonarsink->display)
  {
    GST_DEBUG_OBJECT(sonarsink, "got display");
    gst_gl_display_filter_gl_api (sonarsink->display, SUPPORTED_GL_APIS);
  }

  GST_ELEMENT_CLASS (parent_class)->set_context (element, context);
}

static gboolean
gst_sonarsink_init_gl(GstSonarsink *sonarsink)
{
  const GstGLFuncs *gl = sonarsink->context->gl_vtable;
  GError *error = NULL;



  if (!(sonarsink->shader = gst_gl_shader_new_link_with_stages (sonarsink->context, &error,
    gst_glsl_stage_new_with_string (sonarsink->context,
      GL_VERTEX_SHADER, GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      gst_sonarsink_vertex_source),
    gst_glsl_stage_new_with_string (sonarsink->context,
      GL_FRAGMENT_SHADER, GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      gst_sonarsink_fragment_source),
    NULL)))
  {
    GST_ERROR_OBJECT (sonarsink, "Failed to link shader: %s", error->message);
    return FALSE;
  }

  GST_WARNING_OBJECT(sonarsink, "Ok shader");

  //gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, sonarsink->vbo_indices);
  //gl->BindBuffer (GL_ARRAY_BUFFER, sonarsink->vertex_buffer);

  ///* Load the vertex position */
  //gl->VertexAttribPointer (sonarsink->attr_position, 3, GL_FLOAT, GL_FALSE,
  //    5 * sizeof (GLfloat), (void *) 0);

  ///* Load the texture coordinate */
  //gl->VertexAttribPointer (sonarsink->attr_texture, 2, GL_FLOAT, GL_FALSE,
  //    5 * sizeof (GLfloat), (void *) (3 * sizeof (GLfloat)));

  //gl->EnableVertexAttribArray (sonarsink->attr_position);
  //gl->EnableVertexAttribArray (sonarsink->attr_texture);

  return TRUE;
}

static gboolean
gst_sonarsink_ensure_gl_setup (GstSonarsink * sonarsink)
{
  GError *error = NULL;

  GST_DEBUG_OBJECT (sonarsink, "Ensuring setup");

  if (!sonarsink->context) {
    GST_OBJECT_LOCK (sonarsink->display);
    do {
      GstGLContext *other_context = NULL;
      GstGLContext *context = NULL;
      GstGLWindow *window = NULL;

      gst_sonarsink_set_gl_context (sonarsink, NULL);

      GST_DEBUG_OBJECT (sonarsink,
          "No current context, creating one for %" GST_PTR_FORMAT,
          sonarsink->display);

      if (sonarsink->other_context) {
        other_context = gst_object_ref (sonarsink->other_context);
      } else {
        other_context =
            gst_gl_display_get_gl_context_for_thread (sonarsink->display, NULL);
      }

      if (!gst_gl_display_create_context (sonarsink->display,
              other_context, &context, &error)) {
        if (other_context)
          gst_object_unref (other_context);
        GST_OBJECT_UNLOCK (sonarsink->display);
        goto context_error;
      }
      gst_sonarsink_set_gl_context (sonarsink, context);
      context = NULL;

      GST_DEBUG_OBJECT (sonarsink,
          "created context %" GST_PTR_FORMAT " from other context %"
          GST_PTR_FORMAT, sonarsink->context, sonarsink->other_context);

      window = gst_gl_context_get_window (sonarsink->context);

      GST_DEBUG_OBJECT (sonarsink, "got window %" GST_PTR_FORMAT, window);

      if (!sonarsink->window_id && !sonarsink->new_window_id)
        gst_video_overlay_prepare_window_handle (GST_VIDEO_OVERLAY (sonarsink));

      GST_DEBUG_OBJECT (sonarsink,
          "window_id : %" G_GUINTPTR_FORMAT " , new_window_id : %"
          G_GUINTPTR_FORMAT, sonarsink->window_id, sonarsink->new_window_id);

      if (sonarsink->window_id != sonarsink->new_window_id) {
        sonarsink->window_id = sonarsink->new_window_id;
        GST_DEBUG_OBJECT (sonarsink, "Setting window handle on gl window");
        gst_gl_window_set_window_handle (window, sonarsink->window_id);
      }

      gst_gl_window_handle_events (window, sonarsink->handle_events);

      /* setup callbacks */
      //gst_gl_window_set_resize_callback (window,
      //    GST_GL_WINDOW_RESIZE_CB (gst_glimage_sink_on_resize),
      //    gst_object_ref (sonarsink), (GDestroyNotify) gst_object_unref);
      //gst_gl_window_set_draw_callback (window,
      //    GST_GL_WINDOW_CB (gst_glimage_sink_on_draw),
      //    gst_object_ref (sonarsink), (GDestroyNotify) gst_object_unref);
      //gst_gl_window_set_close_callback (window,
      //    GST_GL_WINDOW_CB (gst_glimage_sink_on_close),
      //    gst_object_ref (sonarsink), (GDestroyNotify) gst_object_unref);
      //sonarsink->key_sig_id = g_signal_connect (window, "key-event", G_CALLBACK
      //    (gst_glimage_sink_key_event_cb), sonarsink);
      //sonarsink->mouse_sig_id =
      //    g_signal_connect (window, "mouse-event",
      //    G_CALLBACK (gst_glimage_sink_mouse_event_cb), sonarsink);
      //sonarsink->mouse_scroll_sig_id =
      //    g_signal_connect (window, "scroll-event",
      //    G_CALLBACK (gst_glimage_sink_mouse_scroll_event_cb), sonarsink);

      //gst_gl_window_set_render_rectangle (window, sonarsink->x, sonarsink->y,
      //    sonarsink->width, sonarsink->height);

      if (other_context)
        gst_object_unref (other_context);
      gst_object_unref (window);
    } while (!gst_gl_display_add_context (sonarsink->display, sonarsink->context));

    GST_OBJECT_UNLOCK (sonarsink->display);
  } else
    GST_TRACE_OBJECT (sonarsink, "Already have a context");

  return TRUE;

context_error:
  {
    GST_ELEMENT_ERROR (sonarsink, RESOURCE, NOT_FOUND, ("%s", error->message),
        (NULL));

    gst_sonarsink_set_gl_context (sonarsink, NULL);

    g_clear_error (&error);

    return FALSE;
  }
}

static GstStateChangeReturn
gst_sonarsink_change_state (GstElement * element, GstStateChange transition)
{
  GstSonarsink *sonarsink;
  GstGLContext *context;

  GST_DEBUG ("changing state: %s => %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  sonarsink = GST_SONARSINK (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (!gst_gl_ensure_element_data (sonarsink, &sonarsink->display,
              &sonarsink->other_context))
        return GST_STATE_CHANGE_FAILURE;

      gst_gl_display_filter_gl_api (sonarsink->display, SUPPORTED_GL_APIS);

      if (!gst_sonarsink_ensure_gl_setup (sonarsink))
        return GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      g_atomic_int_set (&sonarsink->to_quit, 0);
      break;
    default:
      break;
    //gst_sonarsink_init_gl(sonarsink);
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
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

  gstelement_class->set_context = gst_sonarsink_set_context;
  gstelement_class->change_state = gst_sonarsink_change_state;

  gobject_class->finalize     = gst_sonarsink_finalize;
  gobject_class->set_property = gst_sonarsink_set_property;
  gobject_class->get_property = gst_sonarsink_get_property;

  basesink_class->render = GST_DEBUG_FUNCPTR (gst_sonarsink_render);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_sonarsink_set_caps);

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
  sonarsink->vertices = NULL;
  sonarsink->colors = NULL;

  sonarsink->display = NULL;
  sonarsink->context = NULL;
  sonarsink->other_context = NULL;

  sonarsink->window_id = 0;
  sonarsink->new_window_id = 0;

  sonarsink->to_quit = 0;
  sonarsink->handle_events = TRUE;

  //int rc = initWp();
  //assert(rc == 0);
}
