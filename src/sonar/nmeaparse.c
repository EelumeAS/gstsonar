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

#include <stdio.h>
#include <sys/time.h>

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(nmeaparse_debug);
#define GST_CAT_DEFAULT nmeaparse_debug

#define gst_nmeaparse_parent_class parent_class
G_DEFINE_TYPE (GstNmeaparse, gst_nmeaparse, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_nmeaparse_sonar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("sonar/multibeam")
  );

static GstStaticPadTemplate gst_nmeaparse_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("sonar/multibeam, "
        //"format = (string) { norbit }, "
        "n_beams = (int) [ 0, MAX ],"
        "resolution = (int) [ 0, MAX ], "
        "framerate = (fraction) [ 0/1, MAX ], "
        "parsed = (boolean) true"
        )
    );

static GstStaticPadTemplate gst_nmeaparse_telemetry_src_template =
GST_STATIC_PAD_TEMPLATE ("tel",
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

  GstNmeaparseTelemetry* telemetry = g_malloc(sizeof(*telemetry));
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

  if (sscanf(mapinfo.data,"$EIHEA,%u,%lf,%lu,%lf*",&len,&timeUTC,&timestamp,&heading) == 4)
    *telemetry =
    (GstNmeaparseTelemetry){
      .yaw = heading,
    };
  else if (sscanf(mapinfo.data,"$EIPOS,%u,%lf,%lu,%lf,%c,%lf,%c*",&len,&timeUTC,&timestamp,&latitude,&north, &longitude,&east) == 7)
    *telemetry =
    (GstNmeaparseTelemetry){
      .latitude = latitude,
      .longitude = longitude,
    };
  else if (sscanf(mapinfo.data,"$EIORI,%u,%lf,%lu,%lf,%lf*",&len,&timeUTC,&timestamp,&roll,&pitch) == 5)
    *telemetry =
    (GstNmeaparseTelemetry){
      .pitch = pitch,
      .roll = roll,
    };
  else if (sscanf(mapinfo.data,"$EIDEP,%u,%lf,%lu,%lf,m,%lf,m*",&len,&timeUTC,&timestamp,&depth,&altitude) == 5)
    *telemetry =
    (GstNmeaparseTelemetry){
      .depth = depth,
      .altitude = altitude,
    };
  else
  {
    g_free(telemetry);
    telemetry = NULL;
  }


  timestamp *= (guint64)1e6; // ms to ns
  if (telemetry != NULL)
  {
    // set time
    if (nmeaparse->initial_time == 0)
    {
      nmeaparse->initial_time = timestamp;

      // set constant caps as well
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
    GST_BUFFER_PTS (frame->out_buffer) = timestamp - nmeaparse->initial_time;
    GST_LOG_OBJECT (nmeaparse, "created telemetry buffer %p with timestamp: %llu, pts: %llu", frame->out_buffer, timestamp, GST_BUFFER_PTS (frame->out_buffer));

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

  gst_element_class_add_static_pad_template (gstelement_class, &gst_nmeaparse_sonar_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_nmeaparse_src_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_nmeaparse_telemetry_src_template);

  baseparse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_nmeaparse_handle_frame);
  baseparse_class->start = GST_DEBUG_FUNCPTR (gst_nmeaparse_start);
}

static void
gst_nmeaparse_init (GstNmeaparse * nmeaparse)
{
  nmeaparse->initial_time = 0;
}