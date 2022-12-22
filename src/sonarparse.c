/**
 * SECTION:element-gst_sonarparse
 *
 * Sonarparse is a TODO
 *
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
  gst-launch-1.0 -v -e fileparse location=test.sbd ! sonarparse ! TODO
 * </refsect2>
 */

#define VERSION "1.0"
#define PACKAGE "gst_sonarparse"
#define GST_PACKAGE_NAME PACKAGE
#define GST_PACKAGE_ORIGIN "Norway"

#include "sonarparse.h"
#include "navi.h"

#include <stdio.h>
#include <sys/time.h>

#include <gst/base/gstbytereader.h>

#define NORBIT_SONAR_PREFIX 0xefbeadde // deadbeef

inline double ms()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);

  double ms = (double)(tp.tv_sec)*1000 + (double)(tp.tv_usec)/1000;
  return ms;
}

GST_DEBUG_CATEGORY_STATIC(sonarparse_debug);
#define GST_CAT_DEFAULT sonarparse_debug

#define gst_sonarparse_parent_class parent_class
G_DEFINE_TYPE (GstSonarparse, gst_sonarparse, GST_TYPE_BASE_PARSE);

#define CAPS_STR "image/jpeg"

static GstStaticPadTemplate gst_sonarparse_parse_template =
GST_STATIC_PAD_TEMPLATE ("src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (CAPS_STR)
  );

static GstStaticPadTemplate gst_sonarparse_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS (CAPS_STR)
  );

static GstFlowReturn
gst_sonarparse_handle_frame (GstBaseParse * baseparse, GstBaseParseFrame * frame, gint * skipsize)
{
  GstSonarparse *sonarparse = GST_SONARPARSE (baseparse);
  GST_DEBUG_OBJECT(sonarparse, "handle_frame");

  // profile time:
  //static double start = 0;
  //GST_WARNING_OBJECT(sonarparse, "%f", ms() - start);
  //start = ms();

  GstClockTime timestamp, duration;
  GstMapInfo mapinfo;
  gboolean header_ok;

  timestamp = GST_BUFFER_PTS (frame->buffer);
  duration = GST_BUFFER_DURATION (frame->buffer);

  if (!gst_buffer_map (frame->buffer, &mapinfo, GST_MAP_READ))
  {
    return GST_FLOW_ERROR;
  }

  #define exit do {\
    gst_buffer_unmap (frame->buffer, &mapinfo); \
    return GST_FLOW_OK; \
  } while (0)

  if (mapinfo.size < 4)
    exit;

  GstByteReader reader;
  gst_byte_reader_init (&reader, mapinfo.data, mapinfo.size);

  *skipsize = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffffff,
      NORBIT_SONAR_PREFIX, 0, mapinfo.size);
  if (*skipsize == -1) {
    *skipsize = mapinfo.size - 3;      /* Last 3 bytes + 1 more may match header. */

    exit;
  }

  GST_DEBUG_OBJECT(sonarparse, "found preamble at %d\n", *skipsize);

  // skip over preamble
  gst_byte_reader_skip (&reader, *skipsize);

  //// read sonar type
  //guint32 sonar_type;
  //if (!gst_byte_reader_get_uint32_le (&reader, &sonar_type))
  //  exit;

  //if (gst_byte_reader_get_remaining (&reader) < sizeof(packet_header_t))
  //  exit;

  const guint8 *header_data = NULL;
  const packet_header_t* header = NULL;
  if (!gst_byte_reader_get_data (&reader, sizeof(*header), &header_data))
    exit;
  header = (const packet_header_t*)header_data;

  GST_DEBUG_OBJECT(sonarparse, "sonar type: %d\n", header->type);

  if (header->type != 2)
  {
      GST_ERROR_OBJECT(sonarparse, "sonar type not implemented: %d\n", header->type);
      gst_buffer_unmap (frame->buffer, &mapinfo);
      return GST_FLOW_ERROR;
  }

  if (header->size < sizeof(packet_header_t))
  {
      GST_ERROR_OBJECT(sonarparse, "size specified in header is too small: %d\n", header->size);
      gst_buffer_unmap (frame->buffer, &mapinfo);
      return GST_FLOW_ERROR;
  }

  gst_base_parse_set_min_frame_size (baseparse, header->size);

  const guint8 *sub_header_data = NULL;
  const fls_data_header_t* sub_header = NULL;
  if (!gst_byte_reader_get_data (&reader, sizeof(*sub_header), &sub_header_data))
    exit;
  sub_header = (const fls_data_header_t* )sub_header_data;

  sonarparse->n_beams = sub_header->N;
  sonarparse->resolution = sub_header->M;
  sonarparse->framerate = sub_header->ping_rate;

  GST_DEBUG_OBJECT(sonarparse, "n_beams: %d, resolution: %d, framerate: %d", sonarparse->n_beams, sonarparse->resolution, sonarparse->framerate);

  //GstCaps *caps = gst_caps_new_simple ("sonar/multibeam", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  //if (!gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (baseparse), caps))

  #undef exit
  return gst_base_parse_finish_frame (baseparse, frame, gst_byte_reader_get_pos (&reader));
}

static gboolean
gst_sonarparse_start (GstBaseParse * baseparse)
{
  GstSonarparse *sonarparse = GST_SONARPARSE (baseparse);

  GST_DEBUG_OBJECT (sonarparse, "start");

  gst_base_parse_set_min_frame_size (baseparse, sizeof(packet_header_t));

  sonarparse->n_beams = 0;
  sonarparse->resolution = 0;

  return TRUE;
}

static void
gst_sonarparse_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSonarparse *sonarparse = GST_SONARPARSE (object);

  GST_OBJECT_LOCK (sonarparse);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (sonarparse);
}

static void
gst_sonarparse_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSonarparse *sonarparse = GST_SONARPARSE (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonarparse_finalize (GObject * object)
{
  GstSonarparse *sonarparse = GST_SONARPARSE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_sonarparse_class_init (GstSonarparseClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseParseClass *baseparse_class = (GstBaseParseClass *) klass;
  GstBaseParseClass *base_class = (GstBaseParseClass *) klass;

  gobject_class->finalize     = gst_sonarparse_finalize;
  gobject_class->set_property = gst_sonarparse_set_property;
  gobject_class->get_property = gst_sonarparse_get_property;

  GST_DEBUG_CATEGORY_INIT(sonarparse_debug, "sonarparse", 0, "Fix for double buffer bug in sonix c1/c1-pro cameras");


  gst_element_class_set_static_metadata (gstelement_class, "Sonarparse",
      "Transform",
      "Fix for double buffer bug in sonix c1/c1-pro cameras",
      "Erlend Eriksen <erlend.eriksen@blueye.no>");

  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarparse_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &gst_sonarparse_parse_template);

  baseparse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_sonarparse_handle_frame);
  baseparse_class->start = GST_DEBUG_FUNCPTR (gst_sonarparse_start);
}

static void
gst_sonarparse_init (GstSonarparse * sonarparse)
{
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "sonarparse", GST_RANK_NONE, gst_sonarparse_get_type()))
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    sonarparse,
          "Parse sonar data",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
