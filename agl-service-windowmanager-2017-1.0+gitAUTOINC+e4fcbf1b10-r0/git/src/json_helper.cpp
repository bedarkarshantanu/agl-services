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

#include "json_helper.hpp"
#include "hmi-debug.h"

#include <json.h>

json_object *to_json(compositor::surface_properties const &s)
{
    // auto j = json::object({
    auto j = json_object_new_object();

    //    {"id", s.id},
    json_object_object_add(j, "id", json_object_new_int(s.id));

    //    {"size", {{"width", s.size.w}, {"height", s.size.h}}},
    auto jsize = json_object_new_object();
    json_object_object_add(jsize, "width", json_object_new_int(s.size.w));
    json_object_object_add(jsize, "height", json_object_new_int(s.size.h));
    json_object_object_add(j, "size", jsize);

    //    {"dst",
    //     {{"width", s.dst_rect.w},
    //      {"height", s.dst_rect.h},
    //      {"x", s.dst_rect.x},
    //      {"y", s.dst_rect.y}}},
    auto jdst = json_object_new_object();
    json_object_object_add(jdst, "width", json_object_new_int(s.dst_rect.w));
    json_object_object_add(jdst, "height", json_object_new_int(s.dst_rect.h));
    json_object_object_add(jdst, "x", json_object_new_int(s.dst_rect.x));
    json_object_object_add(jdst, "y", json_object_new_int(s.dst_rect.y));
    json_object_object_add(j, "dst", jdst);

    //    {"src",
    //     {{"width", s.src_rect.w},
    //      {"height", s.src_rect.h},
    //      {"x", s.src_rect.x},
    //      {"y", s.src_rect.y}}},
    auto jsrc = json_object_new_object();
    json_object_object_add(jsrc, "width", json_object_new_int(s.src_rect.w));
    json_object_object_add(jsrc, "height", json_object_new_int(s.src_rect.h));
    json_object_object_add(jsrc, "x", json_object_new_int(s.src_rect.x));
    json_object_object_add(jsrc, "y", json_object_new_int(s.src_rect.y));
    json_object_object_add(j, "src", jsrc);

    //    {"visibility", s.visibility},
    json_object_object_add(
        j, "visibility",
        json_object_new_boolean(static_cast<json_bool>(s.visibility == 1)));

    //    {"opacity", s.opacity},
    json_object_object_add(j, "opacity", json_object_new_double(s.opacity));

    //    {"orientation", s.orientation},
    json_object_object_add(j, "orientation", json_object_new_int(s.orientation));

    // });
    return j;
}

json_object *to_json(compositor::screen const *s)
{
    auto o = json_object_new_object();
    json_object_object_add(o, "id", json_object_new_int(s->id));
    return o;
}

template <typename T>
json_object *to_json_(T const &s)
{
    auto a = json_object_new_array();

    if (!s.empty())
    {
        for (auto const &i : s)
        {
            json_object_array_add(a, to_json(i.second));
        }
    }

    return a;
}

json_object *to_json(compositor::controller::props_map const &s)
{
    return to_json_(s);
}

json_object *to_json(std::vector<uint32_t> const &v)
{
    auto a = json_object_new_array();
    for (const auto i : v)
    {
        json_object_array_add(a, json_object_new_int(i));
    }
    return a;
}

namespace jh {

const char* getStringFromJson(json_object* obj, const char* key)
{
    json_object* tmp;
    if (!json_object_object_get_ex(obj, key, &tmp))
    {
        HMI_DEBUG("wm:jh", "Not found key \"%s\"", key);
        return nullptr;
    }

    return json_object_get_string(tmp);
}

int getIntFromJson(json_object *obj, const char *key)
{
    json_object *tmp;
    if (!json_object_object_get_ex(obj, key, &tmp))
    {
        HMI_DEBUG("wm:jh", "Not found key \"%s\"", key);
        return 0;
    }

    return json_object_get_int(tmp);
}

int inputJsonFilie(const char* file, json_object** obj)
{
    const int input_size = 128;
    int ret = -1;

    HMI_DEBUG("wm:jh", "Input file: %s", file);

    // Open json file
    FILE *fp = fopen(file, "rb");
    if (nullptr == fp)
    {
        HMI_ERROR("wm:jh", "Could not open file");
        return ret;
    }

    // Parse file data
    struct json_tokener *tokener = json_tokener_new();
    enum json_tokener_error json_error;
    char buffer[input_size];
    int block_cnt = 1;
    while (1)
    {
        size_t len = fread(buffer, sizeof(char), input_size, fp);
        *obj = json_tokener_parse_ex(tokener, buffer, len);
        if (nullptr != *obj)
        {
            HMI_DEBUG("wm:jh", "File input is success");
            ret = 0;
            break;
        }

        json_error = json_tokener_get_error(tokener);
        if ((json_tokener_continue != json_error)
            || (input_size > len))
        {
            HMI_ERROR("wm:jh", "Failed to parse file (byte:%d err:%s)",
                      (input_size * block_cnt), json_tokener_error_desc(json_error));
            HMI_ERROR("wm:jh", "\n%s", buffer);
            *obj = nullptr;
            break;
        }
        block_cnt++;
    }

    // Close json file
    fclose(fp);

    // Free json_tokener
    json_tokener_free(tokener);

    return ret;
}

} // namespace jh
