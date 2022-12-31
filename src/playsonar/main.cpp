#include <gst/gst.h>
#include <signal.h>
#include <assert.h>

#include <string>

gboolean cb_bus_message(GstBus * bus, GstMessage * message, gpointer p_pipeline)
{
  GstElement *pipeline = (GstElement*) p_pipeline;

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream received\n");
      gst_element_set_state (pipeline, GST_STATE_NULL);
      gst_object_unref (pipeline);
      exit(0);
      break;
    default:
      break;
  }

  return TRUE;
}

int main(int argc, char** argv)
{
    gst_init(NULL, NULL);


    std::string launch = "";
    for (int i=1; i<argc; ++i) //skip program name
      launch += std::string(" ") + argv[i];

    g_print("launch: %s\n", launch.c_str());

    assert(launch != "");

    GstElement *pipeline = gst_parse_launch(launch.c_str(), NULL);

    // attach message callback on bus for eos
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(cb_bus_message), pipeline);
    gst_object_unref(GST_OBJECT(bus));

    // try to play pipeline
    GstStateChangeReturn rc = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("playing: %d\n", rc);
    assert(rc != GST_STATE_CHANGE_FAILURE);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
}
