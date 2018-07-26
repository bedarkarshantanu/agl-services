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

#include "../include/hmi-debug.h"
#include "json_helper.hpp"
#include "wm_config.hpp"

using std::string;

namespace wm
{

WMConfig::WMConfig(){}

WMConfig::~WMConfig()
{
    json_object_put(j_setting_conf);
}

WMError WMConfig::loadConfigs()
{
    WMError ret;
    char const *pRoot = getenv("AFM_APP_INSTALL_DIR");
    string root_path = pRoot;
    if (root_path.length() == 0)
    {
        HMI_ERROR("wm", "AFM_APP_INSTALL_DIR is not defined");
    }
    ret = this->loadSetting(root_path);

    return ret;
}

const string WMConfig::getConfigAspect()
{
    json_object *j;
    string ret;
    if (!json_object_object_get_ex(this->j_setting_conf, "settings", &j))
    {
        ret = "aspect_fit";
    }
    else
    {
        const char* scaling = jh::getStringFromJson(j, "scaling");
        if(scaling == nullptr)
        {
            ret = "aspect_fit";
        }
        else
        {
            ret = scaling;
        }
    }
    return ret;
}

/*
 ***** Private Functions *****
 */

WMError WMConfig::loadSetting(const string &path)
{
    string setting_path = path;
    int iret = -1;
    WMError ret = SUCCESS;

    if (setting_path.length() != 0)
    {
        setting_path = path + string("/etc/setting.json");
        iret = jh::inputJsonFilie(setting_path.c_str(), &this->j_setting_conf);
        if (iret < 0)
            ret = FAIL;
    }

    return ret;
}

} // namespace wm
