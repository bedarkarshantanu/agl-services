/*
 * Copyright (C) 2018 Konsulko Group
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
 *
 */

#ifndef AFM_NFC_COMMON_H
#define AFM_NFC_COMMON_H

#include <glib.h>
#include <json-c/json.h>
#include <nfc/nfc-types.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

char *to_hex_string(unsigned char *data, size_t size);


typedef struct {
	nfc_context *ctx;
	nfc_device *dev;

	gchar *adapter;
	GMainLoop *loop;

	/* cached JSON event response */
	json_object *jresp;
} nfc_binding_data;

#endif // AFM_NFC_COMMON_H
