/*
 * Copyright (C) 2015, 2016, 2017 "IoT.bzh"
 * Author: José Bollo <jose.bollo@iot.bzh>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <systemd/sd-bus.h>

#include "aia-uds-bluez.h"


void p(const char *tag, const struct aia_uds_value *value)
{
	printf("%10s %c %.*s\n", tag, value->changed?'*':' ', (int)value->length, value->data);
}

void onchg(const struct aia_uds *uds)
{
	printf("\n");
	p("first name", &uds->first_name);
	p("last name", &uds->last_name);
	p("email", &uds->email);
	p("language", &uds->language);
}

int main()
{
	sd_bus *bus;
	sd_id128_t id;
	int rc;

	aia_uds_set_on_change(onchg);
	rc = sd_bus_default_system(&bus);
	rc = sd_id128_randomize(&id);
	rc = sd_bus_set_server(bus, 1, id);
	rc = sd_bus_start(bus);
	rc = aia_uds_init(bus);
	rc = aia_uds_advise(1, NULL, NULL);
	for (;;) {
		rc = sd_bus_process(bus, NULL);
		if (rc < 0)
			break;
		else if (rc == 0)
			rc = sd_bus_wait(bus, (uint64_t) -1);
	}
}

