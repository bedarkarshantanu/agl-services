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
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "af-steering-wheel-binding.h"
#include "steering_wheel_json.h"
#include "prop_info.h"
#include "prop_search.h"

extern double gearRatio[];

struct wheel_info_t *wheel_info;

static int parse_property(struct wheel_info_t *wheel_info, int idx, json_object *obj_property)
{
	int var_type = 0;
	char *name = NULL;

	if(obj_property)
	{
		json_object_object_foreach(obj_property, key, val)
		{
			if (strcmp("PROPERTY", key) == 0)
			{
				name = (char *)json_object_get_string(val);
			}
			else if(strcmp("TYPE", key) == 0)
			{
				const char *_varname = json_object_get_string(val);
				var_type = string2vartype(_varname);
			}
		}

		wheel_info->property[idx].name = strdup(name);
		wheel_info->property[idx].var_type = (unsigned char)var_type;
	}

	return 0;
}

#define DEFAULT_PROP_CNT (16)
static int parse_propertys(json_object *obj_propertys)
{
	int err = 0;
	json_object * obj_property;
	size_t ms = sizeof(struct wheel_info_t) + (size_t)(DEFAULT_PROP_CNT*sizeof(struct prop_info_t));

	if(obj_propertys)
	{
		wheel_info = malloc(ms);
		if (wheel_info == NULL)
		{
			ERRMSG("Not enogh memory");
			return 1;
		}
		memset(wheel_info, 0 , ms);

		enum json_type type = json_object_get_type(obj_propertys);
		if (type == json_type_array)
		{
			int array_len = json_object_array_length(obj_propertys);
			if(array_len > DEFAULT_PROP_CNT)
			{
				wheel_info = realloc(wheel_info, sizeof(struct wheel_info_t) + ((size_t)array_len * sizeof(struct prop_info_t)));
				if (wheel_info == NULL)
				{
					ERRMSG("not enogh memory");
					exit(1);
				}

				memset(&wheel_info->property[DEFAULT_PROP_CNT], 0, (size_t)(array_len - DEFAULT_PROP_CNT)*sizeof(struct prop_info_t));
			}

			for (int i = 0; i < array_len; i++)
			{
				obj_property = json_object_array_get_idx(obj_propertys, i);
				err = parse_property(wheel_info, i, obj_property);
			}

			wheel_info->nData = (unsigned int)array_len;
			addAllPropDict(wheel_info);
		}
	}

	return err;
}

static int parse_json(json_object *obj)
{
	int err = 0;
    json_object_object_foreach(obj, key, val)
	{
        if (strcmp(key,"PROPERTYS") == 0)
        {
			err += parse_propertys(val);
		}
        else
        {
			++err;
			ERRMSG("json: Unknown  key \"%s\"", key);
		}
    }
	return err;
}

int wheel_define_init(const char *fname)
{
	struct json_object *jobj;
	int fd_wheel_map;
	struct stat stbuf;
	char *filebuf;

	fd_wheel_map = afb_daemon_rootdir_open_locale(fname, O_RDONLY, NULL);
	if (fd_wheel_map < 0)
	{
		ERRMSG("wheel map is not access");
		return -1;
	}

	FILE *fp = fdopen(fd_wheel_map,"r");
	if (fp == NULL)
	{
		ERRMSG("canno read wheel map file");
		return -1;
	}

	if (fstat(fd_wheel_map, &stbuf) == -1)
	{
		ERRMSG("cant get file state");
		return -1;
	}

	filebuf = (char *)malloc(stbuf.st_size);
	fread(filebuf, 1, stbuf.st_size, fp);
	fclose(fp);

	jobj = json_tokener_parse(filebuf);
	if (jobj == NULL)
	{
		ERRMSG("cannot data from file \"%s\"",fname);
		free(filebuf);
		return 1;
	}

	parse_json(jobj);

	json_object_put(jobj);

	free(filebuf);
	return 0;
}

