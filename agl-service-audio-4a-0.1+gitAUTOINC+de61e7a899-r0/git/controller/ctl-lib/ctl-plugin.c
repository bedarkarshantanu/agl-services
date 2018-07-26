/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Reference:
 *   Json load using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
 */

#define _GNU_SOURCE
#include <string.h>
#include <dlfcn.h>

#include "ctl-config.h"

int PluginGetCB (AFB_ApiT apiHandle, CtlActionT *action , json_object *callbackJ) {
    const char *plugin=NULL, *function=NULL;
    json_object *argsJ;
    int idx;

    if (!ctlPlugins) {
        AFB_ApiError(apiHandle, "PluginGetCB plugin section missing cannot call '%s'", json_object_get_string(callbackJ));
        return 1;
    }

    int err = wrap_json_unpack(callbackJ, "{ss,ss,s?o!}",
        "plugin", &plugin,
        "function", &function,
        "args", &argsJ);
    if (err) {
        AFB_ApiError(apiHandle, "PluginGet missing plugin|function|[args] in %s", json_object_get_string(callbackJ));
        return 1;
    }

    for (idx=0; ctlPlugins[idx].uid != NULL; idx++) {
        if (!strcasecmp (ctlPlugins[idx].uid, plugin)) break;
    }

    if (!ctlPlugins[idx].uid) {
        AFB_ApiError(apiHandle, "PluginGetCB no plugin with uid=%s", plugin);
        return 1;
    }

    action->exec.cb.funcname = function;
    action->exec.cb.callback = dlsym(ctlPlugins[idx].dlHandle, function);
    action->exec.cb.plugin= &ctlPlugins[idx];

    if (!action->exec.cb.callback) {
       AFB_ApiError(apiHandle, "PluginGetCB no plugin=%s no function=%s", plugin, function);
       return 1;
    }
    return 0;
}

// Wrapper to Lua2c plugin command add context and delegate to LuaWrapper
static int DispatchOneL2c(void* luaState, char *funcname, Lua2cFunctionT callback) {
#ifndef CONTROL_SUPPORT_LUA
    fprintf(stderr, "CTL-ONE-L2C: LUA support not selected (cf:CONTROL_SUPPORT_LUA) in config.cmake");
    return 1;
#else
    int err=Lua2cWrapper(luaState, funcname, callback);
    return err;
#endif
}

static int PluginLoadCOne(AFB_ApiT apiHandle, const char *pluginpath, json_object *lua2csJ, const char *lua2c_prefix, void * handle, CtlPluginT *ctlPlugin)
{
    void *dlHandle = dlopen(pluginpath, RTLD_NOW);

    if (!dlHandle) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
        return -1;
    }

    CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*) dlsym(dlHandle, "CtlPluginMagic");
    if (!ctlPluginMagic || ctlPluginMagic->magic != CTL_PLUGIN_MAGIC) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE symbol'CtlPluginMagic' missing or !=  CTL_PLUGIN_MAGIC plugin=%s", pluginpath);
        return -1;
    } else {
        AFB_ApiNotice(apiHandle, "CTL-PLUGIN-LOADONE %s successfully registered", ctlPluginMagic->uid);
    }

    // store dlopen handle to enable onload action at exec time
    ctlPlugin->dlHandle = dlHandle;

#ifndef AFB_BINDING_PREV3
    // Jose hack to make verbosity visible from sharelib with API-V2
    struct afb_binding_data_v2 *afbHidenData = dlsym(dlHandle, "afbBindingV2data");
    if (afbHidenData) *afbHidenData = afbBindingV2data;
#endif

    // Push lua2cWrapper @ into plugin
    Lua2cWrapperT *lua2cInPlug = dlsym(dlHandle, "Lua2cWrap");
#ifndef CONTROL_SUPPORT_LUA
    if (lua2cInPlug) *lua2cInPlug = NULL;
