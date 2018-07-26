/*
 * Copyright (C) 2017 Konsulko Group
 * Author: Matt Ranostay <matt.ranostay@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>
#include <pthread.h>
#include <gst/gst.h>
#include <gst/tag/tag.h>
#include <json-c/json.h>
#include "afm-common.h"

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

static struct afb_event playlist_event;
static struct afb_event metadata_event;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static GList *playlist = NULL;
static GList *metadata_track = NULL;
static GList *current_track = NULL;

typedef struct _CustomData {
	GstElement *playbin, *fake_sink, *alsa_sink;
	gboolean playing;
	gboolean loop;
	gboolean one_time;
	long int volume;
	gint64 position;
	gint64 duration;
} CustomData;

CustomData data = {
	.volume = 50,
	.position = GST_CLOCK_TIME_NONE,
	.duration = GST_CLOCK_TIME_NONE,
};

static json_object *populate_json(struct playlist_item *track)
{
	json_object *jresp = json_object_new_object();
	json_object *jstring = json_object_new_string(track->media_path);
	json_object_object_add(jresp, "path", jstring);

	if (track->title) {
		jstring = json_object_new_string(track->title);
		json_object_object_add(jresp, "title", jstring);
	}

	if (track->album) {
		jstring = json_object_new_string(track->album);
		json_object_object_add(jresp, "album", jstring);
	}

	if (track->artist) {
		jstring = json_object_new_string(track->artist);
		json_object_object_add(jresp, "artist", jstring);
	}

	if (track->genre) {
		jstring = json_object_new_string(track->genre);
		json_object_object_add(jresp, "genre", jstring);
	}

	if (track->duration > 0)
		json_object_object_add(jresp, "duration",
			       json_object_new_int64(track->duration));

	json_object_object_add(jresp, "index",
			       json_object_new_int(track->id));

	if (current_track && current_track->data)
		json_object_object_add(jresp, "selected",
			json_object_new_boolean(track == current_track->data));

	return jresp;
}

static gboolean populate_from_json(struct playlist_item *item, json_object *jdict)
{
	gboolean ret;
	json_object *val = NULL;

	ret = json_object_object_get_ex(jdict, "path", &val);
	if (!ret)
		return ret;
	item->media_path = g_strdup(json_object_get_string(val));

	ret = json_object_object_get_ex(jdict, "type", &val);
	if (!ret) {
		g_object_unref(item->media_path);
		return ret;
	}
	item->media_type = g_strdup(json_object_get_string(val));


	ret = json_object_object_get_ex(jdict, "title", &val);
	if (ret) {
		item->title = g_strdup(json_object_get_string(val));
	}

	ret = json_object_object_get_ex(jdict, "album", &val);
	if (ret) {
		item->album = g_strdup(json_object_get_string(val));
	}

	ret = json_object_object_get_ex(jdict, "artist", &val);
	if (ret) {
		item->artist = g_strdup(json_object_get_string(val));
	}

	ret = json_object_object_get_ex(jdict, "genre", &val);
	if (ret) {
		item->genre = g_strdup(json_object_get_string(val));
	}

	ret = json_object_object_get_ex(jdict, "duration", &val);
	if (ret) {
		item->duration = json_object_get_int64(val);
	}

	return TRUE;
}

static int set_media_uri(struct playlist_item *item)
{
	if (!item || !item->media_path)
		return -ENOENT;

	gst_element_set_state(data.playbin, GST_STATE_NULL);

	g_object_set(data.playbin, "uri", item->media_path, NULL);

	data.position = GST_CLOCK_TIME_NONE;
	data.duration = GST_CLOCK_TIME_NONE;

	if (data.playing) {
		g_object_set(data.playbin, "audio-sink", data.alsa_sink, NULL);
		gst_element_set_state(data.playbin, GST_STATE_PLAYING);
	} else {
		g_object_set(data.playbin, "audio-sink", data.fake_sink, NULL);
		gst_element_set_state(data.playbin, GST_STATE_PAUSED);
	}

	g_object_set(data.playbin, "volume", (double) data.volume / 100.0, NULL);

	return 0;
}


static int in_list(gconstpointer item, gconstpointer list)
{
	return g_strcmp0(((struct playlist_item *) item)->media_path,
			 ((struct playlist_item *) list)->media_path);
}

static void populate_playlist(json_object *jquery)
{
	int i, idx = 0;
	GList *list = g_list_last(playlist);

	if (list && list->data) {
		struct playlist_item *item = list->data;
		idx = item->id + 1;
	}

	for (i = 0; i < json_object_array_length(jquery); i++) {
		json_object *jdict = json_object_array_get_idx(jquery, i);
		struct playlist_item *item = g_malloc0(sizeof(*item));
		int ret;

		if (item == NULL)
			break;

		ret = populate_from_json(item, jdict);
		if (!ret || g_list_find_custom(playlist, item, in_list)) {
			g_free_playlist_item(item);
			continue;
		}

		item->id = idx++;
		playlist = g_list_append(playlist, item);
	}

	if (current_track == NULL) {
		current_track = g_list_first(playlist);
		if (current_track && current_track->data)
			set_media_uri(current_track->data);
	}
}

static json_object *populate_json_playlist(json_object *jresp)
{
	GList *l;
	json_object *jarray = json_object_new_array();

	for (l = playlist; l; l = l->next) {
		struct playlist_item *track = l->data;

		if (track && !g_strcmp0(track->media_type, "audio")) {
			json_object *item = populate_json(track);
			json_object_array_add(jarray, item);
		}
	}

	json_object_object_add(jresp, "list", jarray);

	return jresp;
}

static void audio_playlist(struct afb_req request)
{
	const char *value = afb_req_value(request, "list");
	json_object *jresp = NULL;

	pthread_mutex_lock(&mutex);

	if (value) {
		json_object *jquery;

		if (playlist) {
			g_list_free_full(playlist, g_free_playlist_item);
			playlist = NULL;
		}

		jquery = json_tokener_parse(value);
		populate_playlist(jquery);

		if (playlist == NULL)
			afb_req_fail(request, "failed", "invalid playlist");
		else
			afb_req_success(request, NULL, NULL);

		json_object_put(jquery);
	} else {
		jresp = json_object_new_object();
		jresp = populate_json_playlist(jresp);

		afb_req_success(request, jresp, "Playlist results");
	}

	pthread_mutex_unlock(&mutex);
}

static int seek_stream(const char *value, int cmd)
{
	gint64 position, current = 0;

	if (value == NULL)
		return -EINVAL;

	position = strtoll(value, NULL, 10);

	if (cmd != SEEK_CMD) {
		gst_element_query_position (data.playbin, GST_FORMAT_TIME, &current);
		position = (current / GST_MSECOND) + (FASTFORWARD_CMD ? position : -position);
	}

	if (position < 0)
		position = 0;

	if (data.duration > 0 && position > data.duration)
		position = data.duration;

	return gst_element_seek_simple(data.playbin, GST_FORMAT_TIME,
				GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
				position * GST_MSECOND);
}

static int seek_track(int cmd)
{
	GList *item = NULL;
	int ret;

	if (current_track == NULL)
		return -EINVAL;

	item = (cmd == NEXT_CMD) ? current_track->next : current_track->prev;

	if (item == NULL) {
		if (cmd == PREVIOUS_CMD) {
			seek_stream("0", SEEK_CMD);
			return 0;
		}
		return -EINVAL;
	}

	data.playing = TRUE;
	ret = set_media_uri(item->data);
	if (ret < 0)
		return -EINVAL;

	current_track = item;

	return 0;
}

/* @value can be one of the following values:
 *   play     - go to playing transition
 *   pause    - go to pause transition
 *   previous - skip to previous track
 *   next     - skip to the next track
 *   seek     - go to position (in milliseconds)
 *
 *   fast-forward - skip forward in milliseconds
 *   rewind       - skip backward in milliseconds
 *
 *   pick-track   - select track via index number
 *   volume       - set volume between 0 - 100%
 *   loop         - set looping of playlist (true or false)
 */

