#include <gst/gst.h>

#include "sonarshared.h"

GstSonarSharedData gst_sonar_shared_data;


void gst_sonarshared_init()
{
  g_mutex_init(&gst_sonar_shared_data.m);
  gst_sonar_shared_data.initial_time = 0;
}

void gst_sonarshared_finalize()
{
  g_mutex_clear(&gst_sonar_shared_data.m);
}

guint64 gst_sonarshared_set_initial_time(guint64 timestamp)
{
  guint64 ret;
  g_mutex_lock(&gst_sonar_shared_data.m);

  if (gst_sonar_shared_data.initial_time == 0)
  {
    GST_WARNING("setting global initial time from %llu", timestamp);

    ret = gst_sonar_shared_data.initial_time = timestamp;
  }
  else if (gst_sonar_shared_data.initial_time * 10 > timestamp)
  {
    GST_WARNING("global initial time is too large: %llu * 10 > %llu, starting from zero", gst_sonar_shared_data.initial_time, timestamp);

    ret = timestamp;
  }
  else if (gst_sonar_shared_data.initial_time < timestamp * 10)
  {
    GST_WARNING("global initial time is too small: %llu < %llu * 10, starting from zero", gst_sonar_shared_data.initial_time, timestamp);

    ret = timestamp;
  }
  else
  {
    GST_WARNING("using global initial time %llu", gst_sonar_shared_data.initial_time);

    ret = gst_sonar_shared_data.initial_time;
  }
  g_mutex_unlock(&gst_sonar_shared_data.m);

  return ret;
}

guint64 gst_sonarshared_get_initial_time()
{
  guint64 ret;
  g_mutex_lock(&gst_sonar_shared_data.m);

  ret = gst_sonar_shared_data.initial_time;

  g_mutex_unlock(&gst_sonar_shared_data.m);

  return ret;
}
