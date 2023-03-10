#include "sonarparse.h"
#include "sonarconvert.h"
#include "sonarsink.h"
#include "sonarmux.h"
#include "sonardetect.h"
#include "nmeaparse.h"
#include "eelnmeadec.h"
#include "sbdmux.h"


static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "sonarparse", GST_RANK_NONE, gst_sonarparse_get_type())
  || !gst_element_register (plugin, "sonarconvert", GST_RANK_NONE, gst_sonarconvert_get_type())
  || !gst_element_register (plugin, "sonarsink", GST_RANK_NONE, gst_sonarsink_get_type())
  || !gst_element_register (plugin, "sonarmux", GST_RANK_NONE, gst_sonarmux_get_type())
  || !gst_element_register (plugin, "sonardetect", GST_RANK_NONE, gst_sonardetect_get_type())
  || !gst_element_register (plugin, "nmeaparse", GST_RANK_NONE, gst_nmeaparse_get_type())
  || !gst_element_register (plugin, "eelnmeadec", GST_RANK_NONE, gst_eelnmeadec_get_type())
  || !gst_element_register (plugin, "sbdmux", GST_RANK_NONE, gst_sbdmux_get_type()))
    return FALSE;
  return TRUE;
}

#define VERSION "1.0"
#define PACKAGE "gstsonar"
#define GST_PACKAGE_NAME PACKAGE
#define GST_PACKAGE_ORIGIN "Norway"


GST_PLUGIN_DEFINE (
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  sonar,
  "sonar processing plugins",
  plugin_init,
  VERSION,
  "LGPL",
  GST_PACKAGE_NAME,
  GST_PACKAGE_ORIGIN);