static void controls(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");
	const char *position = afb_req_value(request, "position");
	int cmd = get_command_index(value);
	json_object *jresp = NULL;

	if (!value) {
		afb_req_fail(request, "failed", "no value was passed");
		return;
	}

	pthread_mutex_lock(&mutex);
	errno = 0;

	switch (cmd) {
	case PLAY_CMD: {
		GstElement *obj = NULL;
		data.playing = TRUE;
		g_object_get(data.playbin, "audio-sink", &obj, NULL);

		if (obj == data.fake_sink) {
			if (current_track && current_track->data)
				set_media_uri(current_track->data);
			else {
				pthread_mutex_unlock(&mutex);
				afb_req_fail(request, "failed", "No playlist");
				return;
			}
		} else {
			g_object_set(data.playbin, "audio-sink", data.alsa_sink, NULL);
			gst_element_set_state(data.playbin, GST_STATE_PLAYING);
		}

		jresp = json_object_new_object();
		json_object_object_add(jresp, "playing", json_object_new_boolean(TRUE));
		break;
	}
	case PAUSE_CMD:
		jresp = json_object_new_object();
		gst_element_set_state(data.playbin, GST_STATE_PAUSED);
		data.playing = FALSE;
		json_object_object_add(jresp, "playing", json_object_new_boolean(FALSE));
		break;
	case PREVIOUS_CMD:
	case NEXT_CMD:
		seek_track(cmd);
		break;
	case SEEK_CMD:
	case FASTFORWARD_CMD:
	case REWIND_CMD:
		seek_stream(position, cmd);
		break;
	case PICKTRACK_CMD: {
		const char *parameter = afb_req_value(request, "index");
		long int idx = strtol(parameter, NULL, 10);
		GList *list = NULL;

		if (idx == 0 && errno) {
			afb_req_fail(request, "failed", "invalid index");
			pthread_mutex_unlock(&mutex);
			return;
		}

		list = find_media_index(playlist, idx);
		if (list != NULL) {
			struct playlist_item *item = list->data;
			data.playing = TRUE;
			set_media_uri(item);
			current_track = list;
		} else {
			afb_req_fail(request, "failed", "couldn't find index");
			pthread_mutex_unlock(&mutex);
			return;
		}

		break;
	}
	case VOLUME_CMD: {
		const char *parameter = afb_req_value(request, "volume");
		long int volume = strtol(parameter, NULL, 10);

		if (volume == 0 && errno) {
			afb_req_fail(request, "failed", "invalid volume");
			pthread_mutex_unlock(&mutex);
			return;
		}

		if (volume < 0)
			volume = 0;
		if (volume > 100)
			volume = 100;

		g_object_set(data.playbin, "volume", (double) volume / 100.0, NULL);

		data.volume = volume;

		break;
	}
	case LOOP_CMD: {
		const char *state = afb_req_value(request, "state");
		data.loop = !strcasecmp(state, "true") ? TRUE : FALSE;
		break;
	}
	case STOP_CMD:
		gst_element_set_state(data.playbin, GST_STATE_NULL);
		break;
	default:
		afb_req_fail(request, "failed", "unknown command");
		pthread_mutex_unlock(&mutex);
		return;
	}

	afb_req_success(request, jresp, NULL);
	pthread_mutex_unlock(&mutex);
}

