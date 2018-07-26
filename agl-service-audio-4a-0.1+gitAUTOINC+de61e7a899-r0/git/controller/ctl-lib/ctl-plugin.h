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
 */


#ifndef _CTL_PLUGIN_INCLUDE_
#define _CTL_PLUGIN_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <json-c/json.h>

#include "afb-definitions.h"

#ifndef CTL_PLUGIN_MAGIC
  #define CTL_PLUGIN_MAGIC 852369147
#endif

#ifndef UNUSED_ARG
  #define UNUSED_ARG(x) UNUSED_ ## x __attribute__((__unused__))
  #define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#endif

#ifdef CONTROL_SUPPORT_LUA
  typedef struct luaL_Reg luaL_Reg;

  typedef struct CtlLua2cFuncT {
    luaL_Reg *l2cFunc;
    const char *prefix;
    int l2cCount;
} CtlLua2cFuncT;
#endif

typedef struct {
  const char *uid;
  const long magic;
} CtlPluginMagicT;

typedef struct {
    const char *uid;
    const char *info;
    AFB_ApiT api;
    void *dlHandle;
    void *context;
#ifdef CONTROL_SUPPORT_LUA
    CtlLua2cFuncT *ctlL2cFunc;
#endif
} CtlPluginT;

typedef enum {
    CTL_TYPE_NONE=0,
    CTL_TYPE_API,
    CTL_TYPE_CB,
    CTL_TYPE_LUA,
} CtlActionTypeT;

typedef enum {
    CTL_STATUS_DONE=0,
    CTL_STATUS_ERR=-1,
    CTL_STATUS_EVT=1,
    CTL_STATUS_FREE=2,
} CtlActionStatusT;

typedef struct {
    const char *uid;
    AFB_ApiT api;
    AFB_ReqT request;
    void *context;
    CtlActionStatusT status;
} CtlSourceT;

typedef struct {
    const char *uid;
    const char *info;
    const char *privileges;
    AFB_ApiT api;
    json_object *argsJ;
    CtlActionTypeT type;
    union {
        struct {
            const char* api;
            const char* verb;
        } subcall;

        struct {
            const char* plugin;
            const char* funcname;
        } lua;

        struct {
            const char* funcname;
            int (*callback)(CtlSourceT *source, json_object *argsJ, json_object *queryJ);
            CtlPluginT *plugin;
        } cb;
    } exec;
} CtlActionT;

extern CtlPluginT *ctlPlugins;

typedef int(*DispatchPluginInstallCbT)(CtlPluginT *plugin, void* handle);

#define MACRO_STR_VALUE(arg) #arg
#define CTLP_CAPI_REGISTER(pluglabel) CtlPluginMagicT CtlPluginMagic={.uid=pluglabel,.magic=CTL_PLUGIN_MAGIC}; struct afb_binding_data_v2;
#define CTLP_ONLOAD(plugin, handle) int CtlPluginOnload(CtlPluginT *plugin, void* handle)
#define CTLP_CAPI(funcname, source, argsJ, queryJ) int funcname(CtlSourceT *source, json_object* argsJ, json_object* queryJ)

// LUA2c Wrapper macro. Allows to call C code from Lua script
typedef int (*Lua2cFunctionT)(CtlSourceT *source, json_object *argsJ, json_object **responseJ);
typedef int (*Lua2cWrapperT) (void*luaHandle, const char *funcname, Lua2cFunctionT callback);

#define CTLP_LUA_REGISTER(pluglabel) Lua2cWrapperT Lua2cWrap; CTLP_CAPI_REGISTER(pluglabel);
#define CTLP_LUA2C(funcname, source, argsJ, responseJ) int funcname (CtlSourceT* source, json_object* argsJ, json_object** responseJ);\
        int lua2c_ ## funcname (void* luaState){return((*Lua2cWrap)(luaState, MACRO_STR_VALUE(funcname), funcname));};\
        int funcname (CtlSourceT* source, json_object* argsJ, json_object** responseJ)

#ifdef __cplusplus
}
#endif

#endif /* _CTL_PLUGIN_INCLUDE_ */
