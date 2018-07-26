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

// Waiting for a clean AppFW-V3 API
#ifdef USE_API_DYN
    #define AFB_BINDING_VERSION dyn
    #include <afb/afb-binding.h>

    #define AFB_BINDING_PREV3
    #define AFB_ReqNone NULL
    typedef afb_request* AFB_ReqT;
    typedef afb_dynapi*  AFB_ApiT;

    typedef afb_eventid* AFB_EventT;
    #define AFB_EventIsValid(eventid) eventid
    #define AFB_EventPush afb_eventid_push
    #define AFB_ReqSubscribe  afb_request_subscribe
    #define AFB_EventMake(api, name) afb_dynapi_make_eventid(api, name)

    #define AFB_ReqJson(request) afb_request_json(request)

    #define AFB_ReqSucess  afb_request_success
    #define AFB_ReqSucessF afb_request_success_f
    #define AFB_ReqFail    afb_request_fail
    #define AFB_ReqFailF   afb_request_fail_f

    #define AFB_ReqNotice(request, ...)   AFB_REQUEST_NOTICE (request, __VA_ARGS__)
    #define AFB_ReqWarning(request, ...)  AFB_REQUEST_WARNING (request, __VA_ARGS__)
    #define AFB_ReqDebug(request, ...)    AFB_REQUEST_DEBUG (request, __VA_ARGS__)
    #define AFB_ReqError(request, ...)    AFB_REQUEST_ERROR (request, __VA_ARGS__)
    #define AFB_ReqInfo(request, ...)     AFB_REQUEST_INFO (request, __VA_ARGS__)

    #define AFB_ApiVerbose(api, level, ...) afb_dynapi_verbose(api, level, __VA_ARGS__)
    #define AFB_ApiNotice(api, ...)   AFB_DYNAPI_NOTICE (api, __VA_ARGS__)
    #define AFB_ApiWarning(api, ...)  AFB_DYNAPI_WARNING (api, __VA_ARGS__)
    #define AFB_ApiDebug(api, ...)    AFB_DYNAPI_DEBUG (api, __VA_ARGS__)
    #define AFB_ApiError(api, ...)    AFB_DYNAPI_ERROR (api, __VA_ARGS__)
    #define AFB_ApiInfo(api, ...)     AFB_DYNAPI_INFO (api, __VA_ARGS__)

    #define AFB_ReqIsValid(request)   request
    #define AFB_EvtIsValid(evtHandle) evtHandle

    #define AFB_ServiceCall(api, ...) afb_dynapi_call(api, __VA_ARGS__)
    #define AFB_ServiceSync(api, ...) afb_dynapi_call_sync(api, __VA_ARGS__)

    #define AFB_RequireApi(api, ...) afb_dynapi_require_api(api, __VA_ARGS__)

    #define AFB_GetEventLoop(api) afb_dynapi_get_event_loop(api)

    #define AFB_ClientCtxSet(request, replace, createCB, freeCB, handle) afb_request_context(request, replace, createCB, freeCB, handle)
    #define AFB_ClientCtxClear(request) afb_request_context_clear(request)


    typedef struct {
            const char *verb;                       /* name of the verb, NULL only at end of the array */
            void (*callback)(AFB_ReqT req);   /* callback function implementing the verb */
            const struct afb_auth *auth;		/* required authorisation, can be NULL */
            const char *info;			/* some info about the verb, can be NULL */
            uint32_t session;
    } AFB_ApiVerbs;

