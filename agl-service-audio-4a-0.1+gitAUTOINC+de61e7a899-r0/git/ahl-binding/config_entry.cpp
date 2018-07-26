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

#include "config_entry.hpp"

config_entry_t::config_entry_t(std::string fp, std::string fn)
    : fullpath_{fp}
    , filename_{fn}
{
}

config_entry_t::config_entry_t(struct json_object* j)
{
    jcast(fullpath_, j, "fullpath");
    jcast(filename_, j, "filename");
}

config_entry_t& config_entry_t::operator<<(struct json_object* j)
{
    jcast(fullpath_, j, "fullpath");
    jcast(filename_, j, "filename");
    return *this;
}

std::string config_entry_t::fullpath() const
{
    return fullpath_;
}

std::string config_entry_t::filename() const
{
    return filename_;
}

std::string config_entry_t::filepath() const
{
    return fullpath_ + '/' + filename_;
}

void config_entry_t::fullpath(std::string fp)
{
    fullpath_ = fp;
}

void config_entry_t::finename(std::string fn)
{
    filename_ = fn;
}
