#include <gst/gst.h>

static GMainLoop *loop;

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *rtspsrc;
  GstElement *queue0, *queue1;
  GstElement *capsfilter0, *capsfilter1;
  GstElement *rtph264depay0, *rtph264depay1;
  GstElement *h264parse0, *h264parse1;
  GstElement *matroskamux;
  GstElement *filesink;
} CustomData;

static void on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;
  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking src/sink\n");
  sinkpad = gst_element_get_static_pad (decoder, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  /* Timestamp stuff for filename */
  char filename[21];
  struct tm *timenow;
  time_t now = time(NULL);
  timenow = gmtime(&now);
  strftime(filename, sizeof(filename), "%Y-%m-%d_%H:%M:%S", timenow);
  //puts(filename);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the pipelines elements */
  data.rtspsrc = gst_element_factory_make ("rtspsrc", "rtspsrc");
  data.queue0 = gst_element_factory_make ("queue0", "queue0");
  data.queue1 = gst_element_factory_make ("queue1", "queue1");
  data.capsfilter0 = gst_element_factory_make ("capsfilter0", "capsfilter0");
  data.capsfilter1 = gst_element_factory_make ("capsfilter1", "capsfilter1");
  data.rtph264depay0 = gst_element_factory_make ("rtph264depay0", "rtph264depay0");
  data.rtph264depay1 = gst_element_factory_make ("rtph264depay1", "rtph264depay1");
  data.h264parse0 = gst_element_factory_make ("h264parse0", "h264parse0");
  data.h264parse1 = gst_element_factory_make ("h264parse1", "h264parse1");
  data.matroskamux = gst_element_factory_make ("matroskamux", "matroskamux");
  data.filesink = gst_element_factory_make ("filesink", "filesink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");

  /* Check the elements validity */
  if ( !data.pipeline || !data.rtspsrc ) {
    g_printerr ("An element in PIPELINE/SRC group could not be created.\n"); return -1;
  } else if ( !data.queue0 || !data.queue1 ) {
    g_printerr ("An element in QUEUE group could not be created.\n"); return -1;
  } else if ( !data.capsfilter0 || !data.capsfilter1 ) {
    g_printerr ("An element in CAPSFILTER group could not be created.\n"); return -1;
  } else if ( !data.rtph264depay0 || !data.rtph264depay1 || !data.h264parse0 || !data.h264parse1 ) {
      g_printerr ("An element in DEPAY/PARSE group could not be created.\n"); return -1;
  } else if ( !data.matroskamux || !data.filesink ) {
      g_printerr ("An element in MUX/SINK group could not be created.\n"); return -1;
  } else {
    g_printerr ("--All elements were created equally.\n");
  }

  /* Set parameters for some elements */
	g_object_set(G_OBJECT(data.rtspsrc), "location", "rtsp://root:hest1234@192.168.130.203/axis-media/media.amp", "ntp-sync", TRUE, "protocols", 0x00000004, NULL);
  g_object_set(G_OBJECT(data.rtspsrc), "location", "rtsp://root:hest1234@192.168.130.204/axis-media/media.amp", "ntp-sync", TRUE, "protocols", 0x00000004, NULL);
  g_object_set(G_OBJECT(data.capsfilter0), "caps", gst_caps_from_string("application/x-rtp,media=video"), NULL);
	g_object_set(G_OBJECT(data.capsfilter1), "caps", gst_caps_from_string("application/x-rtp,media=video"), NULL);
  //g_object_set(G_OBJECT(data.matroskamux), "name", "mux", NULL);
	g_object_set(G_OBJECT(data.filesink), "location", "/home/kim/thebarnOWL/theGstream/record/clips/%s.mkv", filename, NULL);

  /* Build the pipeline. Though not adding the src yet*/
  g_print ("---Adding elements to a bin ... \n");
  gst_bin_add_many (GST_BIN (data.pipeline), data.rtspsrc, data.queue0, data.queue1, data.capsfilter0, data.capsfilter1, data.rtph264depay0, data.rtph264depay1, data.h264parse0, data.h264parse1, data.matroskamux, data.filesink, NULL);

  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipe_PAUSE");

  g_print ("---and linking them ... \n");
  gst_element_link_many(data.rtspsrc, data.queue1, NULL);
  g_signal_connect(data.rtspsrc, "pad-added", G_CALLBACK (on_pad_added), data.queue1);
  if (!gst_element_link_many(data.queue0, data.capsfilter0, data.rtph264depay0, data.h264parse0, data.matroskamux, NULL))
    g_error("---Failed to link convert pipe 0!");
  if (!gst_element_link_many(data.queue1, data.capsfilter1, data.rtph264depay1, data.h264parse1, data.matroskamux, NULL))
    g_error("---Failed to link convert pipe 1!");
  if (!gst_element_link_many(data.matroskamux, data.filesink, NULL))
    g_error("Failed to link matroskamux_0 to filesink!");
  if (!gst_element_link_many(data.matroskamux, data.filesink, NULL))
    g_error("Failed to link matroskamux_1 to filesink!");

  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  } else {
    g_printerr ("Pipe should be playing now..now..now..\n");
  }
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipe_PLAY");

  loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

  g_print ("Stop the pipeline ... \n");
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  g_main_loop_unref(loop);
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipe_NULL");

  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
                gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  return 0;

  //GST_DEBUG_BIN_TO_DOT_FILE(data.pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "test-pipeline");
}

// Compiling shit
// libtool --mode=link gcc `pkg-config --cflags --libs gstreamer-1.0` -o myprog myprog.c




























//Bye