#else
    // Lua2cWrapper is part of binder and not expose to dynamic link
    if (lua2csJ && lua2cInPlug) {
        *lua2cInPlug = (Lua2cWrapperT)DispatchOneL2c;

        int Lua2cAddOne(luaL_Reg *l2cFunc, const char* l2cName, int index) {
            if(ctlPlugin->ctlL2cFunc->l2cCount)
                {index += ctlPlugin->ctlL2cFunc->l2cCount+1;}
            char *funcName;
            size_t p_length = 6 + strlen(l2cName);
            funcName = malloc(p_length + 1);

            strncpy(funcName, "lua2c_", p_length);
            strncat(funcName, l2cName, p_length - strlen (funcName));

            Lua2cFunctionT l2cFunction = (Lua2cFunctionT) dlsym(dlHandle, funcName);
            if (!l2cFunction) {
                AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE symbol'%s' missing err=%s", funcName, dlerror());
                return 1;
            }
            l2cFunc[index].func = (void*) l2cFunction;
            l2cFunc[index].name = strdup(l2cName);

            return 0;
        }

        int count = 0, errCount = 0;
        luaL_Reg *l2cFunc = NULL;
        if(!ctlPlugin->ctlL2cFunc) {
            ctlPlugin->ctlL2cFunc = calloc(1, sizeof(CtlLua2cFuncT));
        }

        ctlPlugin->ctlL2cFunc->prefix = (lua2c_prefix) ?
            lua2c_prefix :
            ctlPlugin->uid;

        // look on l2c command and push them to LUA
        if (json_object_get_type(lua2csJ) == json_type_array) {
            size_t length = json_object_array_length(lua2csJ);
            l2cFunc = calloc(length + ctlPlugin->ctlL2cFunc->l2cCount + 1, sizeof (luaL_Reg));
            for (count = 0; count < length; count++) {
                int err;
                const char *l2cName = json_object_get_string(json_object_array_get_idx(lua2csJ, count));
                err = Lua2cAddOne(l2cFunc, l2cName, count);
                if (err) errCount++;
            }
        } else {
            l2cFunc = calloc(2 + ctlPlugin->ctlL2cFunc->l2cCount, sizeof (luaL_Reg));
            const char *l2cName = json_object_get_string(lua2csJ);
            errCount = Lua2cAddOne(l2cFunc, l2cName, count);
            count++;
        }
        if (errCount) {
            AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE %d symbols not found in plugin='%s'", errCount, pluginpath);
            return -1;
        }
        int total = ctlPlugin->ctlL2cFunc->l2cCount + count;
        if(ctlPlugin->ctlL2cFunc->l2cCount) {
            for (int offset = ctlPlugin->ctlL2cFunc->l2cCount; offset < total; offset++)
            {
                int index = offset - ctlPlugin->ctlL2cFunc->l2cCount;
                l2cFunc[index] = ctlPlugin->ctlL2cFunc->l2cFunc[index];
            }
            free(ctlPlugin->ctlL2cFunc->l2cFunc);
        }
        ctlPlugin->ctlL2cFunc->l2cFunc = l2cFunc;
        ctlPlugin->ctlL2cFunc->l2cCount = total;

        LuaL2cNewLib(ctlPlugin->ctlL2cFunc->l2cFunc, ctlPlugin->ctlL2cFunc->l2cCount, ctlPlugin->ctlL2cFunc->prefix);
    }
#endif
    ctlPlugin->api = apiHandle;
    DispatchPluginInstallCbT ctlPluginOnload = dlsym(dlHandle, "CtlPluginOnload");
    if (ctlPluginOnload) {
        if((*ctlPluginOnload) (ctlPlugin, handle)) {
            AFB_ApiError(apiHandle, "Plugin Onload function hasn't finish well. Abort initialization");
            return -1;
        }
    }

    return 0;
}