static GstSample *parse_album(GstTagList *tags, gchar *tag_type)
{
	GstSample *sample = NULL;
	int num = gst_tag_list_get_tag_size(tags, tag_type);
	guint i;

	for (i = 0; i < num ; i++) {
		const GValue *value;
		GstStructure *caps;
		int type;

		value = gst_tag_list_get_value_index(tags, tag_type, i);
		if (value == NULL)
			break;

		sample = gst_value_get_sample(value);
		caps = gst_caps_get_structure(gst_sample_get_caps(sample), 0);
		gst_structure_get_enum(caps, "image-type",
				       GST_TYPE_TAG_IMAGE_TYPE, &type);

		if (type == GST_TAG_IMAGE_TYPE_FRONT_COVER)
			break;
	}

	return sample;
}

static gchar *get_album_art(GstTagList *tags)
{
	GstSample *sample;

	sample = parse_album(tags, GST_TAG_IMAGE);
	if (!sample)
		sample = parse_album(tags, GST_TAG_PREVIEW_IMAGE);

	if (sample) {
		GstBuffer *buffer = gst_sample_get_buffer(sample);
		GstMapInfo map;
		gchar *data, *mime_type, *image;

		if (!gst_buffer_map (buffer, &map, GST_MAP_READ))
			return NULL;

		image = g_base64_encode(map.data, map.size);
		mime_type = g_content_type_guess(NULL, map.data, map.size, NULL);

		data = g_strconcat("data:", mime_type, ";base64,", image, NULL);

		g_free(image);
		g_free(mime_type);
		gst_buffer_unmap(buffer, &map);

		return data;
	}

	return NULL;
}

