#include <gst/gst.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

static GMainLoop *loop;

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *rtspsrc;
  GstElement *capsfilter;
  GstElement *rtph264depay;
  GstElement *h264parse;
  GstElement *matroskamux;
  GstElement *multifilesink;
  //GstElement *filesink;
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

  /* Get path of current directory */
  char cwd[128];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    fprintf(stdout, "Current working dir: %s\n", cwd);
  else
    perror("---getcwd() error");
  /*Time stamp stuff */
  char dstamp[16];
  char tstamp[16];
  struct tm *timenow;
  time_t now = time(NULL);
  timenow = gmtime(&now);
  if (strftime(dstamp, sizeof(dstamp), "%Y-%m-%d", timenow) != '\0')
    fprintf(stdout, "DATE be: %s\n", dstamp);
  else
    perror("---strftime DATE error");
  if (strftime(tstamp, sizeof(tstamp), "%H:%M:%S", timenow) != '\0')
    fprintf(stdout, "TIME be: %s\n", tstamp);
  else
    perror("---strftime TIME error");
  /* make dir */
  char *dirname = NULL;
  dirname = g_strconcat (cwd, "/clips/", dstamp, "/", NULL);
  int result = mkdir(dirname, 0777);
  /* Connect into filename */
  char *filename = NULL;
  filename = g_strconcat (dirname, tstamp, "_%05d", ".mkv", NULL);
  fprintf(stdout, "FILENAME be: %s\n", filename);

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the pipelines elements */
  data.rtspsrc = gst_element_factory_make ("rtspsrc", "rtspsrc");
  data.capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  data.rtph264depay = gst_element_factory_make ("rtph264depay", "rtph264depay");
  data.h264parse = gst_element_factory_make ("h264parse", "h264parse");
  data.matroskamux = gst_element_factory_make ("matroskamux", "matroskamux");
  data.multifilesink = gst_element_factory_make ("multifilesink", "multifilesink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("test-pipeline");

  // !data.capsfilter ||
  /* Check the elements validity */
  if (!data.pipeline || !data.rtspsrc || !data.matroskamux || !data.rtph264depay || !data.h264parse || !data.multifilesink ) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  } else {
    g_printerr ("\n --All elements was created equally.\n");
  }

  /* Set parameters for some elements */
	g_object_set(G_OBJECT(data.rtspsrc), "location", "rtsp://root:hest1234@192.168.130.203/axis-media/media.amp", "ntp-sync", TRUE, "protocols", 0x00000004, NULL);
	g_object_set(G_OBJECT(data.capsfilter), "caps", gst_caps_from_string("application/x-rtp,media=video"), NULL);
  g_printerr ("---capsfilter.\n");
	//g_object_set(G_OBJECT(data.filesink), "location", filename, NULL);
  g_object_set(G_OBJECT(data.multifilesink), "next-file", "max-duration", "max-file-duration", "100000000000", "location", filename, NULL);
  g_printerr ("---filesink.\n");
  //multifilesink next-file=max-duration max-file-duration=10000000000 location=$VID_DIR/$FILENAME-%05d.mkv

  /* Build the pipeline. Though not adding the src yet*/
  g_print ("---Adding elements to a bin ... \n");
  gst_bin_add_many (GST_BIN (data.pipeline), data.rtspsrc, data.capsfilter, data.rtph264depay, data.h264parse, data.matroskamux, data.multifilesink, NULL);

  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipe_PAUSE");

  g_print ("---and linking them ... \n");
  gst_element_link_many(data.rtspsrc, data.capsfilter, NULL);
  g_signal_connect(data.rtspsrc, "pad-added", G_CALLBACK (on_pad_added), data.capsfilter);
  if (!gst_element_link_many(data.capsfilter, data.rtph264depay, NULL))
		g_error("Failed to link rtph264depay to h264parse!");
  if (!gst_element_link_many(data.rtph264depay, data.h264parse, NULL))
		g_error("Failed to link rtph264depay to h264parse!");
  if (!gst_element_link_many(data.h264parse, data.matroskamux, NULL))
    g_error("Failed to link h264parse to matroskamux!");
  if (!gst_element_link_many(data.matroskamux, data.multifilesink, NULL))
    g_error("Failed to link matroskamux to filesink!");

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

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
  GstPad *sink_pad = gst_element_get_static_pad (data->multifilesink, "sink");
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("\nReceived new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

  /* If our converter is already linked, we have nothing to do here */
  if (gst_pad_is_linked (sink_pad)) {
    g_print ("  We are already linked. Ignoring.\n");
    goto exit;
  }

  /* Check the new pad's type */
  new_pad_caps = gst_pad_query_caps (new_pad, NULL);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  g_print ("new_pad_caps type is '%s'.\n", new_pad_caps);
  g_print ("new_pad_struct type is '%s'.\n", new_pad_struct);
  g_print ("new_pad_caps type is '%s'.\n", new_pad_type);

/*  if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("  It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    goto exit;
  }
*/
  /* Attempt the link */
  ret = gst_pad_link (new_pad, sink_pad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("  Type is '%s' but link failed.\n", new_pad_type);
  } else {
    g_print ("  Link succeeded (type '%s').\n", new_pad_type);
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

  /* Unreference the sink pad */
  gst_object_unref (sink_pad);
}






























//Bye
