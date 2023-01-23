/**
 * SECTION:element-gst_nmeaparse
 *
 * Nmeaparse is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=nmeaparse:5,nmeaconvert:5 gst-launch-1.0 filesrc location=../in.nmea ! nmeaparse ! fakesink
 * </refsect2>
 */

#include "nmeaparse.h"
#include "sonarparse.h"

#include <stdio.h>

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(nmeaparse_debug);
#define GST_CAT_DEFAULT nmeaparse_debug

#define gst_nmeaparse_parent_class parent_class
G_DEFINE_TYPE (GstNmeaparse, gst_nmeaparse, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_nmeaparse_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS_ANY
  );

static GstStaticPadTemplate gst_nmeaparse_telemetry_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("application/telemetry")
  );

static GstFlowReturn
gst_nmeaparse_handle_frame (GstBaseParse * baseparse, GstBaseParseFrame * frame, gint * skipsize)
{
  GstNmeaparse *nmeaparse = GST_NMEAPARSE (baseparse);

  GstMapInfo mapinfo;
  if (!gst_buffer_map (frame->buffer, &mapinfo, GST_MAP_READ))
    return GST_FLOW_ERROR;

  #define exit(value) do {\
    gst_buffer_unmap (frame->buffer, &mapinfo); \
    return (value); \
  } while (0)

  GstByteReader reader;
  gst_byte_reader_init (&reader, mapinfo.data, mapinfo.size);

  *skipsize = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffff00, '$' << 24 | 'E' << 16 | 'I' << 8, 0, mapinfo.size);

  if (*skipsize == -1)
  {
    *skipsize = mapinfo.size;

    exit(GST_FLOW_OK);
  }
  else if (*skipsize != 0)
    exit(GST_FLOW_OK);

  guint32 nmea_size = gst_byte_reader_masked_scan_uint32 (&reader, 0xffff0000, '\r' << 24 | '\n' << 16, 0, mapinfo.size);

  if (nmea_size == -1)
  {
    gst_base_parse_set_min_frame_size (baseparse, mapinfo.size + 1);
    exit(GST_FLOW_OK);
  }

  GST_LOG_OBJECT(nmeaparse, "nmea entry of size %d: %.*s", nmea_size, nmea_size, mapinfo.data);

  GstSonarTelemetry* telemetry = g_malloc(sizeof(*telemetry));
  guint64 timestamp = 0;
  gdouble timeUTC;
  guint32 len;

  gdouble heading;

  gdouble longitude;
  gdouble latitude;
  gchar north;
  gchar east;

  gdouble roll;
  gdouble pitch;

  gdouble depth;
  gdouble altitude;

  if ((sscanf(mapinfo.data,"$EIHEA,%u,%lf,%lu,%lf*",&len,&timeUTC,&timestamp,&heading) == 4)
      && (heading != -1))
    *telemetry =
    (GstSonarTelemetry){
      .yaw = heading,
      .presence = GST_SONAR_TELEMETRY_PRESENCE_YAW,
    };
  else if ((sscanf(mapinfo.data,"$EIPOS,%u,%lf,%lu,%lf,%c,%lf,%c*",&len,&timeUTC,&timestamp,&latitude,&north,&longitude,&east) == 7)
      && (latitude != -1) && (longitude != -1))
    *telemetry =
    (GstSonarTelemetry){
      .latitude = latitude * (north == 'N' ? 1 : -1),
      .longitude = longitude * (east == 'E' ? 1 : -1),
      .presence = GST_SONAR_TELEMETRY_PRESENCE_LATITUDE | GST_SONAR_TELEMETRY_PRESENCE_LONGITUDE,
    };
  else if ((sscanf(mapinfo.data,"$EIORI,%u,%lf,%lu,%lf,%lf*",&len,&timeUTC,&timestamp,&roll,&pitch) == 5)
      && (roll != -1) && (pitch != -1))
    *telemetry =
    (GstSonarTelemetry){
      .roll = roll,
      .pitch = pitch,
      .presence = GST_SONAR_TELEMETRY_PRESENCE_ROLL | GST_SONAR_TELEMETRY_PRESENCE_PITCH,
    };
  else if ((sscanf(mapinfo.data,"$EIDEP,%u,%lf,%lu,%lf,m,%lf,m*",&len,&timeUTC,&timestamp,&depth,&altitude) == 5)
      && (depth != -1) && (altitude != -1))
    *telemetry =
    (GstSonarTelemetry){
      .depth = depth,
      .altitude = altitude,
      .presence = GST_SONAR_TELEMETRY_PRESENCE_ALTITUDE | GST_SONAR_TELEMETRY_PRESENCE_DEPTH,
    };
  else
  {
    g_free(telemetry);
    telemetry = NULL;
  }


  timestamp *= (guint64)1e6; // ms to ns
  if (telemetry != NULL)
  {
    if (nmeaparse->initial_time == 0)
    {
      // set initial time
      g_mutex_lock(&gst_sonar_shared_data.m);
      if (gst_sonar_shared_data.initial_time == 0)
      {
        GST_DEBUG_OBJECT(nmeaparse, "setting global initial time from %llu", timestamp);
        nmeaparse->initial_time = gst_sonar_shared_data.initial_time = timestamp;
      }
      else if (gst_sonar_shared_data.initial_time * 10 > timestamp)
      {
        GST_WARNING_OBJECT(nmeaparse, "global initial time is too large: %llu * 10 > %llu, starting from zero", gst_sonar_shared_data.initial_time, timestamp);
        nmeaparse->initial_time = timestamp;
      }
      else if (gst_sonar_shared_data.initial_time < timestamp * 10)
      {
        GST_WARNING_OBJECT(nmeaparse, "global initial time is too small: %llu < %llu * 10, starting from zero", gst_sonar_shared_data.initial_time, timestamp);
        nmeaparse->initial_time = timestamp;
      }
      else
      {
        GST_DEBUG_OBJECT(nmeaparse, "using global initial time %llu", gst_sonar_shared_data.initial_time);
        nmeaparse->initial_time = gst_sonar_shared_data.initial_time;
      }
      g_mutex_unlock(&gst_sonar_shared_data.m);

      // set constant caps
      GstCaps *caps = gst_caps_new_simple ("application/telemetry", NULL);
      GST_DEBUG_OBJECT (nmeaparse, "setting downstream caps on %s:%s to %" GST_PTR_FORMAT,
        GST_DEBUG_PAD_NAME (GST_BASE_PARSE_SRC_PAD (nmeaparse)), caps);

      if (!gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (baseparse), caps))
      {
        GST_ERROR_OBJECT(nmeaparse, "couldn't set caps");
        gst_caps_unref (caps);
        return FALSE;
      }
      gst_caps_unref (caps);

    }

    frame->out_buffer = gst_buffer_new_wrapped (telemetry, sizeof(*telemetry));

    // set pts
    if (timestamp < nmeaparse->initial_time)
    {
      GST_WARNING_OBJECT(nmeaparse, "timestamp would be negative: %llu < %llu, reset to zero", timestamp, nmeaparse->initial_time);
      GST_BUFFER_PTS (frame->out_buffer) = GST_BUFFER_DTS (frame->out_buffer) = 0;
    }
    else
      GST_BUFFER_PTS (frame->out_buffer) = GST_BUFFER_DTS (frame->out_buffer) = timestamp - nmeaparse->initial_time;

    GST_TRACE_OBJECT (nmeaparse, "created telemetry buffer %p with timestamp: %llu, pts: %llu", frame->out_buffer, timestamp, GST_BUFFER_PTS (frame->out_buffer));

    exit(gst_base_parse_finish_frame (baseparse, frame, nmea_size + 2));
  }
  else
    exit(GST_FLOW_OK);

  #undef exit
}