static int LoadFoundPlugins(AFB_ApiT apiHandle, json_object *scanResult, json_object *lua2csJ, const char *lua2c_prefix, void *handle, CtlPluginT *ctlPlugin)
{
    char pluginpath[CONTROL_MAXPATH_LEN];
    char *filename;
    char *fullpath;
    char *ext;
    int len;
    json_object *object = NULL;

    pluginpath[CONTROL_MAXPATH_LEN - 1] = '\0';

    if (!json_object_is_type(scanResult, json_type_array))
        return -1;

    len = (int)json_object_array_length(scanResult);

    // TODO/Proposal RFOR: load a plugin after a first fail.
    if(len) {
        object = json_object_array_get_idx(scanResult, 0);
        int err = wrap_json_unpack(object, "{s:s, s:s !}",
                "fullpath", &fullpath,
                "filename", &filename);

        if (err) {
            AFB_ApiError(apiHandle, "HOOPs invalid plugin file path=\n-- %s", json_object_get_string(scanResult));
            return -1;
        }

        ext = strrchr(filename, '.');
        strncpy(pluginpath, fullpath, CONTROL_MAXPATH_LEN - 1);
        strncat(pluginpath, "/", CONTROL_MAXPATH_LEN - strlen(pluginpath) - 1);
        strncat(pluginpath, filename, CONTROL_MAXPATH_LEN - strlen (pluginpath) - 1);

        if(ext && !strcasecmp(ext, CTL_PLUGIN_EXT) && PluginLoadCOne(apiHandle, pluginpath, lua2csJ, lua2c_prefix, handle, ctlPlugin)) {
            return -1;
        }
        else if(ext && !strcasecmp(ext, CTL_SCRIPT_EXT)) {
            ctlPlugin->api = apiHandle;
            ctlPlugin->context = handle;
            if(LuaLoadScript(pluginpath)) {
                AFB_ApiError(apiHandle, "There was an error loading the lua file %s", pluginpath);
                return -1;
            }
        }
    }

    if(len > 1)
        AFB_ApiWarning(apiHandle, "Plugin multiple instances in searchpath will use %s/%s", fullpath, filename);

    return 0;
}

static char *GetDefaultSearchPath(AFB_ApiT apiHandle)
{
    char *searchPath;
    const char *bindingPath;
    const char *envPath;
    size_t bindingPath_len, envPath_len, CTL_PLGN_len;

    bindingPath = GetBindingDirPath(apiHandle);
    envPath = getenv("CONTROL_PLUGIN_PATH");
    bindingPath_len = strlen(bindingPath);
    envPath_len = envPath ? strlen(envPath) : 0;
    CTL_PLGN_len = envPath_len ? 0 : strlen(CONTROL_PLUGIN_PATH);

    /* Allocating with the size of binding root dir path + environment if found
     * + 2 for the NULL terminating character and the additional separator
     * between bindingPath and envPath concatenation.
     */
    if(envPath)  {
        searchPath = calloc(1, sizeof(char) *((bindingPath_len + envPath_len) + 2));
        strncat(searchPath, envPath, envPath_len);
    }
    else {
        searchPath = calloc(1, sizeof(char) * ((bindingPath_len + CTL_PLGN_len) + 2));
        strncat(searchPath, CONTROL_PLUGIN_PATH, CTL_PLGN_len);
    }

    strncat(searchPath, ":", sizeof(searchPath) - 1);
    strncat(searchPath, bindingPath, bindingPath_len);

    return searchPath;
}

static int FindPlugins(AFB_ApiT apiHandle, const char *searchPath, const char *file, json_object **pluginPathJ)
{
    *pluginPathJ = ScanForConfig(searchPath, CTL_SCAN_RECURSIVE, file, NULL);
    if (!*pluginPathJ || json_object_array_length(*pluginPathJ) == 0) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Missing plugin=%s* (config ldpath?) search=\n-- %s", file, searchPath);
        return -1;
    }

    return 0;
}

