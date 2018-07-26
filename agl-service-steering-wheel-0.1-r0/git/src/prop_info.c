/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
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
#include <string.h>
#include <stdint.h>

#include "prop_info.h"
#include "af-steering-wheel-binding.h"

#define ENABLE1_TYPENAME "ENABLE-1"	/* spec. type1.json original type name */

static const char *vartype_strings[] = {
/* 0 */	"void",
/* 1 */	"int8_t",
/* 2 */	"int16_t",
/* 3 */	"int",
/* 4 */	"int32_t",
/* 5 */	"int64_t",
/* 6 */	"uint8_t",
/* 7 */	"uint16_t",
/* 8 */	"uint",
/* 9 */	"uint32_t",
/* 10 */"uint64_t",
/* 11 */	"string",
/* 12 */	"bool",
/* 13 */	"LIST",
/* 14 */ ENABLE1_TYPENAME
};

int string2vartype(const char *varname)
{
	unsigned int i;
	if (varname == NULL)
	{
		return -1;
	}

	for(i=0; i < (sizeof(vartype_strings)/sizeof(char *)); i++)
	{
		if (strcmp(varname, vartype_strings[i]) == 0)
			return (int)i;
	}

	return -1;
};

int propertyValue_int(struct prop_info_t *property_info)
{
	int x = -1;
	switch(property_info->var_type) {
	case INT8_T:
		x = (int)property_info->curValue.int8_val;
		break;
	case INT16_T:
		x =	(int)property_info->curValue.int16_val;
		break;
	case VOID_T:/* FALLTHROGH */
	case INT_T:	/* FALLTHROGH */
	case INT32_T:/* FALLTHROGH */
		x =	(int)property_info->curValue.int32_val;
		break;
	case UINT8_T:
		x =	(int)property_info->curValue.uint8_val;
		break;
	case UINT16_T:
		x =	(int)property_info->curValue.uint16_val;
		break;
	case UINT_T:
	case UINT32_T:
		x = (int)property_info->curValue.uint32_val;
		break;
	case BOOL_T:
		x = (int)property_info->curValue.bool_val;
		break;
	case ENABLE1_T:
		x = (int)property_info->curValue.int32_val;
		break;

	default:
	case STRING_T:
	case ARRAY_T:
	case UINT64_T:
	case INT64_T:
		ERRMSG("Getting property Value:NOT SUPPORT vartype contents:%s %d", property_info->name, property_info->var_type);
		break;
	}
	return x;
}
