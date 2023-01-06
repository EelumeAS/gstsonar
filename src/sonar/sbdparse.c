/**
 * SECTION:element-gst_sbdparse
 *
 * Sbdparse is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  GST_PLUGIN_PATH=$(pwd) GST_DEBUG=sbdparse:5,sbdconvert:5 gst-launch-1.0 filesrc location=../in.sbd ! sbdparse ! fakesink
 * </refsect2>
 */

#include "sbdparse.h"
#include "navi.h"

#include <stdio.h>
#include <sys/time.h>

#include <gst/base/gstbytereader.h>

GST_DEBUG_CATEGORY_STATIC(sbdparse_debug);
#define GST_CAT_DEFAULT sbdparse_debug

#define gst_sbdparse_parent_class parent_class
G_DEFINE_TYPE (GstSbdparse, gst_sbdparse, GST_TYPE_BASE_PARSE);

static GstStaticPadTemplate gst_sbdparse_src_template =
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

static GstStaticPadTemplate gst_sbdparse_sonar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("sonar/multibeam")
  );

static GstStaticPadTemplate gst_sbdparse_telemetry_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("application/telemetry")
  );

static GstFlowReturn
gst_sbdparse_handle_frame (GstBaseParse * baseparse, GstBaseParseFrame * frame, gint * skipsize)
{
  GstSbdparse *sbdparse = GST_SBDPARSE (baseparse);

  GstMapInfo mapinfo;
  if (!gst_buffer_map (frame->buffer, &mapinfo, GST_MAP_READ))
    return GST_FLOW_ERROR;

  #define exit(value) do {\
    gst_buffer_unmap (frame->buffer, &mapinfo); \
    return value; \
  } while (0)

  const sbd_entry_header_t *header = (sbd_entry_header_t*)mapinfo.data;

  if (mapinfo.size < sizeof(*header))
    exit(GST_FLOW_OK);

  const uint32_t entry_size = header->entry_size;

  const uint32_t total_size = sizeof(*header) + entry_size;
  if (mapinfo.size < total_size)
  {
    gst_base_parse_set_min_frame_size (baseparse, total_size);
    exit(GST_FLOW_OK);
  }

  GST_LOG_OBJECT(sbdparse, "entry with size: %llu %llu", sizeof(*header), total_size);

  // set time
  guint64 time = (guint64)header->absolute_time.tv_sec * (guint64)1e9 + (guint64)header->absolute_time.tv_usec * (guint64)1e3;
  if (sbdparse->initial_time == 0)
  {
    sbdparse->initial_time = time;

    // set constant caps as well
    GstCaps *caps = gst_caps_new_simple ("sonar/multibeam", "parsed", G_TYPE_BOOLEAN, FALSE, NULL);
    GST_DEBUG_OBJECT (sbdparse, "setting downstream caps on %s:%s to %" GST_PTR_FORMAT,
      GST_DEBUG_PAD_NAME (GST_BASE_PARSE_SRC_PAD (sbdparse)), caps);

    if (!gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (baseparse), caps))
    {
      GST_ERROR_OBJECT(sbdparse, "couldn't set caps");
      gst_caps_unref (caps);
      return FALSE;
    }
    gst_caps_unref (caps);

  }

  GST_BUFFER_PTS (frame->buffer) = GST_BUFFER_DTS (frame->buffer) = time - sbdparse->initial_time;
  //GST_BUFFER_DURATION (frame->buffer) = (guint64)(1e9/sub_header->ping_rate);

  GST_LOG_OBJECT(sbdparse, "time: %d %llu", header->absolute_time.tv_sec, GST_BUFFER_PTS (frame->buffer));

  switch (header->entry_type)
  {
      case WBMS_BATH:
        GST_ERROR_OBJECT (sbdparse, "bathymetry format not implemented");
        exit(GST_FLOW_ERROR);

      case WBMS_FLS:
        GST_DEBUG_OBJECT(sbdparse, "Got fls");
        //*skipsize = sizeof(*header);
        gst_buffer_unmap (frame->buffer, &mapinfo);
        return gst_base_parse_finish_frame (baseparse, frame, total_size);

      // the size of the nmea string (with null terminator) is the integer before the string:
      // uint32_t size = ((uint32_t*)nmea) - 1;
      case NMEA_EIHEA:
      {
        // EIHEA
        const char* nmea = mapinfo.data + sizeof(*header) + 20;
        GST_DEBUG_OBJECT(sbdparse, "EIHEA nmea: %s", nmea);

        uint32_t len;
        uint64_t timestamp;
        double timeUTC;
        double heading;
        int sscanf_ret = sscanf(nmea,"$EIHEA,%u,%lf,%lu,%lf*",&len,&timeUTC,&timestamp,&heading);

        if (sscanf_ret < 4)
        {
          GST_WARNING_OBJECT (sbdparse, "could only parse %d fields in EIHEA entry", sscanf_ret);
          exit(GST_FLOW_EOS);
        }

        *skipsize = total_size;
        exit(GST_FLOW_OK);
      }
      case NMEA_EIPOS:
      {
        // EIPOS
        const char* nmea = mapinfo.data + sizeof(*header) + 20;
        GST_DEBUG_OBJECT(sbdparse, "EIPOS nmea: %s", nmea);

        uint32_t len;
        uint64_t timestamp;
        double timeUTC;
        double longitude;
        double latitude;
        char north;
        char east;
        int sscanf_ret = sscanf(nmea,"$EIPOS,%u,%lf,%lu,%lf,%c,%lf,%c*",&len,&timeUTC,&timestamp,&latitude,&north, &longitude,&east);

        if (sscanf_ret < 7)
        {
          GST_WARNING_OBJECT (sbdparse, "could only parse %d fields in EIPOS entry", sscanf_ret);
          exit(GST_FLOW_EOS);
        }

        *skipsize = total_size;
        exit(GST_FLOW_OK);
      }
      case NMEA_EIORI:
      {
        // EIORI
        const char* nmea = mapinfo.data + sizeof(*header) + 16;
        GST_DEBUG_OBJECT(sbdparse, "EIORI nmea: %s", nmea);

        uint32_t len;
        uint64_t timestamp;
        double timeUTC;
        double roll;
        double pitch;
        int sscanf_ret = sscanf(nmea,"$EIORI,%u,%lf,%lu,%lf,%lf*",&len,&timeUTC,&timestamp,&roll,&pitch);

        if (sscanf_ret < 5)
        {
          GST_WARNING_OBJECT (sbdparse, "could only parse %d fields in EIORI entry", sscanf_ret);
          exit(GST_FLOW_EOS);
        }

        *skipsize = total_size;
        exit(GST_FLOW_OK);
      }
      case NMEA_EIDEP:
      {
        // EIDEP
        const char* nmea = mapinfo.data + sizeof(*header) + 16;
        GST_DEBUG_OBJECT(sbdparse, "EIDEP nmea: %s", nmea);

        uint32_t len;
        uint64_t timestamp;
        double timeUTC;
        double depth;
        double altitude;
        int sscanf_ret = sscanf(nmea,"$EIDEP,%u,%lf,%lu,%lf,m,%lf,m*",&len,&timeUTC,&timestamp,&depth,&altitude);
        if (sscanf_ret < 5)
        {
          GST_WARNING_OBJECT (sbdparse, "could only parse %d fields in EIDEP entry", sscanf_ret);
          exit(GST_FLOW_EOS);
        }

        *skipsize = total_size;
        exit(GST_FLOW_OK);
      }
      case SBD_HEADER:
        GST_WARNING_OBJECT (sbdparse, "ignoring SBD_HEADER entry");
        *skipsize = total_size;
        exit(GST_FLOW_OK);
        break;
      default:
        GST_WARNING_OBJECT(sbdparse, "unrecognized entry_type: 0x%02x", header->entry_type);
        *skipsize = total_size;
        exit(GST_FLOW_OK);
  }

  #undef exit
}

