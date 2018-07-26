/*
 * Copyright (C) 2017 Konsulko Group
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
#include <string.h>
#include <errno.h>
#include <pulse/pulseaudio.h>

#include "radio_output.h"
#include "rtl_fm.h"

static pa_threaded_mainloop *mainloop;
static pa_context *context;
static pa_stream *stream;

static unsigned int extra;
static int16_t extra_buf[1];
static unsigned char *output_buf;

static void pa_context_state_cb(pa_context *c, void *data) {

	assert(c);
	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		case PA_CONTEXT_READY:
			break;
		case PA_CONTEXT_TERMINATED:
			pa_threaded_mainloop_stop(mainloop);
			break;
		case PA_CONTEXT_FAILED:
		default:
			fprintf(stderr, "PA connection failed: %s\n",
				pa_strerror(pa_context_errno(c)));
			pa_threaded_mainloop_stop(mainloop);
			break;
	}
	pa_threaded_mainloop_signal(mainloop, 0);
}

int radio_output_open(void)
{
	pa_context *c;
	pa_mainloop_api *mapi;
	char *client;

	if(context)
		return 0;

	if (!(mainloop = pa_threaded_mainloop_new())) {
		fprintf(stderr, "pa_mainloop_new() failed.\n");
		return -1;
	}

	pa_threaded_mainloop_set_name(mainloop, "pa_mainloop");
	mapi = pa_threaded_mainloop_get_api(mainloop);

	client = pa_xstrdup("radio");
	if (!(c = pa_context_new(mapi, client))) {
		fprintf(stderr, "pa_context_new() failed.\n");
		goto exit;
	}

	pa_context_set_state_callback(c, pa_context_state_cb, NULL);
	if (pa_context_connect(c, NULL, 0, NULL) < 0) {
		fprintf(stderr, "pa_context_connect(): %s", pa_strerror(pa_context_errno(c)));
		goto exit;
	}

	if (pa_threaded_mainloop_start(mainloop) < 0) {
		fprintf(stderr, "pa_mainloop_run() failed.\n");
		goto exit;
	}

	context = c;

	extra = 0;
	output_buf = malloc(sizeof(unsigned char) * RTL_FM_MAXIMUM_BUF_LENGTH);

	return 0;

exit:
	if (c)
		pa_context_unref(c);

	if (mainloop)
		pa_threaded_mainloop_free(mainloop);

	pa_xfree(client);
	return -1;
}

int radio_output_start(void)
{
	int error = 0;
	pa_sample_spec *spec;

	if(stream)
		return 0;

	if(!context) {
		error = radio_output_open();
		if(error != 0)
			return error;
	}

	while(pa_context_get_state(context) != PA_CONTEXT_READY)
		pa_threaded_mainloop_wait(mainloop);

	spec = (pa_sample_spec*) calloc(1, sizeof(pa_sample_spec));
	spec->format = PA_SAMPLE_S16LE;
	spec->rate = 24000;
	spec->channels = 2;
	if (!pa_sample_spec_valid(spec)) {
		fprintf(stderr, "%s\n",
			pa_strerror(pa_context_errno(context)));
		return -1;
	}

	pa_threaded_mainloop_lock(mainloop);
	pa_proplist *props = pa_proplist_new();
	pa_proplist_sets(props, PA_PROP_MEDIA_ROLE, "radio");
	stream = pa_stream_new_with_proplist(context, "radio-output", spec, 0, props);
	if(!stream) {
		fprintf(stderr, "Error creating stream %s\n",
			pa_strerror(pa_context_errno(context)));
		pa_proplist_free(props);
		free(spec);
		pa_threaded_mainloop_unlock(mainloop);
		return -1;
	}
	pa_proplist_free(props);
	free(spec);

	if(pa_stream_connect_playback(stream,
				      NULL,
				      NULL,
				      (pa_stream_flags_t) 0,
				      NULL,
				      NULL) < 0) {
		fprintf(stderr, "Error connecting to PulseAudio : %s\n",
			pa_strerror(pa_context_errno(context)));
		pa_stream_unref(stream);
		stream = NULL;
		pa_threaded_mainloop_unlock(mainloop);
		return -1;
	}

	pa_threaded_mainloop_unlock(mainloop);

	while(pa_stream_get_state(stream) != PA_STREAM_READY)
		pa_threaded_mainloop_wait(mainloop);

	return error;
}

void radio_output_stop(void)
{
	if(stream) {
		pa_threaded_mainloop_lock(mainloop);

		pa_stream_set_state_callback(stream, 0, 0);
		pa_stream_set_write_callback(stream, 0, 0);
		pa_stream_set_underflow_callback(stream, 0, 0);
		pa_stream_set_overflow_callback(stream, 0, 0);
		pa_stream_set_latency_update_callback(stream, 0, 0);

		pa_operation *o = pa_stream_flush(stream, NULL, NULL);
		if(o)
			pa_operation_unref(o);

		pa_stream_disconnect(stream);
		pa_stream_unref(stream);
		stream = NULL;

		pa_threaded_mainloop_unlock(mainloop);
	}
}

void radio_output_suspend(int state)
{
	if(stream) {
		pa_stream_cork(stream, state, NULL, NULL);
	}
}

void radio_output_close(void)
{
	radio_output_stop();

	if(context) {
		pa_context_disconnect(context);
		pa_context_unref(context);
		context = NULL;
	}

	if(mainloop) {
		pa_threaded_mainloop_stop(mainloop);
		pa_threaded_mainloop_free(mainloop);
		mainloop = NULL;
	}

	free(output_buf);
	output_buf = NULL;
}

ssize_t radio_output_write(void *buf, int len)
{
	ssize_t rc = -EINVAL;

	size_t n = len;
	size_t avail;
	int samples = len / 2;
	void *p;

	if(!stream) {
		return -1;
	}

	if(!buf) {
		fprintf(stderr, "Error: buf == null!\n");
		return rc;
	}

	pa_threaded_mainloop_lock(mainloop);

	avail = pa_stream_writable_size(stream);
	if(avail < n) {
		/*
		 * NOTE: Definitely room for improvement here,but for now just
		 *       check for the no space case that happens when the
		 *       stream is corked.
		 */
		if(!avail) {
			rc = 0;
			goto exit;
		}
	}

	/*
	 * Handle the rtl_fm code giving us an odd number of samples, which
	 * PA does not like.  This extra buffer copying approach is not
	 * particularly efficient, but works for now.  It looks feasible to
	 * hack in something in the demod and output thread routines in
	 * rtl_fm.c to handle it there if more performance is required.
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

	if ((rc = pa_stream_write(stream, p, n, NULL, 0, PA_SEEK_RELATIVE)) < 0) {
		fprintf(stderr, "Error writing %zu bytes to PulseAudio : %s\n",
			n, pa_strerror(pa_context_errno(context)));
	}
exit:
	pa_threaded_mainloop_unlock(mainloop);

	return rc;
}