#else
    #define AFB_BINDING_VERSION 2
    #include <afb/afb-binding.h>

    typedef afb_req AFB_ReqT;
    typedef void* AFB_ApiT;
    #define AFB_ReqNone (struct afb_req){0,0}

    typedef afb_event AFB_EventT;
    #define AFB_EventPush afb_event_push
    #define AFB_ReqSubscribe  afb_req_subscribe
    #define AFB_EventIsValid(event) afb_event_is_valid(event)
    #define AFB_EventMake(api, name) afb_daemon_make_event(name)

    #define AFB_ReqJson(request) afb_req_json(request)
    #define AFB_ReqSucess  afb_req_success
    #define AFB_ReqSucessF afb_req_success_f
    #define AFB_ReqFail    afb_req_fail
    #define AFB_ReqFailF   afb_req_fail_f

    #define AFB_ReqNotice(request, ...)   AFB_NOTICE (__VA_ARGS__)
    #define AFB_ReqWarning(request, ...)  AFB_WARNING (__VA_ARGS__)
    #define AFB_ReqDebug(request, ...)    AFB_DEBUG (__VA_ARGS__)
    #define AFB_ReqError(request, ...)    AFB_ERROR (__VA_ARGS__)
    #define AFB_ReqInfo(request, ...)     AFB_INFO (__VA_ARGS__)

    #define AFB_ApiVerbose(api, level, ...) afb_daemon_verbose_v2(level,__VA_ARGS__)
    #define AFB_ApiNotice(api, ...)   AFB_NOTICE (__VA_ARGS__)
    #define AFB_ApiWarning(api, ...)  AFB_WARNING (__VA_ARGS__)
    #define AFB_ApiDebug(api, ...)    AFB_DEBUG (__VA_ARGS__)
    #define AFB_ApiError(api, ...)    AFB_ERROR (__VA_ARGS__)
    #define AFB_ApiInfo(api, ...)     AFB_INFO (__VA_ARGS__)

    #define AFB_ReqIsValid(request)   afb_req_is_valid(request)
    #define AFB_EvtIsValid(evtHandle) afb_event_is_valid(evtHandle)

    #define AFB_ServiceCall(api, ...) afb_service_call(__VA_ARGS__)
    #define AFB_ServiceSync(api, ...) afb_service_call_sync(__VA_ARGS__)

    #define AFB_RequireApi(api, ...)  afb_daemon_require_api(__VA_ARGS__)

    #define AFB_GetEventLoop(api) afb_daemon_get_event_loop()

    static inline void* AFB_ClientCtxSet(afb_req request, int replace, void *(*create_context)(void *closure), void (*free_context)(void*), void *closure)
    {
        void *ctx = create_context(closure);
        if(ctx)
            {afb_req_context_set(request, ctx, free_context);}
        return ctx;
    }

    #define AFB_ClientCtxClear(request) afb_req_context_clear(request)

    #define AFB_ApiVerbs afb_verb_v2
#endif


#ifndef CTL_PLUGIN_MAGIC
  #define CTL_PLUGIN_MAGIC 852369147
#endif

#ifndef PUBLIC
  #define PUBLIC
#endif

#ifndef STATIC
  #define STATIC static
#endif

#ifndef UNUSED_ARG
  #define UNUSED_ARG(x) UNUSED_ ## x __attribute__((__unused__))
  #define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
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
            const char* load;
            const char* funcname;
        } lua;

        struct {
            const char* funcname;
            int (*callback)(CtlSourceT *source, json_object *argsJ, json_object *queryJ);
            CtlPluginT *plugin;
        } cb;
    } exec;
} CtlActionT;


typedef void*(*DispatchPluginInstallCbT)(CtlPluginT *plugin, void* handle);

#define MACRO_STR_VALUE(arg) #arg
#define CTLP_CAPI_REGISTER(pluglabel) CtlPluginMagicT CtlPluginMagic={.uid=pluglabel,.magic=CTL_PLUGIN_MAGIC}; struct afb_binding_data_v2;
#define CTLP_ONLOAD(plugin, handle) void* CtlPluginOnload(CtlPluginT *plugin, void* handle)
#define CTLP_CAPI(funcname, source, argsJ, queryJ) int funcname(CtlSourceT *source, json_object* argsJ, json_object* queryJ)

// LUA2c Wrapper macro. Allows to call C code from Lua script
typedef int (*Lua2cFunctionT)(CtlSourceT *source, json_object *argsJ, json_object **responseJ);
typedef int (*Lua2cWrapperT) (void*luaHandle, const char *funcname, Lua2cFunctionT callback);

#define CTLP_LUA_REGISTER(pluglabel) Lua2cWrapperT Lua2cWrap; CTLP_CAPI_REGISTER(pluglabel);
#define CTLP_LUA2C(funcname, source, argsJ, responseJ) static int funcname (CtlSourceT* source, json_object* argsJ, json_object** responseJ);\
        int lua2c_ ## funcname (void* luaState){return((*Lua2cWrap)(luaState, MACRO_STR_VALUE(funcname), funcname));};\
        static int funcname (CtlSourceT* source, json_object* argsJ, json_object** responseJ)

#ifdef __cplusplus
}
#endif

#endif /* _CTL_PLUGIN_INCLUDE_ */