static gboolean
gst_sbdparse_start (GstBaseParse * baseparse)
{
  GstSbdparse *sbdparse = GST_SBDPARSE (baseparse);

  GST_DEBUG_OBJECT (sbdparse, "start");

  gst_base_parse_set_min_frame_size (baseparse, sizeof(sbd_entry_header_t));

  return TRUE;
}

static void
gst_sbdparse_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSbdparse *sbdparse = GST_SBDPARSE (object);

  GST_OBJECT_LOCK (sbdparse);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sbdparse);
}

static void
gst_sbdparse_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSbdparse *sbdparse = GST_SBDPARSE (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sbdparse_finalize (GObject * object)
{
  GstSbdparse *sbdparse = GST_SBDPARSE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sbdparse_class_init (GstSbdparseClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseParseClass *baseparse_class = (GstBaseParseClass *) klass;

  gobject_class->finalize     = gst_sbdparse_finalize;
  gobject_class->set_property = gst_sbdparse_set_property;
  gobject_class->get_property = gst_sbdparse_get_property;

  GST_DEBUG_CATEGORY_INIT(sbdparse_debug, "sbdparse", 0, "TODO");


  gst_element_class_set_static_metadata (gstelement_class, "Sbdparse",
      "Transform",
      "TODO", // TODO
      "Erlend Eriksen <erlend.eriksen@eelume.com>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sbdparse_sonar_sink_template);
  //gst_element_class_add_static_pad_template (gstelement_class, &gst_sbdparse_telemetry_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_sbdparse_src_template);

  baseparse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_sbdparse_handle_frame);
  baseparse_class->start = GST_DEBUG_FUNCPTR (gst_sbdparse_start);
}

static void
gst_sbdparse_init (GstSbdparse * sbdparse)
{
}
