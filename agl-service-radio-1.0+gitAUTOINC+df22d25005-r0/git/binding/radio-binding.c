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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <json-c/json.h>

#include <afb/afb-binding.h>

#include "radio_impl.h"
#include "radio_impl_rtlsdr.h"
#include "radio_impl_kingfisher.h"

static radio_impl_ops_t *radio_impl_ops;

static struct afb_event freq_event;
static struct afb_event scan_event;

static void freq_callback(uint32_t frequency, void *data)
{
	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_int((int) frequency);

	json_object_object_add(jresp, "value", value);
	afb_event_push(freq_event, json_object_get(jresp));
}

static void scan_callback(uint32_t frequency, void *data)
{
	json_object *jresp = json_object_new_object();
	json_object *value = json_object_new_int((int) frequency);

	json_object_object_add(jresp, "value", value);
	afb_event_push(scan_event, json_object_get(jresp));
}

/*
 * Binding verb handlers
 */

/*
 * @brief Get (and optionally set) frequency
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void frequency(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	uint32_t frequency;

	if(value) {
		char *p;
		frequency = (uint32_t) strtoul(value, &p, 10);
		if(frequency && *p == '\0') {
			radio_impl_ops->set_frequency(frequency);
		} else {
			afb_req_fail(request, "failed", "Invalid scan direction");
			return;
		}
	}
	ret_json = json_object_new_object();
	frequency = radio_impl_ops->get_frequency();
	json_object_object_add(ret_json, "frequency", json_object_new_int((int32_t) frequency));
	afb_req_success(request, ret_json, NULL);
}

/* @brief Get RDS information
*
* @param struct afb_req : an afb request structure
*
*/
static void rds(struct afb_req request)
{
	json_object *ret_json;
	char * rds;

	if (radio_impl_ops->get_rds_info == NULL) {
		afb_req_fail(request, "failed", "not supported");
		return;
	}

	ret_json = json_object_new_object();
	rds = radio_impl_ops->get_rds_info();
	json_object_object_add(ret_json, "rds", json_object_new_string(rds?rds:""));
	free(rds);

	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Get (and optionally set) frequency band
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void band(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	int valid = 0;
	radio_band_t band;
	char band_name[4];

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
		if(valid) {
			radio_impl_ops->set_band(band);
		} else {
			afb_req_fail(request, "failed", "Invalid band");
			return;
		}
	}
	ret_json = json_object_new_object();
	band = radio_impl_ops->get_band();
	sprintf(band_name, "%s", band == BAND_AM ? "AM" : "FM");
	json_object_object_add(ret_json, "band", json_object_new_string(band_name));
	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Check if band is supported
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void band_supported(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_fail(request, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	json_object_object_add(ret_json,
			       "supported",
			       json_object_new_int(radio_impl_ops->band_supported(band)));
	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Get frequency range for a band
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void frequency_range(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;
	uint32_t min_frequency;
	uint32_t max_frequency;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_fail(request, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	min_frequency = radio_impl_ops->get_min_frequency(band);
	max_frequency = radio_impl_ops->get_max_frequency(band);
	json_object_object_add(ret_json, "min", json_object_new_int((int32_t) min_frequency));
	json_object_object_add(ret_json, "max", json_object_new_int((int32_t) max_frequency));
	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Get frequency step size (Hz) for a band
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void frequency_step(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "band");
	int valid = 0;
	radio_band_t band;
	uint32_t step;

	if(value) {
		if(!strcasecmp(value, "AM")) {
			band = BAND_AM;
			valid = 1;
		} else if(!strcasecmp(value, "FM")) {
			band = BAND_FM;
			valid = 1;
		} else {
			char *p;
			band = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(band) {
				case BAND_AM:
				case BAND_FM:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_fail(request, "failed", "Invalid band");
		return;
	}
	ret_json = json_object_new_object();
	step = radio_impl_ops->get_frequency_step(band);
	json_object_object_add(ret_json, "step", json_object_new_int((int32_t) step));
	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Start radio playback
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void start(struct afb_req request)
{
	radio_impl_ops->start();
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Stop radio playback
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void stop(struct afb_req request)
{
	radio_impl_ops->stop();
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Scan for a station in the specified direction
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void scan_start(struct afb_req request)
{
	const char *value = afb_req_value(request, "direction");
	int valid = 0;
	radio_scan_direction_t direction;

	if(value) {
		if(!strcasecmp(value, "forward")) {
			direction = SCAN_FORWARD;
			valid = 1;
		} else if(!strcasecmp(value, "backward")) {
			direction = SCAN_BACKWARD;
			valid = 1;
		} else {
			char *p;
			direction = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(direction) {
				case SCAN_FORWARD:
				case SCAN_BACKWARD:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
	}
	if(!valid) {
		afb_req_fail(request, "failed", "Invalid direction");
		return;
	}
	radio_impl_ops->scan_start(direction, scan_callback, NULL);
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Stop station scan
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void scan_stop(struct afb_req request)
{
	radio_impl_ops->scan_stop();
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Get (and optionally set) stereo mode
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void stereo_mode(struct afb_req request)
{
	json_object *ret_json;
	const char *value = afb_req_value(request, "value");
	int valid = 0;
	radio_stereo_mode_t mode;

	if(value) {
		if(!strcasecmp(value, "mono")) {
			mode = MONO;
			valid = 1;
		} else if(!strcasecmp(value, "stereo")) {
			mode = STEREO;
			valid = 1;
		} else {
			char *p;
			mode = strtoul(value, &p, 10);
			if(p != value && *p == '\0') {
				switch(mode) {
				case MONO:
				case STEREO:
					valid = 1;
					break;
				default:
					break;
				}
			}
		}
		if(valid) {
			radio_impl_ops->set_stereo_mode(mode);
		} else {
			afb_req_fail(request, "failed", "Invalid mode");
			return;
		}
	}
	ret_json = json_object_new_object();
	mode = radio_impl_ops->get_stereo_mode();

	json_object_object_add(ret_json, "mode", json_object_new_string(mode == MONO ? "mono" : "stereo"));
	afb_req_success(request, ret_json, NULL);
}

/*
 * @brief Subscribe for an event
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void subscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "frequency")) {
			afb_req_subscribe(request, freq_event);
		} else if(!strcasecmp(value, "station_found")) {
			afb_req_subscribe(request, scan_event);
		} else {
			afb_req_fail(request, "failed", "Invalid event");
			return;
		}
	}
	afb_req_success(request, NULL, NULL);
}

/*
 * @brief Unsubscribe for an event
 *
 * @param struct afb_req : an afb request structure
 *
 */
static void unsubscribe(struct afb_req request)
{
	const char *value = afb_req_value(request, "value");
	if(value) {
		if(!strcasecmp(value, "frequency")) {
			afb_req_unsubscribe(request, freq_event);
		} else if(!strcasecmp(value, "station_found")) {
			afb_req_unsubscribe(request, scan_event);
		} else {
			afb_req_fail(request, "failed", "Invalid event");
			return;
		}
	}
	afb_req_success(request, NULL, NULL);
}

static const struct afb_verb_v2 verbs[]= {
	{ .verb = "frequency",		.session = AFB_SESSION_NONE, .callback = frequency,		.info = "Get/Set frequency" },
	{ .verb = "band",		.session = AFB_SESSION_NONE, .callback = band,			.info = "Get/Set band" },
	{ .verb = "rds",		.session = AFB_SESSION_NONE, .callback = rds,			.info = "Get RDS information" },
	{ .verb = "band_supported",	.session = AFB_SESSION_NONE, .callback = band_supported,	.info = "Check band support" },
	{ .verb = "frequency_range",	.session = AFB_SESSION_NONE, .callback = frequency_range,	.info = "Get frequency range" },
	{ .verb = "frequency_step",	.session = AFB_SESSION_NONE, .callback = frequency_step,	.info = "Get frequency step" },
	{ .verb = "start",		.session = AFB_SESSION_NONE, .callback = start,			.info = "Start radio playback" },
	{ .verb = "stop",		.session = AFB_SESSION_NONE, .callback = stop,			.info = "Stop radio playback" },
	{ .verb = "scan_start",		.session = AFB_SESSION_NONE, .callback = scan_start,		.info = "Start station scan" },
	{ .verb = "scan_stop",		.session = AFB_SESSION_NONE, .callback = scan_stop,		.info = "Stop station scan" },
	{ .verb = "stereo_mode",	.session = AFB_SESSION_NONE, .callback = stereo_mode,		.info = "Get/Set stereo_mode" },
	{ .verb = "subscribe",		.session = AFB_SESSION_NONE, .callback = subscribe,		.info = "Subscribe for an event" },
	{ .verb = "unsubscribe",	.session = AFB_SESSION_NONE, .callback = unsubscribe,		.info = "Unsubscribe for an event" },
	{ }
};

static int init()
{
	int rc;
	char *output = NULL;

#ifdef HAVE_4A_FRAMEWORK
	json_object *response = NULL;
	json_object *jsonData = json_object_new_object();

	json_object_object_add(jsonData, "action", json_object_new_string("open"));
	rc = afb_service_call_sync("ahl-4a", "get_roles", NULL, &response);
	if (rc < 0) {
		AFB_ERROR("Failed to query 4A about roles");
		goto failed;
	}
	AFB_NOTICE("4A: available roles are '%s'", json_object_get_string(response));
	json_object_put(response);

	rc = afb_service_call_sync("ahl-4a", "radio", jsonData, &response);
	if (rc < 0) {
		AFB_ERROR("Failed to query 'radio' role to 4A");
		goto failed;
	}

	json_object *valJson = NULL;
	json_object *val = NULL;

	rc = json_object_object_get_ex(response, "response", &valJson);
	if (rc == 0) {
		AFB_ERROR("Reply from 4A is missing a 'response' field");
		goto failed_malformed;
	}

	rc = json_object_object_get_ex(valJson, "device_uri", &val);
	if (rc == 0) {
		AFB_ERROR("Reply from 4A is missing a 'device_uri' field");
		goto failed_malformed;
	}

	const char *jres_pcm = json_object_get_string(val);
	char * p = strchr(jres_pcm, ':');

	if (p == NULL) {
		AFB_ERROR("Unable to parse 'device_uri' value field");
		rc = -1;
		goto failed_malformed;
	}

	if (asprintf(&output, "hw:%s", p + 1) < 0) {
		AFB_ERROR("Insufficient memory");
		rc = -1;
		goto failed_malformed;
	}
#endif /* HAVE_4A_FRAMEWORK */

	// Initialize event structures
	freq_event = afb_daemon_make_event("frequency");
	scan_event = afb_daemon_make_event("station_found");

	// Look for RTL-SDR USB adapter
	radio_impl_ops = &rtlsdr_impl_ops;
	rc = radio_impl_ops->init(output);
	if(rc != 0) {
		// Look for Kingfisher Si4689
		radio_impl_ops = &kf_impl_ops;
		rc = radio_impl_ops->init(output);
	}
	if (rc != 0) {
		AFB_ERROR("No radio device found, exiting");
		goto failed;
	}

	if(rc == 0) {
		AFB_NOTICE("%s found\n", radio_impl_ops->name);
		radio_impl_ops->set_frequency_callback(freq_callback, NULL);
	}
	free(output);

#ifdef HAVE_4A_FRAMEWORK
failed_malformed:
	json_object_put(response);
#endif /* HAVE_4A_FRAMEWORK */

failed:
	return rc;
}

const struct afb_binding_v2 afbBindingV2 = {
	.info = "radio service",
	.api  = "radio",
	.verbs = verbs,
	.init = init,
};