static int parse_gear_para_json(json_object *obj)
{
	int err = 0;
	json_object * obj_speed_para;
	char *pos_name = NULL;
	double pos_val = 0;

    json_object_object_foreach(obj, key, val)
	{
        if (strcmp(key,"GEAR_PARA") == 0)
        {
        	enum json_type type = json_object_get_type(val);
			if (type == json_type_array)
			{
				int array_len = json_object_array_length(val);
				NOTICEMSG("array_len:%d!", array_len);
				for (int i = 0; i < array_len; i++)
				{
					obj_speed_para = json_object_array_get_idx(val, i);
					if(obj_speed_para)
					{
						json_object_object_foreach(obj_speed_para, key, val)
						{
							///
							if (strcmp("POS", key) == 0)
							{
								pos_name = (char *)json_object_get_string(val);
							}
							else if(strcmp("VAL", key) == 0)
							{
								pos_val = json_object_get_double(val);
							}

							///
							if(strcmp("First", pos_name) == 0)
							{
								gearRatio[1] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Second", pos_name) == 0)
							{
								gearRatio[2] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Third", pos_name) == 0)
							{
								gearRatio[3] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Fourth", pos_name) == 0)
							{
								gearRatio[4] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Fifth", pos_name) == 0)
							{
								gearRatio[5] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Sixth", pos_name) == 0)
							{
								gearRatio[6] = (double)(1.0 / pos_val);
							}
							else if(strcmp("Reverse", pos_name) == 0)
							{
								gearRatio[7] = (double)(1.0 / pos_val);
							}
						}
					}
				}
			}
			else
			{
				ERRMSG("json: Need  array \"%s\"", key);
			}
		}
        else
        {
			ERRMSG("json: Unknown  key \"%s\"", key);
		}
    }
	return err;
}

int wheel_gear_para_init(const char *fname)
{
	struct json_object *jobj;
	int fd_gear_para;
	struct stat stbuf;
	char *filebuf;

	fd_gear_para = afb_daemon_rootdir_open_locale(fname, O_RDONLY, NULL);
	if (fd_gear_para < 0)
	{
		ERRMSG("gear para is not access");
		return -1;
	}

	FILE *fp = fdopen(fd_gear_para,"r");
	if (fp == NULL)
	{
		ERRMSG("canno read gear para file");
		return -1;
	}

	if (fstat(fd_gear_para, &stbuf) == -1)
	{
		ERRMSG("cant get file state.\n");
		return -1;
	}

	filebuf = (char*)malloc(stbuf.st_size);
	fread(filebuf, 1, stbuf.st_size, fp);
	fclose(fp);

	jobj = json_tokener_parse(filebuf);
	if (jobj == NULL)
	{
		ERRMSG("cannot data from file \"%s\"",fname);
		free(filebuf);
		return 1;
	}
	parse_gear_para_json(jobj);

	json_object_put(jobj);

	free(filebuf);
	return 0;
}

struct json_object *property2json(struct prop_info_t *property)
{
	struct json_object *jobj;
	struct json_object *nameObject = NULL;
	struct json_object *valueObject = NULL;

	if (property == NULL)
		return NULL;

	jobj = json_object_new_object();
	if (jobj == NULL)
		return NULL;
	switch(property->var_type)
	{
		case VOID_T:	/* FALLTHROUGH */
		case INT8_T:	/* FALLTHROUGH */
		case INT16_T:	/* FALLTHROUGH */
		case INT_T:		/* FALLTHROUGH */
		case INT32_T:	/* FALLTHROUGH */
		case INT64_T:	/* FALLTHROUGH */
		case UINT8_T:	/* FALLTHROUGH */
		case UINT16_T:	/* FALLTHROUGH */
		case UINT_T:	/* FALLTHROUGH */
		case UINT32_T:	/* FALLTHROUGH */
		case ENABLE1_T:
			valueObject = json_object_new_int(propertyValue_int(property));
			break;

		case BOOL_T:
			valueObject = json_object_new_boolean(property->curValue.bool_val);
			break;

		default:
			ERRMSG("Unknown value type:%d", property->var_type);
			break;
	}

	if (valueObject == NULL)
	{
		ERRMSG("fail json ValueObject");
		json_object_put(jobj);
		return NULL;
	}

	nameObject = json_object_new_string(property->name);

	json_object_object_add(jobj, "name", nameObject);
	json_object_object_add(jobj, "value", valueObject);

	return jobj;
}