static json_object *populate_json_metadata_image(json_object *jresp)
{
	GstTagList *tags = NULL;

	g_signal_emit_by_name(G_OBJECT(data.playbin), "get-audio-tags", 0, &tags);

	if (tags) {
		gchar *image = get_album_art(tags);

		json_object_object_add(jresp, "image",
			json_object_new_string(image ? image : "data:null"));

		g_free(image);

		gst_tag_list_free(tags);
	}

	return jresp;
}

static json_object *populate_json_metadata(void)
{
	struct playlist_item *track;
	json_object *jresp;

	if (current_track == NULL || current_track->data == NULL)
		return NULL;

	track = current_track->data;
	jresp = populate_json(track);

	if (data.duration != GST_CLOCK_TIME_NONE)
		json_object_object_add(jresp, "duration",
			       json_object_new_int64(data.duration / GST_MSECOND));

	if (data.position != GST_CLOCK_TIME_NONE)
		json_object_object_add(jresp, "position",
			       json_object_new_int64(data.position / GST_MSECOND));

	json_object_object_add(jresp, "volume",
			       json_object_new_int(data.volume));

	jresp = populate_json_metadata_image(jresp);

	return jresp;
}

static void metadata(struct afb_req request)
{
	json_object *jresp;

	pthread_mutex_lock(&mutex);
	jresp = populate_json_metadata();
	pthread_mutex_unlock(&mutex);

	if (jresp == NULL)
		afb_req_fail(request, "failed", "No metadata");
	else
		afb_req_success(request, jresp, "Metadata results");
}

static void subscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");

	if (!strcasecmp(value, "metadata")) {
		json_object *jresp = NULL;

		afb_req_subscribe(request, metadata_event);
		afb_req_success(request, NULL, NULL);

		pthread_mutex_lock(&mutex);
		jresp = populate_json_metadata();
		pthread_mutex_unlock(&mutex);

		afb_event_push(metadata_event, jresp);

		return;
	} else if (!strcasecmp(value, "playlist")) {
		json_object *jresp = json_object_new_object();

		afb_req_subscribe(request, playlist_event);
		afb_req_success(request, NULL, NULL);

		pthread_mutex_lock(&mutex);
		jresp = populate_json_playlist(jresp);
		pthread_mutex_unlock(&mutex);

		afb_event_push(playlist_event, jresp);

		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static void unsubscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");

	if (!strcasecmp(value, "metadata")) {
		afb_req_unsubscribe(request, metadata_event);
		afb_req_success(request, NULL, NULL);
		return;
	} else if (!strcasecmp(value, "playlist")) {
		afb_req_unsubscribe(request, playlist_event);
		afb_req_success(request, NULL, NULL);
		return;
	}

	afb_req_fail(request, "failed", "Invalid event");
}

