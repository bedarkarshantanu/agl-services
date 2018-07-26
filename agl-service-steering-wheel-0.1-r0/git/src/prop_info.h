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

#ifndef _CANID_INFO_H_
#define _CANID_INFO_H_

#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>

enum var_type_t {
/* 0 */	VOID_T,
/* 1 */	INT8_T,
/* 2 */	INT16_T,
/* 3 */	INT_T,
/* 4 */	INT32_T,
/* 5 */	INT64_T,
/* 6 */	UINT8_T,
/* 7 */	UINT16_T,
/* 8 */	UINT_T,
/* 9 */	UINT32_T,
/* 10 */	UINT64_T,
/* 11 */	STRING_T,
/* 12 */	BOOL_T,
/* 13 */	ARRAY_T,
/* 14 */	ENABLE1_T	/* AMB#CANRawPlugin original type */
};

union data_content_t	{
	uint32_t uint32_val;
	int32_t  int32_val;
	uint16_t uint16_val;
	int16_t  int16_val;
	uint8_t  uint8_val;
	int8_t   int8_val;
	int8_t     bool_val;
};

struct prop_info_t {
	const char * name;
	unsigned char var_type;
	union data_content_t curValue;

	struct prop_info_t *next;
};

struct wheel_info_t {
	unsigned int   nData;
	struct prop_info_t property[0];
	/* This is variable structure */
};

extern const char * vartype2string(unsigned int t);
extern int string2vartype(const char *varname);
extern int propertyValue_int(struct prop_info_t *property_info);

#endif
