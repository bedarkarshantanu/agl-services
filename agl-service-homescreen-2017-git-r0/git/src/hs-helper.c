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

#include "hs-helper.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <json-c/json.h>
#include <stdarg.h>

REQ_ERROR get_value_uint16(const struct afb_req request, const char *source, uint16_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > UINT16_MAX) || (tmp_id < 0) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (uint16_t)tmp_id;
    return REQ_OK;
}

REQ_ERROR get_value_int16(const struct afb_req request, const char *source, int16_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > INT16_MAX) || (tmp_id < INT16_MIN) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (int16_t)tmp_id;
    return REQ_OK;
}

REQ_ERROR get_value_int32(const struct afb_req request, const char *source, int32_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > INT32_MAX) || (tmp_id < INT32_MIN) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (int32_t)tmp_id;
    return REQ_OK;
}

void hs_add_object_to_json_object(struct json_object* j_obj, int count,...)
{
    va_list args;
    va_start(args, count);
    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        int value = va_arg(args, int);
        json_object_object_add(j_obj, key, json_object_new_int((int32_t)value));
        ++i;
    }
    va_end(args);
}

void hs_add_object_to_json_object_str(struct json_object* j_obj, int count,...)
{
    va_list args;
    va_start(args, count);
    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        char *value = va_arg(args, char*);
        json_object_object_add(j_obj, key, json_object_new_string(value));
        ++i;
    }
    va_end(args);
}


void hs_add_object_to_json_object_func(struct json_object* j_obj, const char* verb_name, int count, ...)
{
    va_list args;
    va_start(args, count);

    json_object_object_add(j_obj,"verb", json_object_new_string(verb_name));

    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        int value = va_arg(args, int);
        json_object_object_add(j_obj, key, json_object_new_int((int32_t)value));
        ++i;
    }
    va_end(args);
}

int hs_search_event_name_index(const char* value)
{
    size_t buf_size = 50;
    size_t size = sizeof evlist / sizeof *evlist;
    int ret = -1;
    for(size_t i = 0 ; i < size ; ++i)
    {
        if(!strncmp(value, evlist[i], buf_size))
        {
            ret = i;
            break;
        }
    }
    return ret;
}