static int PluginLoad (AFB_ApiT apiHandle, CtlPluginT *ctlPlugin, json_object *pluginJ, void *handle)
{
    int err = 0, i = 0;
    char *searchPath;
    const char *sPath = NULL, *file = NULL, *lua2c_prefix = NULL;
    json_object *luaJ = NULL, *lua2csJ = NULL, *fileJ = NULL, *pluginPathJ = NULL;

    // plugin initialises at 1st load further init actions should be place into onload section
    if (!pluginJ) return 0;

    err = wrap_json_unpack(pluginJ, "{ss,s?s,s?s,s?o,s?o !}",
            "uid", &ctlPlugin->uid,
            "info", &ctlPlugin->info,
            "spath", &sPath,
            "libs", &fileJ,
            "lua", &luaJ
            );
    if (err) {
        AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Plugin missing uid|[info]|libs|[spath]|[lua] in:\n-- %s", json_object_get_string(pluginJ));
        return 1;
    }

    if(luaJ) {
        err = wrap_json_unpack(luaJ, "{ss,s?o !}",
            "prefix", &lua2c_prefix,
            "functions", &lua2csJ);
        if(err) {
            AFB_ApiError(apiHandle, "CTL-PLUGIN-LOADONE Missing 'function' in:\n-- %s", json_object_get_string(pluginJ));
            return 1;
        }
    }

    // if search path not in Json config file, then try default
    searchPath = (sPath) ?
        strdup(sPath) :
        GetDefaultSearchPath(apiHandle);

    // default file equal uid
    if (!fileJ) {
        file = ctlPlugin->uid;
        if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
            free(searchPath);
            if(pluginPathJ)
                json_object_put(pluginPathJ); // No more needs for that json_object.
            return 1;
        }
        LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
    }
    else if(json_object_is_type(fileJ, json_type_string)) {
        file = json_object_get_string(fileJ);
        if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
            free(searchPath);
            json_object_put(pluginPathJ); // No more needs for that json_object.
            return 1;
        }
        LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
    }
    else if(json_object_is_type(fileJ, json_type_array)) {
        for(i = 0; i < json_object_array_length(fileJ);++i) {
            file = json_object_get_string(json_object_array_get_idx(fileJ, i));
            if(FindPlugins(apiHandle, searchPath, file, &pluginPathJ)) {
                free(searchPath);
                json_object_put(pluginPathJ); // No more needs for that json_object.
                return 1;
            }
            LoadFoundPlugins(apiHandle, pluginPathJ, lua2csJ, lua2c_prefix, handle, ctlPlugin);
        }
    }

    if(err) {
        free(searchPath);
        json_object_put(pluginPathJ); // No more needs for that json_object.
        return 1;
    }

    free(searchPath);
    json_object_put(pluginPathJ); // No more needs for that json_object.
    return 0;
}

static int PluginParse(AFB_ApiT apiHandle, CtlSectionT *section, json_object *pluginsJ, int *pluginNb) {
    int idx = 0, err = 0;

    switch (json_object_get_type(pluginsJ)) {
        case json_type_array: {
            *pluginNb = (int)json_object_array_length(pluginsJ);
            ctlPlugins = calloc (*pluginNb + 1, sizeof(CtlPluginT));
            for (idx=0; idx < *pluginNb; idx++) {
                json_object *pluginJ = json_object_array_get_idx(pluginsJ, idx);
                err += PluginLoad(apiHandle, &ctlPlugins[idx], pluginJ, section->handle);
            }
            break;
        }
        case json_type_object: {
            ctlPlugins = calloc (2, sizeof(CtlPluginT));
            err += PluginLoad(apiHandle, &ctlPlugins[0], pluginsJ, section->handle);
            (*pluginNb)++;
            break;
        }
        default: {
            AFB_ApiError(apiHandle, "Wrong JSON object passed: %s", json_object_get_string(pluginsJ));
            err = -1;
        }
    }

        return err;
}

int PluginConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *pluginsJ) {
    int err = 0;
    int idx = 0, jdx = 0;
    int pluginNb = 0, newPluginsNb = 0, totalPluginNb = 0;

    if (ctlPlugins)
    {
        // There is something to add let  that happens
        if(pluginsJ) {
            CtlPluginT *ctlPluginsNew = NULL, *ctlPluginsOrig = ctlPlugins;
            err = PluginParse(apiHandle, section, pluginsJ, &newPluginsNb);
            ctlPluginsNew = ctlPlugins;

            while(ctlPlugins[pluginNb].uid != NULL) {
                pluginNb++;
            }

            totalPluginNb = pluginNb + newPluginsNb;
            ctlPlugins = calloc(totalPluginNb + 1, sizeof(CtlPluginT));
            while(ctlPluginsOrig[idx].uid != NULL) {
                ctlPlugins[idx] = ctlPluginsOrig[idx];
                idx++;
            }
            while(ctlPluginsNew[jdx].uid != NULL && idx <= totalPluginNb) {
                ctlPlugins[idx] = ctlPluginsNew[jdx];
                idx++;
                jdx++;
            }

            free(ctlPluginsOrig);
            free(ctlPluginsNew);
        }

        while(ctlPlugins[idx].uid != NULL)
        {
            // Jose hack to make verbosity visible from sharedlib and
            // be able to call verb from others api inside the binder
            struct afb_binding_data_v2 *afbHidenData = dlsym(ctlPlugins[idx++].dlHandle, "afbBindingV2data");
            if (afbHidenData) *afbHidenData = afbBindingV2data;
        }
        return 0;
    }
    else
    {
        err = PluginParse(apiHandle, section, pluginsJ, &pluginNb);
    }

    return err;
}