static gboolean handle_message(GstBus *bus, GstMessage *msg, CustomData *data)
{
	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_EOS: {
		int ret;

		pthread_mutex_lock(&mutex);

		data->position = GST_CLOCK_TIME_NONE;
		data->duration = GST_CLOCK_TIME_NONE;

		ret = seek_track(NEXT_CMD);

		if (ret < 0) {
			if (!data->loop) {
				data->playing = FALSE;
				data->one_time = TRUE;
			}

			current_track = playlist;

			if (current_track != NULL)
				set_media_uri(current_track->data);
		} else if (data->playing) {
			gst_element_set_state(data->playbin, GST_STATE_PLAYING);
		}

		pthread_mutex_unlock(&mutex);
		break;
	}
	case GST_MESSAGE_DURATION:
		data->duration = GST_CLOCK_TIME_NONE;
		break;
	default:
		break;
	}

	return TRUE;
}

static gboolean position_event(CustomData *data)
{
	struct playlist_item *track;
	json_object *jresp = NULL;

	pthread_mutex_lock(&mutex);

	if (data->one_time) {
		data->one_time = FALSE;
		data->playing = FALSE;

		json_object *jresp = json_object_new_object();
		json_object_object_add(jresp, "status",
				       json_object_new_string("stopped"));
		pthread_mutex_unlock(&mutex);

		afb_event_push(metadata_event, jresp);
		return TRUE;
	}

	if (!data->playing || current_track == NULL) {
		pthread_mutex_unlock(&mutex);
		return TRUE;
	}

	track = current_track->data;
	jresp = populate_json(track);

	if (!GST_CLOCK_TIME_IS_VALID(data->duration))
		gst_element_query_duration(data->playbin,
					GST_FORMAT_TIME, &data->duration);

	gst_element_query_position(data->playbin,
					GST_FORMAT_TIME, &data->position);

	json_object_object_add(jresp, "duration",
			       json_object_new_int64(data->duration / GST_MSECOND));
	json_object_object_add(jresp, "position",
			       json_object_new_int64(data->position / GST_MSECOND));
	json_object_object_add(jresp, "status",
			       json_object_new_string("playing"));

	if (metadata_track != current_track) {
		jresp = populate_json_metadata_image(jresp);
		metadata_track = current_track;
	}

	pthread_mutex_unlock(&mutex);

        afb_event_push(metadata_event, jresp);

	return TRUE;
}

static void gstreamer_init()
{
	GstBus *bus;
	json_object *response;
	int ret;

	gst_init(NULL, NULL);

	data.playbin = gst_element_factory_make("playbin", "playbin");
	if (!data.playbin) {
		AFB_ERROR("Cannot create playbin");
		exit(1);
	}

	data.fake_sink = gst_element_factory_make("fakesink", NULL);
	data.alsa_sink = gst_element_factory_make("alsasink", NULL);

#ifdef HAVE_4A_FRAMEWORK
	json_object *jsonData = json_object_new_object();
	json_object_object_add(jsonData, "audio_role", json_object_new_string("Multimedia"));
	json_object_object_add(jsonData, "endpoint_type", json_object_new_string("sink"));
	ret = afb_service_call_sync("ahl-4a", "stream_open", jsonData, &response);

	if (!ret) {
		json_object *valJson = NULL;
		json_object *val = NULL;
		gboolean ret;
		ret = json_object_object_get_ex(response, "response", &valJson);
		if (ret) {
			ret = json_object_object_get_ex(valJson, "device_uri", &val);
			if (ret) {
				char* jres_pcm = json_object_get_string(val);
				gchar ** res_pcm= g_strsplit (jres_pcm,":",-1);
				if (res_pcm) {
					g_object_set(data.alsa_sink,  "device",  res_pcm[1], NULL);
					g_free(res_pcm);
				}
			}
			ret = json_object_object_get_ex(valJson, "stream_id", &val);
			if (ret) {
				int stream_id = json_object_get_int(val);
			}
		}
	}
#endif

	g_object_set(data.playbin, "audio-sink", data.fake_sink, NULL);
	gst_element_set_state(data.playbin, GST_STATE_PAUSED);

	bus = gst_element_get_bus(data.playbin);
	gst_bus_add_watch(bus, (GstBusFunc) handle_message, &data);
	g_timeout_add_seconds(1, (GSourceFunc) position_event, &data);

	ret = afb_service_call_sync("mediascanner", "media_result", NULL, &response);
	if (!ret) {
		json_object *val = NULL;
		gboolean ret;

		ret = json_object_object_get_ex(response, "response", &val);
		if (ret)
			ret = json_object_object_get_ex(val, "Media", &val);

		if (ret)
			populate_playlist(val);
	}
	json_object_put(response);
}

