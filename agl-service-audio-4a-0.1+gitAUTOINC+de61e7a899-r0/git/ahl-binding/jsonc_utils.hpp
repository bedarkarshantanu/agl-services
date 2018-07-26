#pragma once

/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author Loïc Collignon <loic.collignon@iot.bzh>
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

#include <string>
#include <vector>
#include <json-c/json.h>

template<class T>
inline T& jcast(T& v, json_object* o)
{
    v << o;
    return v;
}

template<class T>
inline T& jcast_array(T& v, json_object* o)
{
    if (o == nullptr) return v;
    auto sz = json_object_array_length(o);
    for(auto i = 0; i < sz ; ++i)
    {
        typename T::value_type item;
        jcast(item, json_object_array_get_idx(o, i));
        v.push_back(item);
    }
    return v;
}

template<>
inline bool& jcast<bool>(bool& v, json_object* o)
{
    v = (json_object_get_boolean(o) == TRUE);
    return v;
}

template<>
inline double& jcast<double>(double& v, json_object* o)
{
    v = json_object_get_double(o);
    return v;
}

template<>
inline int32_t& jcast<int32_t>(int32_t& v, json_object* o)
{
    v = json_object_get_int(o);
    return v;
}

template<>
inline int64_t& jcast<int64_t>(int64_t& v, json_object* o)
{
    v = json_object_get_int(o);
    return v;
}

template<>
inline std::string& jcast<std::string>(std::string& v, json_object* o)
{
    const char* tmp = json_object_get_string(o);
    v = tmp ? tmp : "";
    return v;
}

template<class T>
inline T& jcast(T& v, json_object* o, std::string field)
{
	json_object * value;
	json_object_object_get_ex(o, field.c_str(), &value);
    return jcast<T>(v, value);
}

template<class T>
inline T& jcast_array(T& v, json_object* o, std::string field)
{
	json_object * value;
	json_object_object_get_ex(o, field.c_str(), &value);
    return jcast_array<T>(v, value);
}
