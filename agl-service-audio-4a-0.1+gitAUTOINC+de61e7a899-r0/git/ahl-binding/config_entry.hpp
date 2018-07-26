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

#include "jsonc_utils.hpp"

class config_entry_t
{
private:
	std::string fullpath_;
	std::string filename_;
	
public:
	explicit config_entry_t() = default;
	explicit config_entry_t(const config_entry_t&) = default;
    explicit config_entry_t(config_entry_t&&) = default;
	
	~config_entry_t() = default;
	
	config_entry_t& operator=(const config_entry_t&) = default;
    config_entry_t& operator=(config_entry_t&&) = default;
	
	explicit config_entry_t(std::string fp, std::string fn);
	explicit config_entry_t(struct json_object* j);
	config_entry_t& operator<<(struct json_object* j);
	
	std::string fullpath() const;
	std::string filename() const;
	std::string filepath() const;
	
	void fullpath(std::string fp);
	void finename(std::string fn);
	
};
