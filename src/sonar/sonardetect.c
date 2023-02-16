/**
 * SECTION:element-gst_sonardetect
 *
 * Sonardetect is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=sonardetect:5,sonarconvert:5 gst-launch-1.0 filesrc location=../in.sbd ! sonardetect ! fakesink
 * </refsect2>
 */

#include "sonardetect.h"

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(sonardetect_debug);
#define GST_CAT_DEFAULT sonardetect_debug

#define gst_sonardetect_parent_class parent_class
G_DEFINE_TYPE (GstSonardetect, gst_sonardetect, GST_TYPE_BASE_TRANSFORM);

static GstStaticPadTemplate gst_sonardetect_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "sonar/multibeam, "
        "n_beams = (int) [ 0, MAX ],"
        "resolution = (int) [ 0, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ], "
        "parsed = (boolean) true ;"

        "sonar/bathymetry,"
        "n_beams = (int) [ 0, MAX ],"
        "resolution = (int) 1,"
        "framerate = (fraction) [ 0/1, MAX ], "
        "parsed = (boolean) true ;"
        )
    );

static GstStaticPadTemplate gst_sonardetect_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("sonar/multibeam; sonar/bathymetry")
  );

static GstFlowReturn
gst_sonardetect_transform_ip (GstBaseTransform * basetransform, GstBuffer * buf)
{
  GstSonardetect *sonardetect = GST_SONARDETECT (basetransform);

  GST_OBJECT_LOCK (sonardetect);

  const GstSonarMetaData *meta_data = &GST_SONAR_META_GET(buf)->data;
  const GstSonarTelemetry *tel = &GST_TELEMETRY_META_GET(buf)->tel;

  if (sonardetect->has_telemetry)
    GST_DEBUG_OBJECT(sonardetect, "%lu:\tn_beams = %d, resolution = %d, sound_speed = %f, sample_rate = %f, t0 = %d, gain = %f"
      ", pitch=%f, roll=%f, yaw=%f, latitude=%f, longitude=%f, depth=%f, altitude=%f, presence: %#02x"
      , buf->pts, sonardetect->n_beams, sonardetect->resolution, meta_data->sound_speed, meta_data->sample_rate, meta_data->t0, meta_data->gain
      , tel->pitch, tel->roll, tel->yaw, tel->latitude, tel->longitude, tel->depth, tel->altitude, tel->presence);
  else
    GST_DEBUG_OBJECT(sonardetect, "%lu:\tn_beams = %d, resolution = %d, sound_speed = %f, sample_rate = %f, t0 = %d, gain = %f"
      , buf->pts, sonardetect->n_beams, sonardetect->resolution, meta_data->sound_speed, meta_data->sample_rate, meta_data->t0, meta_data->gain);

  GstMapInfo mapinfo;
  if (!gst_buffer_map (buf, &mapinfo, GST_MAP_READ | GST_MAP_WRITE))
  {
    GST_WARNING_OBJECT(sonardetect, "couldn't map sonar buffer at %p", buf);
  }
  else
  {
    switch(sonardetect->wbms_type)
    {
      case WBMS_FLS:
        if (sonardetect->has_telemetry)
          sonardetect_detect(mapinfo.data, sonardetect->n_beams, sonardetect->resolution, meta_data, tel);
        break;
      default:
      case WBMS_BATH:
        break;
    }

    gst_buffer_unmap(buf, &mapinfo);
  }

  GST_OBJECT_UNLOCK (sonardetect);

  return GST_FLOW_OK;
}

static gboolean
gst_sonardetect_set_caps (GstBaseTransform * basetransform, GstCaps * incaps, GstCaps * outcaps)
{
  GstSonardetect *sonardetect = GST_SONARDETECT (basetransform);

  GstStructure *s = gst_caps_get_structure (incaps, 0);

  GST_DEBUG_OBJECT(sonardetect, "caps structure: %s\n", gst_structure_to_string(s));

  gint n_beams, resolution;
  gboolean has_telemetry;
  const gchar *caps_name;

  if ((caps_name = gst_structure_get_name(s))
    && gst_structure_get_int(s, "n_beams", &n_beams)
    && gst_structure_get_int(s, "resolution", &resolution)
    && gst_structure_get_boolean(s, "has_telemetry", &has_telemetry))
  {
    GST_OBJECT_LOCK (sonardetect);

    GST_DEBUG_OBJECT(sonardetect, "got caps details caps_name: %s, n_beams: %d, resolution: %d, has_telemetry: %d", caps_name, n_beams, resolution, has_telemetry);
    sonardetect->n_beams = (guint32)n_beams;
    sonardetect->resolution = (guint32)resolution;
    sonardetect->has_telemetry = has_telemetry;

    if (strcmp(caps_name, "sonar/multibeam") == 0)
      sonardetect->wbms_type = WBMS_FLS;
    else if (strcmp(caps_name, "sonar/bathymetry") == 0)
      sonardetect->wbms_type = WBMS_BATH;
    else
      g_assert_not_reached();

    GST_OBJECT_UNLOCK (sonardetect);

    return TRUE;
  }
  else
  {
    GST_DEBUG_OBJECT(sonardetect, "no details in caps\n");

    return FALSE;
  }
}

static void
gst_sonardetect_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonardetect *sonardetect = GST_SONARDETECT (object);

  GST_OBJECT_LOCK (sonardetect);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonardetect);
}

static void
gst_sonardetect_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSonardetect *sonardetect = GST_SONARDETECT (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonardetect_finalize (GObject * object)
{
  GstSonardetect *sonardetect = GST_SONARDETECT (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonardetect_class_init (GstSonardetectClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;

  gobject_class->finalize     = gst_sonardetect_finalize;
  gobject_class->set_property = gst_sonardetect_set_property;
  gobject_class->get_property = gst_sonardetect_get_property;

  GST_DEBUG_CATEGORY_INIT(sonardetect_debug, "sonardetect", 0, "TODO");


  gst_element_class_set_static_metadata (gstelement_class, "Sonardetect",
      "Transform",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonardetect_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonardetect_src_template);

  basetransform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_sonardetect_transform_ip);
  basetransform_class->set_caps = GST_DEBUG_FUNCPTR (gst_sonardetect_set_caps);
}

static void
gst_sonardetect_init (GstSonardetect * sonardetect)
{
  sonardetect->n_beams = 0;
  sonardetect->resolution = 0;
  sonardetect->has_telemetry = FALSE;
}