static void onevent(const char *event, struct json_object *object)
{
	json_object *jresp = NULL;

	if (!g_strcmp0(event, "mediascanner/media_added")) {
		json_object *val = NULL;
		gboolean ret;

		pthread_mutex_lock(&mutex);

		ret = json_object_object_get_ex(object, "Media", &val);
		if (ret)
			populate_playlist(val);

	} else if (!g_strcmp0(event, "mediascanner/media_removed")) {
		json_object *val = NULL;
		const char *path;
		GList *l = playlist;
		gboolean ret;

		ret = json_object_object_get_ex(object, "Path", &val);
		if (!ret)
			return;

		path = json_object_get_string(val);

		pthread_mutex_lock(&mutex);

		while (l) {
			struct playlist_item *item = l->data;

			l = l->next;

			if (!strncasecmp(path, item->media_path, strlen(path))) {
				if (current_track && current_track->data == item) {
					current_track = NULL;
					data.one_time = TRUE;
					gst_element_set_state(data.playbin, GST_STATE_NULL);
				}

				playlist = g_list_remove(playlist, item);
				g_free_playlist_item(item);
			}
		}

		if (current_track == NULL)
			current_track = g_list_first(playlist);
	} else {
		AFB_ERROR("Invalid event: %s", event);
		return;
	}

	jresp = json_object_new_object();
	jresp = populate_json_playlist(jresp);

	pthread_mutex_unlock(&mutex);

        afb_event_push(playlist_event, jresp);
}

void *gstreamer_loop_thread(void *ptr)
{
	g_main_loop_run(g_main_loop_new(NULL, FALSE));
}

static int init()
{
	pthread_t thread_id;
	json_object *response, *query;
	int ret;

	ret = afb_daemon_require_api("mediascanner", 1);
	if (ret < 0) {
		AFB_ERROR("Cannot request mediascanner");
		return ret;
	}

	query = json_object_new_object();
	json_object_object_add(query, "value", json_object_new_string("media_added"));

	ret = afb_service_call_sync("mediascanner", "subscribe", query, &response);
	json_object_put(response);

	if (ret < 0) {
		AFB_ERROR("Cannot subscribe to mediascanner media_added event");
		return ret;
	}

	query = json_object_new_object();
	json_object_object_add(query, "value", json_object_new_string("media_removed"));

	ret = afb_service_call_sync("mediascanner", "subscribe", query, &response);
	json_object_put(response);

	if (ret < 0) {
		AFB_ERROR("Cannot subscribe to mediascanner media_remove event");
		return ret;
	}

	metadata_event = afb_daemon_make_event("metadata");
	playlist_event = afb_daemon_make_event("playlist");

	gstreamer_init();

	return pthread_create(&thread_id, NULL, gstreamer_loop_thread, NULL);
}

static const struct afb_verb_v2 binding_verbs[] = {
	{ .verb = "playlist",     .callback = audio_playlist, .info = "Get/set playlist" },
	{ .verb = "controls",     .callback = controls,       .info = "Audio controls" },
	{ .verb = "metadata",     .callback = metadata,       .info = "Get metadata of current track" },
	{ .verb = "subscribe",    .callback = subscribe,      .info = "Subscribe to GStreamer events" },
	{ .verb = "unsubscribe",  .callback = unsubscribe,    .info = "Unsubscribe to GStreamer events" },
	{ }
};

/*
 * binder API description
 */
const struct afb_binding_v2 afbBindingV2 = {
	.api = "mediaplayer",
	.specification = "Mediaplayer API",
	.verbs = binding_verbs,
	.onevent = onevent,
	.init = init,
};
