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

#include <stdlib.h>
#include <stdio.h>
#include "afm-nfc-common.h"

char *to_hex_string(unsigned char *data, size_t size)
{
	char *buffer = malloc((2 * size) + 1);
	char *tmp = buffer;
	int i;

	if (buffer == NULL)
		return buffer;

	for (i = 0; i < size; i++) {
		tmp += sprintf(tmp, "%.2x", data[i]);
	}

	return buffer;
}
