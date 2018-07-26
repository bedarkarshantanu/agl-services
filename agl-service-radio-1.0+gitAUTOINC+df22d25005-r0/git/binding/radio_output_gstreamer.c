/*
 * Copyright (C) 2018 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <gst/gst.h>

#include "radio_output.h"
#include "rtl_fm.h"

// Output buffer
static unsigned int extra;
static int16_t extra_buf[1];
static unsigned char *output_buf;

// GStreamer state
static GstElement *pipeline, *appsrc;
static bool running;

int radio_output_open()
{
	unsigned int rate = 24000;
	GstElement *queue, *convert, *sink, *resample;
	char *p;

	if(pipeline)
		return 0;

	// Initialize GStreamer
#ifdef DEBUG
	unsigned int argc = 2;
	char **argv = malloc(2 * sizeof(char*));
	argv[0] = strdup("test");
	argv[1] = strdup("--gst-debug-level=5");
	gst_init(&argc, &argv);
#else
	gst_init(NULL, NULL);
#endif

	// Setup pipeline
	// NOTE: With our use of the simple buffer pushing mode, there seems to
	//       be no need for a mainloop, so currently not instantiating one.
	pipeline = gst_pipeline_new("pipeline");
	appsrc = gst_element_factory_make("appsrc", "source");
	queue = gst_element_factory_make("queue", "queue");
	convert = gst_element_factory_make("audioconvert", "convert");
	resample = gst_element_factory_make("audioresample", "resample");
	sink = gst_element_factory_make("alsasink", "sink");
	if(!(pipeline && appsrc && queue && convert && resample && sink)) {
		fprintf(stderr, "pipeline element construction failed!\n");
	}
	g_object_set(G_OBJECT(appsrc), "caps",
		     gst_caps_new_simple("audio/x-raw",
					 "format", G_TYPE_STRING, "S16LE",
					 "rate", G_TYPE_INT, rate,
					 "channels", G_TYPE_INT, 2,
					 "layout", G_TYPE_STRING, "interleaved",
					 "channel-mask", G_TYPE_UINT64, 3,
					 NULL), NULL);

	if((p = getenv("RADIO_OUTPUT"))) {
		fprintf(stderr, "Using output device %s\n", p);
		g_object_set(sink,  "device",  p, NULL);
	}
	gst_bin_add_many(GST_BIN(pipeline), appsrc, queue, convert, resample, sink, NULL);
	gst_element_link_many(appsrc, queue, convert, resample, sink, NULL);
	//gst_bin_add_many(GST_BIN(pipeline), appsrc, convert, resample, sink, NULL);
	//gst_element_link_many(appsrc, convert, resample, sink, NULL);

	// Set up appsrc
	// NOTE: Radio seems like it matches the use case the "is-live" property
	//       is for, but setting it seems to require a lot more work with
	//       respect to latency settings to make the pipeline work smoothly.
	//       For now, leave it unset since the result seems to work
	//       reasonably well.
	g_object_set(G_OBJECT(appsrc),
		     "stream-type", 0,
		     "format", GST_FORMAT_TIME,
		     NULL);

	// Start pipeline in paused state
	gst_element_set_state(pipeline, GST_STATE_PAUSED);

	// Set up output buffer
	extra = 0;
	output_buf = malloc(sizeof(unsigned char) * RTL_FM_MAXIMUM_BUF_LENGTH);

	return 0;
}

int radio_output_start(void)
{
	int rc = 0;

	if(!pipeline) {
		rc = radio_output_open();
		if(rc)
			return rc;
	}

	// Start pipeline
	running = true;
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return rc;
}

void radio_output_stop(void)
{
	GstEvent *event;

	if(pipeline && running) {
		// Stop pipeline
		running = false;
		gst_element_set_state(pipeline, GST_STATE_PAUSED);

		// Flush pipeline
		// This seems required to avoid stutters on starts after a stop
		event = gst_event_new_flush_start();
		gst_element_send_event(GST_ELEMENT(pipeline), event);
		event = gst_event_new_flush_stop(TRUE);
		gst_element_send_event(GST_ELEMENT(pipeline), event); 
	}
}

void radio_output_suspend(int state)
{
	// Placeholder
}

void radio_output_close(void)
{
	radio_output_stop();

	if(pipeline) {
		// Tear down pipeline
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(pipeline));
		pipeline = NULL;
		running = false;
	}

	free(output_buf);
	output_buf = NULL;
}

ssize_t radio_output_write(void *buf, int len)
{
	ssize_t rc = -EINVAL;
	size_t n = len;
	int samples = len / 2;
	unsigned char *p;
	GstBuffer *buffer;
	GstFlowReturn ret;

	if(!(pipeline && buf)) {
		return rc;
	}

	// Don't bother pushing samples if output hasn't started
	if(!running)
		return 0;

	/*
	 * Handle the rtl_fm code giving us an odd number of samples.
	 * This extra buffer copying approach is not particularly efficient,
	 * but works for now.  It looks feasible to hack in something in the
	 * demod and output thread routines in rtl_fm.c to handle it there
	 * if more performance is required.
	 */
	p = output_buf;
	if(extra) {
		memcpy(output_buf, extra_buf, sizeof(int16_t));
		if((extra + samples) % 2) {
			// We still have an extra sample, n remains the same, store the extra
			memcpy(output_buf + sizeof(int16_t), buf, n - 2);
			memcpy(extra_buf, ((unsigned char*) buf) + n - 2, sizeof(int16_t));
		} else {
			// We have an even number of samples, no extra
			memcpy(output_buf + sizeof(int16_t), buf, n);
			n += 2;
			extra = 0;
		}
	} else if(samples % 2) {
		// We have an extra sample, store it, and decrease n
		n -= 2;
		memcpy(output_buf + sizeof(int16_t), buf, n);
		memcpy(extra_buf, ((unsigned char*) buf) + n, sizeof(int16_t));
		extra = 1;
	} else {
		p = buf;
	}

	// Push buffer into pipeline
	buffer = gst_buffer_new_allocate(NULL, n, NULL);
	gst_buffer_fill(buffer, 0, p, n);
	g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
	gst_buffer_unref(buffer);
	rc = n;

	return rc;
}