static gboolean
gst_nmeaparse_start (GstBaseParse * baseparse)
{
  GstNmeaparse *nmeaparse = GST_NMEAPARSE (baseparse);

  GST_DEBUG_OBJECT (nmeaparse, "start");

  return TRUE;
}

static void
gst_nmeaparse_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstNmeaparse *nmeaparse = GST_NMEAPARSE (object);

  GST_OBJECT_LOCK (nmeaparse);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (nmeaparse);
}

static void
gst_nmeaparse_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstNmeaparse *nmeaparse = GST_NMEAPARSE (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_nmeaparse_finalize (GObject * object)
{
  GstNmeaparse *nmeaparse = GST_NMEAPARSE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_nmeaparse_class_init (GstNmeaparseClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseParseClass *baseparse_class = (GstBaseParseClass *) klass;

  gobject_class->finalize     = gst_nmeaparse_finalize;
  gobject_class->set_property = gst_nmeaparse_set_property;
  gobject_class->get_property = gst_nmeaparse_get_property;

  GST_DEBUG_CATEGORY_INIT(nmeaparse_debug, "nmeaparse", 0, "TODO");


  gst_element_class_set_static_metadata (gstelement_class, "Nmeaparse",
      "Transform",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_nmeaparse_telemetry_src_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_nmeaparse_sink_template);

  baseparse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_nmeaparse_handle_frame);
  baseparse_class->start = GST_DEBUG_FUNCPTR (gst_nmeaparse_start);
}

static void
gst_nmeaparse_init (GstNmeaparse * nmeaparse)
{
  nmeaparse->initial_time = 0;
}
