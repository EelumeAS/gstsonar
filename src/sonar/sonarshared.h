// global object for synchronization during initialization
// FIXME: This limits the scalability of the sonar elements

#include <glib.h>

typedef struct
{
  GMutex m;
  guint64 initial_time;
} GstSonarSharedData;

extern GstSonarSharedData gst_sonar_shared_data; 

void gst_sonarshared_init();

void gst_sonarshared_finalize();

guint64 gst_sonarshared_set_initial_time(guint64 timestamp);

guint64 gst_sonarshared_get_initial_time();
