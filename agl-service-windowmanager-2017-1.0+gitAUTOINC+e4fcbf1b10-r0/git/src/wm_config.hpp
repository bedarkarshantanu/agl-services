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

#ifndef WM_CONFIG_HPP
#define WM_CONFIG_HPP
#include <string>
#include "wm_error.hpp"

struct json_object;

namespace wm
{

class WMConfig {
public:
    WMConfig();
    ~WMConfig();
    WMConfig(const WMConfig &) = delete;
    WMConfig &operator=(const WMConfig &) = delete;
    WMError loadConfigs();
    const std::string getConfigAspect();

  private:
    WMError loadSetting(const std::string &path);
    // private variable
    json_object *j_setting_conf;
};

} // namespace wm
#endif // WM_CONFIG_HPP