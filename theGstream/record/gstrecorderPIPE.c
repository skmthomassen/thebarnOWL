#include <gst/gst.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

static GMainLoop *loop;

GstElement *pipeline;
GstMessage *msg;
GstBus *bus;
GError *error = NULL;

int fifo;
char *fifo_name = "/home/kim/thebarnOWL/theGstream/record/gst.fifo";
char buf[128];
guint bus_watch_id;
int running = 0;

int bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
	return TRUE;
}

void record()
{
	printf("Now recording...\n");

	if(pipeline) {
		return;
	}

	error = NULL;

	pipeline = gst_parse_launch("mpegtsmux name=mux ! filesink location=/home/kim/thebarnOWL/theGstream/record/clips/2018-3-16_dome.ts ! rtspsrc location=rtsp://root:hest1234@192.168.130.203/axis-media/media.amp ntp-sync=true protocols=GST_RTSP_LOWER_TRANS_TCP ! queue ! capsfilter caps=\"application/x-rtp,media=video\" ! rtph264depay ! mux. rtspsrc location=rtsp://root:hest1234@192.168.130.204/axis-media/media.amp ntp-sync=true protocols=GST_RTSP_LOWER_TRANS_TCP ! queue ! capsfilter caps=\"application/x-rtp,media=video\" ! rtph264depay ! mux.", &error);

	if(!pipeline) {
		printf("Parse error: %s\n", error->message);
		exit(1);
	}

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	bus_watch_id = gst_bus_add_watch (bus, bus_callback, loop);
	gst_object_unref (bus);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void stop_recording()
{
	if(pipeline) {
		gst_element_set_state (pipeline, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (pipeline));
		pipeline = NULL;
		g_source_remove(bus_watch_id);
	}
}

gboolean timeout(gpointer data)
{
	fifo = open(fifo_name, O_RDONLY);

	memset(buf, 0, strlen(buf));

	if(read(fifo, &buf, sizeof(char)*128) > 0)
	{
		size_t ln = strlen(buf) - 1;

		if (buf[ln] == '\n')
			buf[ln] = '\0';

		if(strcmp(buf, "start") == 0) {
			// Start recording
			printf("Starting recorder...\n");
			record();
			running = 1;
		} else if(strcmp(buf, "stop") == 0) {
			// Stop recording
			printf("Stopping recorder...\n");
			stop_recording();
			running = 0;
		} else if(strcmp(buf, "status") == 0) {
			printf("Writing status to /var/run/gst.status\n");

			FILE *status_fp;
			status_fp = fopen("/var/run/gst.status", "w");

			if(running == 0) {
				fprintf(status_fp, "idle");
			} else if(running == 1) {
				fprintf(status_fp, "recording");
			}

			fclose(status_fp);
		}
	}

	close(fifo);

	return TRUE;
}

int main(int argc, char *argv[])
{
	gst_init(&argc, &argv);

	g_timeout_add(500, timeout, NULL);

	loop = g_main_loop_new (NULL, FALSE);

	g_main_loop_run (loop);

	g_main_loop_quit(loop);

	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);
	gst_message_unref (msg);

	return 0;
}
