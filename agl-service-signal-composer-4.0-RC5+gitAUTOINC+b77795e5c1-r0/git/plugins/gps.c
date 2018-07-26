/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Romain Forlot <romain.forlot@iot.bzh>
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
 *
*/

#define _GNU_SOURCE  // needed for vasprintf

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <systemd/sd-event.h>
#include <json-c/json_object.h>
#include <stdbool.h>
#include <string.h>

#include "ctl-plugin.h"
#include "wrap-json.h"

CTLP_CAPI_REGISTER("gps");

// Call at initialisation time
/*CTLP_ONLOAD(plugin, api) {
	AFB_NOTICE ("GPS plugin: uid='%s' 'info='%s'", plugin->uid, plugin->info);
	return api;
}*/

CTLP_CAPI (subscribeToLow, source, argsJ, eventJ) {
	json_object* subscribeArgsJ = NULL, *responseJ = NULL;

	int err = 0;
	wrap_json_pack(&subscribeArgsJ, "{ss}", "value", "location");

	AFB_DEBUG("Calling subscribe with %s", json_object_to_json_string_ext(subscribeArgsJ, JSON_C_TO_STRING_PRETTY));
	err = afb_service_call_sync("gps", "subscribe", subscribeArgsJ, &responseJ);
	if(err)
	{
		AFB_ERROR("Can't find api 'gpsd'");
		return err;
	}

	return err;
}
