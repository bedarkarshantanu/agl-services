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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ref:
 *  (manual) https://www.lua.org/manual/5.3/manual.html
 *  (lua->C) http://www.troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm#_Anatomy_of_a_Lua_Call
 *  (lua/C Var) http://acamara.es/blog/2012/08/passing-variables-from-lua-5-2-to-c-and-vice-versa/
 *  (Lua/C Lib)https://john.nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
 *  (Lua/C Table) https://gist.github.com/SONIC3D/10388137
 *  (Lua/C Nested table) https://stackoverflow.com/questions/45699144/lua-nested-table-from-lua-to-c
 *  (Lua/C Wrapper) https://stackoverflow.com/questions/45699950/lua-passing-handle-to-function-created-with-newlib
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "ctl-config.h"

#define LUA_FIRST_ARG 2 // when using luaL_newlib callback receive libtable as 1st arg
#define LUA_MSG_MAX_LENGTH 2048
#define JSON_ERROR (json_object*)-1
#define LUA_PATH_VALUE "package.path = package.path .. ';?.lua;"
#define LUA_GLOB_PATTERN "/?.lua;"

static lua_State* luaState;
CtlPluginT *ctlPlugins = NULL;

#if CTX_MAGIC_VALUE
static int CTX_MAGIC = CTX_MAGIC_VALUE;
#else
static int CTX_MAGIC;
#endif

#if TIMER_MAGIC_VALUE
static int TIMER_MAGIC = TIMER_MAGIC_VALUE;
#else
static int TIMER_MAGIC;
#endif

typedef struct {
    char *name;
    int count;
    AFB_EventT event;
} LuaAfbEvent;

typedef struct {
    int ctxMagic;
    CtlSourceT *source;
} LuaAfbSourceT;

typedef struct {
    const char *callback;
    json_object *context;
    CtlSourceT *source;
} LuaCbHandleT;

/*
 * Note(Fulup): I fail to use luaL_setmetatable and replaced it with a simple opaque
 * handle while waiting for someone smarter than me to find a better solution
 *  https://stackoverflow.com/questions/45596493/lua-using-lua-newuserdata-from-lua-pcall
 */
static CtlSourceT *LuaSourcePop(lua_State *luaState, int index) {
    LuaAfbSourceT *afbSource;
    luaL_checktype(luaState, index, LUA_TLIGHTUSERDATA);
    afbSource = (LuaAfbSourceT *) lua_touserdata(luaState, index);
    if (afbSource == NULL || afbSource->ctxMagic != CTX_MAGIC) {
        luaL_error(luaState, "(Hoops) Invalid source handle");
        return NULL;
    }
    return afbSource->source;
}

static LuaAfbSourceT *LuaSourcePush(lua_State *luaState, CtlSourceT *source) {
    LuaAfbSourceT *afbSource = (LuaAfbSourceT *) calloc(1, sizeof (LuaAfbSourceT));
    if (!afbSource) {
        AFB_ApiError(source->api, "LuaSourcePush fail to allocate user data context");
        return NULL;
    }

    lua_pushlightuserdata(luaState, afbSource);
    afbSource->ctxMagic = CTX_MAGIC;
    afbSource->source = source;
    return afbSource;
}

// Push a json structure on the stack as a LUA table

static int LuaPushArgument(CtlSourceT *source, json_object *argsJ) {

    //AFB_NOTICE("LuaPushArgument argsJ=%s", json_object_to_json_string(argsJ));

    json_type jtype = json_object_get_type(argsJ);
    switch (jtype) {
        case json_type_object:
        {
            lua_newtable(luaState);

            json_object_object_foreach(argsJ, key, val) {
                int done = LuaPushArgument(source, val);
                if (done) {
                    lua_setfield(luaState, -2, key);
                }
            }
            break;
        }
        case json_type_array:
        {
            int length = (int)json_object_array_length(argsJ);
            lua_newtable(luaState);
            for (int idx = 0; idx < length; idx++) {
                json_object *val = json_object_array_get_idx(argsJ, idx);
                LuaPushArgument(source, val);
                lua_seti(luaState, -2, idx);
            }
            break;
        }
        case json_type_int:
            lua_pushinteger(luaState, json_object_get_int(argsJ));
            break;
        case json_type_string:
            lua_pushstring(luaState, json_object_get_string(argsJ));
            break;
        case json_type_boolean:
            lua_pushboolean(luaState, json_object_get_boolean(argsJ));
            break;
        case json_type_double:
            lua_pushnumber(luaState, json_object_get_double(argsJ));
            break;
        case json_type_null:
            AFB_ApiNotice(source->api, "LuaPushArgument: NULL object type %s", json_object_to_json_string(argsJ));
            lua_pushnil(luaState);
            break;

        default:
            AFB_ApiError(source->api, "LuaPushArgument: unsupported Json object type %s", json_object_to_json_string(argsJ));
            return 0;
    }
    return 1;
}

static json_object *LuaPopOneArg(CtlSourceT *source, lua_State* luaState, int idx);

// Move a table from Internal Lua representation to Json one
// Numeric table are transformed in json array, string one in object
// Mix numeric/string key are not supported

static json_object *LuaTableToJson(CtlSourceT *source, lua_State* luaState, int index) {
#define LUA_KEY_INDEX -2
#define LUA_VALUE_INDEX -1

    int idx;
    int tableType;
    json_object *tableJ = NULL;

    lua_pushnil(luaState); // 1st key
    if (index < 0) index--;
    for (idx = 1; lua_next(luaState, index) != 0; idx++) {

        // uses 'key' (at index -2) and 'value' (at index -1)
        if (lua_type(luaState, LUA_KEY_INDEX) == LUA_TSTRING) {

            if (!tableJ) {
                tableJ = json_object_new_object();
                tableType = LUA_TSTRING;
            } else if (tableType != LUA_TSTRING) {
                AFB_ApiError(source->api, "MIX Lua Table with key string/numeric not supported");
                return NULL;
            }

            const char *key = lua_tostring(luaState, LUA_KEY_INDEX);
            json_object *argJ = LuaPopOneArg(source, luaState, LUA_VALUE_INDEX);
            json_object_object_add(tableJ, key, argJ);

        } else {
            if (!tableJ) {
                tableJ = json_object_new_array();
                tableType = LUA_TNUMBER;
            } else if (tableType != LUA_TNUMBER) {
                AFB_ApiError(source->api, "MIX Lua Table with key numeric/string not supported");
                return NULL;
            }

            json_object *argJ = LuaPopOneArg(source, luaState, LUA_VALUE_INDEX);
            json_object_array_add(tableJ, argJ);
        }


        lua_pop(luaState, 1); // removes 'value'; keeps 'key' for next iteration
    }

    // Query is empty free empty json object
    if (idx == 1) {
        json_object_put(tableJ);
        return NULL;
    }
    return tableJ;
}

static json_object *LuaPopOneArg(CtlSourceT *source, lua_State* luaState, int idx) {
    json_object *value = NULL;

    int luaType = lua_type(luaState, idx);
    switch (luaType) {
        case LUA_TNUMBER:
        {
            lua_Number number = lua_tonumber(luaState, idx);
            ;
            int nombre = (int) number; // evil trick to determine wether n fits in an integer. (stolen from ltcl.c)
            if (number == nombre) {
                value = json_object_new_int((int) number);
            } else {
                value = json_object_new_double(number);
            }
            break;
        }
        case LUA_TBOOLEAN:
            value = json_object_new_boolean(lua_toboolean(luaState, idx));
            break;
        case LUA_TSTRING:
            value = json_object_new_string(lua_tostring(luaState, idx));
            break;
        case LUA_TTABLE:
            value = LuaTableToJson(source, luaState, idx);
            break;
        case LUA_TNIL:
            value = json_object_new_string("nil");
            break;
        case LUA_TUSERDATA:
            value = json_object_new_int64((int64_t) lua_touserdata(luaState, idx)); // store userdata as int !!!
            break;

        default:
            AFB_ApiNotice(source->api, "LuaPopOneArg: script returned Unknown/Unsupported idx=%d type:%d/%s", idx, luaType, lua_typename(luaState, luaType));
            value = NULL;
    }

    return value;
}

static json_object *LuaPopArgs(CtlSourceT *source, lua_State* luaState, int start) {
    json_object *responseJ;

    int stop = lua_gettop(luaState);
    if (stop - start < 0) return NULL;

    // start at 2 because we are using a function array lib
    if (start == stop) {
        responseJ = LuaPopOneArg(source, luaState, start);
    } else {
        // loop on remaining return arguments
        responseJ = json_object_new_array();
        for (int idx = start; idx <= stop; idx++) {
            json_object *argJ = LuaPopOneArg(source, luaState, idx);
            if (!argJ)
                return NULL;
            json_object_array_add(responseJ, argJ);
        }
    }

    return responseJ;
}

static int LuaFormatMessage(lua_State* luaState, int verbosity, int level) {
    char *message;

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source)
        return 1;

    // if log level low then silently ignore message
#ifndef AFB_BINDING_PREV3
    if (afb_get_verbosity() < verbosity) return 0;
#elif !defined(AFB_BINDING_INTERFACE_VERSION)
    if (source->api->verbosity < verbosity) return 0;
#else
    if (!afb_dynapi_wants_log_level(source->api, level)) return 0;
#endif

    json_object *responseJ = LuaPopArgs(source, luaState, LUA_FIRST_ARG + 1);

    if (!responseJ) {
        luaL_error(luaState, "LuaFormatMessage empty message");
        return 1;
    }

    // if we have only on argument just return the value.
    if (json_object_get_type(responseJ) != json_type_array || json_object_array_length(responseJ) < 2) {
        message = (char*) json_object_get_string(responseJ);
        goto PrintMessage;
    }

    // extract format and push all other parameter on the stack
    message = alloca(LUA_MSG_MAX_LENGTH);
    const char *format = json_object_get_string(json_object_array_get_idx(responseJ, 0));

    int arrayIdx = 1;
    int uidIdx = 0;

    for (int idx = 0; format[idx] != '\0'; idx++) {

        if (format[idx] == '%' && format[idx+1] != '\0') {
            json_object *slotJ = json_object_array_get_idx(responseJ, arrayIdx);
            //if (slotJ) AFB_NOTICE("**** idx=%d slotJ=%s", arrayIdx, json_object_get_string(slotJ));

            switch (format[++idx]) {
                case 'd':
                    if (slotJ) uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "%d", json_object_get_int(slotJ));
                    else uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "nil");
                    arrayIdx++;
                    break;
                case 'f':
                    if (slotJ) uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "%f", json_object_get_double(slotJ));
                    else uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "nil");
                    arrayIdx++;
                    break;

                case'%':
                    message[uidIdx] = '%';
                    uidIdx++;
                    break;

                case 'A':
                    uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "level: %s", source->uid);
                    break;

                case 's':
                default:
                    if (slotJ) uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "%s", json_object_get_string(slotJ));
                    else uidIdx += snprintf(&message[uidIdx], LUA_MSG_MAX_LENGTH - uidIdx, "nil");
                    arrayIdx++;
            }

        } else {
            if (uidIdx >= LUA_MSG_MAX_LENGTH) {
                const char *trunc = "... <truncated> ";

                AFB_ApiWarning(source->api, "LuaFormatMessage: message[%s] overflow LUA_MSG_MAX_LENGTH=%d\n", format, LUA_MSG_MAX_LENGTH);
                uidIdx = LUA_MSG_MAX_LENGTH - 1;
                memcpy(&message[uidIdx - strlen(trunc)], trunc, strlen(trunc));
                break;
            } else {
                message[uidIdx++] = format[idx];
            }
        }
    }
    message[uidIdx] = '\0';

PrintMessage:
    // TBD: __file__ and __line__ should match LUA source code
    AFB_ApiVerbose(source->api, level, __FILE__, __LINE__, source->uid, "%s", message);
    json_object_put(responseJ);
    return 0; // nothing return to lua
}

static int LuaPrintInfo(lua_State* luaState) {
    int err = LuaFormatMessage(luaState, AFB_VERBOSITY_LEVEL_INFO, _AFB_SYSLOG_LEVEL_INFO_);
    return err;
}

static int LuaPrintError(lua_State* luaState) {
    int err = LuaFormatMessage(luaState, AFB_VERBOSITY_LEVEL_ERROR, _AFB_SYSLOG_LEVEL_ERROR_);
    return err; // no value return
}

static int LuaPrintWarning(lua_State* luaState) {
    int err = LuaFormatMessage(luaState, AFB_VERBOSITY_LEVEL_WARNING, _AFB_SYSLOG_LEVEL_WARNING_);
    return err;
}

static int LuaPrintNotice(lua_State* luaState) {
    int err = LuaFormatMessage(luaState, AFB_VERBOSITY_LEVEL_NOTICE, _AFB_SYSLOG_LEVEL_NOTICE_);
    return err;
}

static int LuaPrintDebug(lua_State* luaState) {
    int err = LuaFormatMessage(luaState, AFB_VERBOSITY_LEVEL_DEBUG, _AFB_SYSLOG_LEVEL_DEBUG_);
    return err;
}

static int LuaAfbSuccess(lua_State* luaState) {
    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_error(luaState);
        return 1;
    }

    // ignore context argument
    json_object *responseJ = LuaPopArgs(source, luaState, LUA_FIRST_ARG + 1);
    if (responseJ == JSON_ERROR) return 1;

    AFB_ReqSuccess(source->request, responseJ, NULL);

    return 0;
}

static int LuaAfbFail(lua_State* luaState) {
    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_error(luaState);
        return 1;
    }

    json_object *responseJ = LuaPopArgs(source, luaState, LUA_FIRST_ARG + 1);
    if (responseJ == JSON_ERROR) return 1;

    AFB_ReqFail(source->request, source->uid, json_object_get_string(responseJ));

    json_object_put(responseJ);
    return 0;
}

#ifdef AFB_BINDING_PREV3

static void LuaAfbServiceCB(void *handle, int iserror, struct json_object *responseJ, AFB_ApiT apiHandle) {
#else

static void LuaAfbServiceCB(void *handle, int iserror, struct json_object *responseJ) {
#endif
    LuaCbHandleT *handleCb = (LuaCbHandleT*) handle;
    int count = 1;

    lua_getglobal(luaState, handleCb->callback);

    // Push AFB client context on the stack
    if (iserror) handleCb->source->status = CTL_STATUS_ERR;
    else handleCb->source->status = CTL_STATUS_DONE;
    LuaSourcePush(luaState, handleCb->source);

    // push response
    count += LuaPushArgument(handleCb->source, responseJ);
    if (handleCb->context) count += LuaPushArgument(handleCb->source, handleCb->context);

    int err = lua_pcall(luaState, count, LUA_MULTRET, 0);
    if (err) {
        AFB_ApiError(apiHandle, "LUA-SERVICE-CB:FAIL response=%s err=%s", json_object_to_json_string(responseJ), lua_tostring(luaState, -1));
    }

    free(handleCb->source);
    free(handleCb);
}

static int LuaAfbService(lua_State* luaState) {
    int count = lua_gettop(luaState);

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbService-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }

    // note: argument start at 2 because of AFB: table
    if (count < 6 || !lua_isstring(luaState, 3) || !lua_isstring(luaState, 4) || !lua_isstring(luaState, 6)) {
        lua_pushliteral(luaState, "ERROR: syntax AFB:service(source, api, verb, {[Lua Table]})");
        lua_error(luaState);
        return 1;
    }

    // get api/verb+query
    const char *api = lua_tostring(luaState, 3);
    const char *verb = lua_tostring(luaState, 4);
    json_object *queryJ = LuaTableToJson(source, luaState, 5);
    if (queryJ == JSON_ERROR) return 1;

    LuaCbHandleT *handleCb = calloc(1, sizeof (LuaCbHandleT));
    handleCb->callback = lua_tostring(luaState, 6);
    handleCb->context = LuaPopArgs(source, luaState, 7);

    // source need to be duplicate because request return free it
    handleCb->source = malloc(sizeof (CtlSourceT));
    handleCb->source = memcpy(handleCb->source, source, sizeof (CtlSourceT));

    AFB_ServiceCall(source->api, api, verb, queryJ, LuaAfbServiceCB, handleCb);

    return 0; // no value return
}

static int LuaAfbServiceSync(lua_State* luaState) {
    int count = lua_gettop(luaState);
    json_object *responseJ;

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbServiceSync-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }

    // note: argument start at 2 because of AFB: table
    if (count != 5 || !lua_isstring(luaState, LUA_FIRST_ARG + 1) || !lua_isstring(luaState, LUA_FIRST_ARG + 2) || !lua_istable(luaState, LUA_FIRST_ARG + 3)) {
        lua_pushliteral(luaState, "ERROR: syntax AFB:servsync(source, api, verb, {[Lua Table]})");
        lua_error(luaState);
        return 1;
    }


    // get source/api/verb+query
    const char *api = lua_tostring(luaState, LUA_FIRST_ARG + 1);
    const char *verb = lua_tostring(luaState, LUA_FIRST_ARG + 2);
    json_object *queryJ = LuaTableToJson(source, luaState, LUA_FIRST_ARG + 3);

    int iserror = AFB_ServiceSync(source->api, api, verb, queryJ, &responseJ);

    // push error status & response
    count = 1;
    lua_pushboolean(luaState, iserror);
    count += LuaPushArgument(source, responseJ);

    return count; // return count values
}

static int LuaAfbEventPush(lua_State* luaState) {
    LuaAfbEvent *afbevt;

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbEventSubscribe-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }

    // if no private event handle then use default binding event
    if (!lua_islightuserdata(luaState, LUA_FIRST_ARG + 1)) {
        lua_pushliteral(luaState, "LuaAfbMakePush-Fail missing event handle");
        lua_error(luaState);
        return 1;
    }

    afbevt = (LuaAfbEvent*) lua_touserdata(luaState, LUA_FIRST_ARG + 1);

    if (!AFB_EventIsValid(afbevt->event)) {
        lua_pushliteral(luaState, "LuaAfbMakePush-Fail invalid event");
        lua_error(luaState);
        return 1;
    }

    json_object *ctlEventJ = LuaTableToJson(source, luaState, LUA_FIRST_ARG + 2);
    if (!ctlEventJ) {
        lua_pushliteral(luaState, "LuaAfbEventPush-Syntax is AFB:signal ([evtHandle], {lua table})");
        lua_error(luaState);
        return 1;
    }

    int done = AFB_EventPush(afbevt->event, ctlEventJ);
    if (!done) {
        lua_pushliteral(luaState, "LuaAfbEventPush-Fail No Subscriber to event");
        AFB_ApiError(source->api, "LuaAfbEventPush-Fail name subscriber event=%s count=%d", afbevt->name, afbevt->count);
        lua_error(luaState);
        return 1;
    }
    afbevt->count++;
    return 0;
}

static int LuaAfbEventSubscribe(lua_State* luaState) {
    LuaAfbEvent *afbevt;

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbEventSubscribe-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }

    // if no private event handle then use default binding event
    if (!lua_islightuserdata(luaState, LUA_FIRST_ARG + 1)) {
        lua_pushliteral(luaState, "LuaAfbMakePush-Fail missing event handle");
        lua_error(luaState);
        return 1;
    }

    afbevt = (LuaAfbEvent*) lua_touserdata(luaState, LUA_FIRST_ARG + 1);

    if (!AFB_EventIsValid(afbevt->event)) {
        lua_pushliteral(luaState, "LuaAfbMakePush-Fail invalid event handle");
        lua_error(luaState);
        return 1;
    }

    int err = AFB_ReqSubscribe(source->request, afbevt->event);
    if (err) {
        lua_pushliteral(luaState, "LuaAfbEventSubscribe-Fail No Subscriber to event");
        AFB_ApiError(source->api, "LuaAfbEventPush-Fail name subscriber event=%s count=%d", afbevt->name, afbevt->count);
        lua_error(luaState);
        return 1;
    }
    afbevt->count++;
    return 0;
}

static int LuaLockWait(lua_State* luaState) {
    luaL_checktype(luaState, LUA_FIRST_ARG, LUA_TLIGHTUSERDATA);
    luaL_checktype(luaState, LUA_FIRST_ARG + 1, LUA_TNUMBER);

    CtlSourceT *source = (CtlSourceT *)LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaLockWait-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }
    uint64_t utimeout = (int)lua_tointeger(luaState, LUA_FIRST_ARG + 1);

    uint64_t remain_timeout = LockWait(source->api, utimeout);
    lua_pushinteger(luaState, remain_timeout);
    return 1; //return nb of pushed elements
}

static int LuaAfbEventMake(lua_State* luaState) {
    int count = lua_gettop(luaState);
    LuaAfbEvent *afbevt = calloc(1, sizeof (LuaAfbEvent));

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbEventMake-Fail Invalid request handle");
        lua_error(luaState);
        return 1;
    }

    if (count != LUA_FIRST_ARG + 1 || !lua_isstring(luaState, LUA_FIRST_ARG + 1)) {
        lua_pushliteral(luaState, "LuaAfbEventMake-Syntax is evtHandle= AFB:event ('myEventName')");
        lua_error(luaState);
        return 1;
    }

    // event name should be the only argument
    afbevt->name = strdup(lua_tostring(luaState, LUA_FIRST_ARG + 1));

    // create a new binder event
    afbevt->event = AFB_EventMake(source->api, afbevt->name);
    if (!AFB_EventIsValid(afbevt->event)) {
        AFB_ApiError(source->api, "Fail to CreateEvent evtname=%s", afbevt->name);
        lua_pushliteral(luaState, "LuaAfbEventMake-Fail to Create Binder event");
        lua_error(luaState);
        return 1;
    }

    // push event handler as a LUA opaque handle
    lua_pushlightuserdata(luaState, afbevt);
    return 1;
}

static int LuaAfbGetUid(lua_State* luaState) {

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbEventSubscribe-Fail Invalid request handle");
        return 0;
    }

    // extract and return afbSource from timer handle
    lua_pushstring(luaState, source->uid);

    return 1; // return argument
}

static int LuaAfbGetStatus(lua_State* luaState) {

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_pushliteral(luaState, "LuaAfbEventSubscribe-Fail Invalid request handle");
        return 0;
    }

    // extract and return afbSource from timer handle
    lua_pushinteger(luaState, source->status);

    return 1; // return argument
}

// Function call from LUA when lua2c plugin L2C is used

int Lua2cWrapper(void* luaHandle, char *funcname, Lua2cFunctionT callback) {
    lua_State* luaState = (lua_State*) luaHandle;
    json_object *responseJ = NULL;

    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);

    json_object *argsJ = LuaPopArgs(source, luaState, LUA_FIRST_ARG + 1);
    int err = (*callback) (source, argsJ, &responseJ);

    // push error code and eventual response to LUA
    int count = 1;
    lua_pushinteger(luaState, err);
    if (responseJ)count += LuaPushArgument(source, responseJ);

    return count;
}

// Call a Lua function from a control action

int LuaCallFunc(CtlSourceT *source, CtlActionT *action, json_object *queryJ) {

    int err = 0, count = 1;

    json_object* argsJ = action->argsJ;
    const char* func = action->exec.lua.funcname;

    // load function (should exist in CONTROL_PATH_LUA)
    lua_getglobal(luaState, func);

    // push source on the stack
    // Push AFB client context on the stack
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, source);
    if (!afbSource)
        return -1;

    // push argsJ on the stack
    if (!argsJ) {
        lua_pushnil(luaState);
        count++;
    } else {
        count += LuaPushArgument(source, argsJ);
    }

    // push queryJ on the stack
    if (!queryJ) {
        lua_pushnil(luaState);
        count++;
    } else {
        count += LuaPushArgument(source, queryJ);
    }

    // effectively exec LUA script code
    err = lua_pcall(luaState, count, 1, 0);
    if (err) {
        AFB_ApiError(source->api, "LuaCallFunc Fail calling %s error=%s", func, lua_tostring(luaState, -1));
        return -1;
    }

    // return LUA script value
    int rc = (int) lua_tointeger(luaState, -1);
    return rc;
}

int LuaLoadScript(const char *luaScriptPath) {
    int err = 0;

    if (!luaScriptPath)
        return -1;

    err = luaL_loadfile(luaState, luaScriptPath);
    if (err)
        return err;

    // Script was loaded we need to parse to make it executable
    err = lua_pcall(luaState, 0, 0, 0);

    return err;
}

static int LuaDoScript(json_object *queryJ, CtlSourceT *source) {
    const char *uid = NULL, *func = NULL, *filename = NULL, *fullpath = NULL;
    char *luaScriptPath = NULL;
    int index, err = 0;
    size_t p_length = 0;
    json_object *argsJ = NULL;
    static json_object *luaScriptPathJ = NULL;

    if (!queryJ) {
        return -1;
    }

    err = wrap_json_unpack(queryJ, "{s:s,s?s,ss,s?o !}",
            "uid", &uid,
            "spath", &luaScriptPathJ,
            "action", &func,
            "args", &argsJ);

    if (err) {
        AFB_ApiError(source->api, "LUA-DOSCRIPT-SCAN: Miss something in JSON object uid|[spath]|action|[args]: %s", json_object_to_json_string(queryJ));
        return -1;
    }

    // search for filename=uid*.lua or fallback on bindername*.lua in current directory
    if (!luaScriptPathJ) {
        luaScriptPathJ = ScanForConfig(".", CTL_SCAN_RECURSIVE, uid, ".lua");
        if (!luaScriptPathJ)
            luaScriptPathJ = ScanForConfig(".", CTL_SCAN_RECURSIVE, GetBinderName(), ".lua");
    }

    if(!luaScriptPathJ) {
        AFB_ApiError(source->api, "LUA-DOSCRIPT-SCAN: No script found");
        return -1;
    }

    for (index = 0; index < json_object_array_length(luaScriptPathJ); index++) {
        json_object *entryJ = json_object_array_get_idx(luaScriptPathJ, index);

        err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ApiError(source->api, "LUA-DOSCRIPT-SCAN:HOOPs invalid config file path = %s", json_object_to_json_string(entryJ));
            return -1;
        }

        // Ignoring other found script. Only take the first one.
        if (!index) {
            p_length = strlen(fullpath) + 1 + strlen(filename);
            luaScriptPath = malloc(p_length + 1);

            strncpy(luaScriptPath, fullpath, CONTROL_MAXPATH_LEN - 1);
            strncat(luaScriptPath, "/", CONTROL_MAXPATH_LEN - strlen(luaScriptPath) - 1);
            strncat(luaScriptPath, filename, CONTROL_MAXPATH_LEN - strlen(luaScriptPath) - 1);
        }
    }

    err = LuaLoadScript(luaScriptPath);
    if (err) {
        AFB_ApiError(source->api, "LUA-DOSCRIPT HOOPs Error in LUA loading scripts=%s err=%s", luaScriptPath, lua_tostring(luaState, -1));
        return -1;
    }

    // load function (should exist in CONTROL_PATH_LUA
    lua_getglobal(luaState, func);

    // Push AFB client context on the stack
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, source);
    if (!afbSource)
        return -1;

    return 0;
}

static int LuaDoCall(json_object *queryJ, CtlSourceT *source) {
    int err = 0;
    int count = 0;
    const char *func;
    json_object *argsJ = NULL;

    if (!queryJ)
        return -1;

    err = wrap_json_unpack(queryJ, "{s:s, s?o !}", "uid", &func, "args", &argsJ);
    if (err)
        return -2;

    // load function (should exist in CONTROL_PATH_LUA
    lua_getglobal(luaState, func);

    // Push AFB client context on the stack
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, source);
    if (!afbSource)
        return -3;

    // push query on the stack
    if (!argsJ) {
        lua_pushnil(luaState);
        count++;
    } else {
        count += LuaPushArgument(source, argsJ);
    }

    return count;
}

static int LuaDoString(const char *script, CtlSourceT *source) {
    int err = 0;

    err = luaL_loadstring(luaState, script);
    if (err)
        return -1;

    // Push AFB client context on the stack
    if (source) {
        LuaAfbSourceT *afbSource = LuaSourcePush(luaState, source);
        if (!afbSource)
            return -2;
    }

    return 0;
}

// Execute LUA code from received API request

static void LuaDoAction(LuaDoActionT action, AFB_ReqT request) {
    int err = 0, count = 0;
    CtlSourceT *source = alloca(sizeof (CtlSourceT));
    source->request = request;

    json_object* queryJ = AFB_ReqJson(request);

    switch (action) {

        case LUA_DOSTRING:
        {
            const char *script = json_object_get_string(queryJ);
            count = LuaDoString(script, source);
            if (count)
                AFB_ApiError(source->api, "DOSTRING goes wrong err=%d, String=%s ", count, script);
            break;
        }

        case LUA_DOCALL:
        {
            count = LuaDoCall(queryJ, source);
            if (count)
                AFB_ApiError(source->api, "DOCALL goes wrong, error = %d, query=%s", count, json_object_get_string(queryJ));
            break;
        }

        case LUA_DOSCRIPT:
        { // Fulup need to fix argument passing
            count = LuaDoScript(queryJ, source);
            if (count)
                AFB_ApiError(source->api, "DOSCRIPT goes wrong error=%d query=%s", count, json_object_to_json_string(queryJ));
            break;
        }

        default:
            AFB_ApiError(source->api, "LUA-DOSCRIPT-ACTION unknown query=%s", json_object_to_json_string(queryJ));
            AFB_ReqFail(request, "LUA:ERROR", lua_tostring(luaState, -1));
            return;
    }

    if (count >= 0)
        err = lua_pcall(luaState, count + 1, 0, 0);
    if (err) {
        AFB_ApiError(source->api, "LUA-DO-EXEC:FAIL query=%s err=%s", json_object_to_json_string(queryJ), lua_tostring(luaState, -1));
        AFB_ReqFail(request, "LUA:ERROR", lua_tostring(luaState, -1));
        return;
    }
    return;
}

void ctlapi_execlua(AFB_ReqT request) {
    LuaDoAction(LUA_DOSTRING, request);
}

void ctlapi_request(AFB_ReqT request) {
    LuaDoAction(LUA_DOCALL, request);
}

void ctlapi_debuglua(AFB_ReqT request) {
    LuaDoAction(LUA_DOSCRIPT, request);
}

static TimerHandleT *LuaTimerPop(lua_State *luaState, int index) {
    TimerHandleT *timerHandle;

    luaL_checktype(luaState, index, LUA_TLIGHTUSERDATA);
    timerHandle = (TimerHandleT *) lua_touserdata(luaState, index);

    if (timerHandle == NULL && timerHandle->magic != TIMER_MAGIC) {
        luaL_error(luaState, "Invalid source handle");
        fprintf(stderr, "LuaSourcePop error retrieving afbSource");
        return NULL;
    }
    return timerHandle;
}

static int LuaTimerClear(lua_State* luaState) {

    // retrieve useful information opaque handle
    TimerHandleT *timerHandle = LuaTimerPop(luaState, LUA_FIRST_ARG);
    if (!timerHandle)
        return 1;

#ifdef AFB_BINDING_PREV3
    // API handle does not exit in API-V2
    LuaCbHandleT *luaCbHandle = (LuaCbHandleT*) timerHandle->context;
#endif
    AFB_ApiNotice(luaCbHandle->source->api, "LuaTimerClear timer=%s", timerHandle->uid);
    TimerEvtStop(timerHandle);

    return 0; //happy end
}

static int LuaTimerGet(lua_State* luaState) {

    // retrieve useful information opaque handle
    TimerHandleT *timerHandle = LuaTimerPop(luaState, LUA_FIRST_ARG);
    if (!timerHandle)
        return 0;
    LuaCbHandleT *luaCbHandle = (LuaCbHandleT*) timerHandle->context;

    // create response as a JSON object
    json_object *responseJ = json_object_new_object();
    json_object_object_add(responseJ, "uid", json_object_new_string(timerHandle->uid));
    json_object_object_add(responseJ, "delay", json_object_new_int(timerHandle->delay));
    json_object_object_add(responseJ, "count", json_object_new_int(timerHandle->count));

    // return JSON object as Lua table
    int count = LuaPushArgument(luaCbHandle->source, responseJ);

    // free json object
    json_object_put(responseJ);

    return count; // return argument
}

// Timer Callback

// Set timer

static int LuaTimerSetCB(TimerHandleT *timer) {
    LuaCbHandleT *LuaCbHandle = (LuaCbHandleT*) timer->context;
    int count;

    // push timer handle and user context on Lua stack
    lua_getglobal(luaState, LuaCbHandle->callback);

    // Push AFB client context on the stack
    count = 1;
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, LuaCbHandle->source);
    if (!afbSource)
        return 1;

    // Push AFB client context on the stack
    count++;
    lua_pushlightuserdata(luaState, timer);
    if (!afbSource)
        return 1;

    // Push user Context
    count += LuaPushArgument(LuaCbHandle->source, LuaCbHandle->context);

    int err = lua_pcall(luaState, count, LUA_MULTRET, 0);
    if (err) {
        AFB_ApiError(LuaCbHandle->source->api, "LUA-TIMER-CB:FAIL response=%s err=%s", json_object_to_json_string(LuaCbHandle->context), lua_tostring(luaState, -1));
        return 1;
    }

    // get return parameter
    if (!lua_isboolean(luaState, -1)) {
        return (lua_toboolean(luaState, -1));
    }

    return 0; // By default we are happy
}

// Free Timer context handle

static int LuaTimerCtxFree(void *handle) {
    LuaCbHandleT *LuaCbHandle = (LuaCbHandleT*) handle;

    free(LuaCbHandle->source);
    free(LuaCbHandle);

    return 0;
}

static int LuaTimerSet(lua_State* luaState) {
    const char *uid = NULL, *info = NULL;
    int delay = 0, count = 0;

    // Get source handle
    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_error(luaState);
        return 1; // return error code
    }

    json_object *timerJ = LuaPopOneArg(source, luaState, LUA_FIRST_ARG + 1);
    const char *callback = lua_tostring(luaState, LUA_FIRST_ARG + 2);
    json_object *contextJ = LuaPopOneArg(source, luaState, LUA_FIRST_ARG + 3);

    if (lua_gettop(luaState) != LUA_FIRST_ARG + 3 || !timerJ || !callback || !contextJ) {
        lua_pushliteral(luaState, "LuaTimerSet: Syntax timerset (source, timerT, 'callback', contextT)");
        lua_error(luaState);
        return 1; // return error code
    }

    int err = wrap_json_unpack(timerJ, "{ss, s?s si, si !}",
            "uid", &uid,
            "info", &info,
            "delay", &delay,
            "count", &count);
    if (err) {

        lua_pushliteral(luaState, "LuaTimerSet: Syntax timerT={uid:xxx delay:ms, count:xx}");
        lua_error(luaState);
        return 1; // return error code
    }

    // Allocate handle to store context and callback
    LuaCbHandleT *handleCb = calloc(1, sizeof (LuaCbHandleT));
    handleCb->callback = callback;
    handleCb->context = contextJ;
    handleCb->source = malloc(sizeof (CtlSourceT));
    memcpy(handleCb->source, source, sizeof (CtlSourceT)); // Fulup need to be free when timer is done

    // everything look fine create timer structure
    TimerHandleT *timerHandle = malloc(sizeof (TimerHandleT));
    timerHandle->magic = TIMER_MAGIC;
    timerHandle->delay = delay;
    timerHandle->count = count;
    timerHandle->uid = uid;
    timerHandle->freeCB = LuaTimerCtxFree;

    // fire timer
    TimerEvtStart(source->api, timerHandle, LuaTimerSetCB, handleCb);

    // Fulup finir les timers avec handle

    // return empty error code plus timer handle
    lua_pushnil(luaState);
    lua_pushlightuserdata(luaState, timerHandle);
    return 2;
}

typedef struct {
    const char *callback;
    json_object *clientCtxJ;
    CtlSourceT *source;
} LuaClientCtxT;

static void *LuaClientCtxNew(void * handle) {

    LuaClientCtxT *clientCtx = (LuaClientCtxT*) handle;
    int count = 1;

    // push callback and client context on Lua stack
    lua_getglobal(luaState, clientCtx->callback);

    // let's Lua script know about new/free
    clientCtx->source->status = CTL_STATUS_DONE;

    // Push AFB client context on the stack
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, clientCtx->source);
    if (!afbSource)
        return NULL;

    // Push user Context
    count += LuaPushArgument(clientCtx->source, clientCtx->clientCtxJ);

    int err = lua_pcall(luaState, count, 1, 0);
    if (err) {
        AFB_ApiError(clientCtx->source->api, "LuaClientCtxNew:FAIL response=%s err=%s", json_object_to_json_string(clientCtx->clientCtxJ), lua_tostring(luaState, -1));
        return NULL;
    }

    // If function return an error status then free client context
    if (lua_tointeger(luaState, -1)) {
        free(clientCtx);
        return NULL;
    }

    return handle; // By default we are happy
}

static void LuaClientCtxFree(void * handle) {

    LuaClientCtxT *clientCtx = (LuaClientCtxT*) handle;
    int count = 1;

    if (!handle) return;

    // let's Lua script know about new/free
    lua_getglobal(luaState, clientCtx->callback);

    // set source status to notify lua script about free
    clientCtx->source->status = CTL_STATUS_FREE;

    // Push AFB client context on the stack
    LuaAfbSourceT *afbSource = LuaSourcePush(luaState, clientCtx->source);
    if (!afbSource)
        return;

    // Push user Context
    count += LuaPushArgument(clientCtx->source, clientCtx->clientCtxJ);

    int err = lua_pcall(luaState, count, LUA_MULTRET, 0);
    if (err) {
        AFB_ApiError(clientCtx->source->api, "LuaClientCtxFree:FAIL response=%s err=%s", json_object_to_json_string(clientCtx->clientCtxJ), lua_tostring(luaState, -1));
        return;
    }

    // If function return an error status then free client context
    if (lua_toboolean(luaState, -1)) {
        free(clientCtx);
        return;
    }

    return; // No return status
}

// set a context that will be free when WS is closed

static int LuaClientCtx(lua_State* luaState) {
    // Get source handle
    CtlSourceT *source = LuaSourcePop(luaState, LUA_FIRST_ARG);
    if (!source) {
        lua_error(luaState);
        return 1; // return error code
    }

    if (!AFB_ReqIsValid(source->request)) {
        lua_pushliteral(luaState, "LuaSessionSet-Syntax should be called within client request context");
        lua_error(luaState);
        return 1; // return error code
    }

    // in only one arg then we should free the clientCtx
    if (lua_gettop(luaState) == LUA_FIRST_ARG) {
        AFB_ClientCtxClear(source->request);
        lua_pushnil(luaState);
        return 1;
    }

    const char *callback = lua_tostring(luaState, LUA_FIRST_ARG + 1);
    json_object *clientCtxJ = LuaPopOneArg(source, luaState, LUA_FIRST_ARG + 2);

    if (lua_gettop(luaState) != LUA_FIRST_ARG + 2 || !clientCtxJ || !callback) {
        lua_pushliteral(luaState, "LuaClientCtx-Syntax clientCtx (source, callback, clientCtx)");
        lua_error(luaState);
        return 1; // return error code
    }

    // Allocate handle to store clientCtx and callback
    LuaClientCtxT *clientCtx = calloc(1, sizeof (LuaCbHandleT));
    clientCtx->callback = callback;
    clientCtx->clientCtxJ = clientCtxJ;
    clientCtx->source = malloc(sizeof (CtlSourceT));
    memcpy(clientCtx->source, source, sizeof (CtlSourceT)); // source if free when command return

    // push client context within session
    void *done = AFB_ClientCtxSet(source->request, 1, LuaClientCtxNew, LuaClientCtxFree, clientCtx);
    if (!done) {
        lua_pushliteral(luaState, "LuaClientCtx-Fail to allocate client context)");
        lua_error(luaState);
        return 1; // return error code
    }

    return 1;
}


// Register a new L2c list of LUA user plugin commands

void LuaL2cNewLib(luaL_Reg *l2cFunc, int count, const char *prefix) {
    // luaL_newlib(luaState, l2cFunc); macro does not work with pointer :(
    luaL_checkversion(luaState);
    lua_createtable(luaState, 0, count + 1);
    luaL_setfuncs(luaState, l2cFunc, 0);
    lua_setglobal(luaState, prefix);
}

static const luaL_Reg afbFunction[] = {
    {"timerclear", LuaTimerClear},
    {"timerget", LuaTimerGet},
    {"timerset", LuaTimerSet},
    {"notice", LuaPrintNotice},
    {"info", LuaPrintInfo},
    {"warning", LuaPrintWarning},
    {"debug", LuaPrintDebug},
    {"error", LuaPrintError},
    {"servsync", LuaAfbServiceSync},
    {"service", LuaAfbService},
    {"success", LuaAfbSuccess},
    {"fail", LuaAfbFail},
    {"subscribe", LuaAfbEventSubscribe},
    {"evtmake", LuaAfbEventMake},
    {"evtpush", LuaAfbEventPush},
    {"getuid", LuaAfbGetUid},
    {"status", LuaAfbGetStatus},
    {"context", LuaClientCtx},
    {"lockwait", LuaLockWait},
    {NULL, NULL} /* sentinel */
};

// Load Lua Interpreter

int LuaConfigExec(AFB_ApiT apiHandle) {
    int err;

    // Load LUA utils functions.
    err = LuaDoString(lua_utils, NULL);
    if (err) {
        AFB_ApiError(apiHandle, "Error loading lua_utils default functions.%s, %d", lua_utils, err);
        return -1;
    }

    err = lua_pcall(luaState, 0, 0, 0);
    if (err) {
        AFB_ApiError(apiHandle, "Error loading lua_utils default functions at pcall.%s, %d", lua_tostring(luaState, 0), err);
        return -1;
    }
    return 0;
}

// Load Lua Interpreter

int LuaConfigLoad(AFB_ApiT apiHandle) {
    static int luaLoaded = 0;
    int i;
    //int err = 0;

    // Lua loads only once
    if (luaLoaded) return 0;
    luaLoaded = 1;

    // open a new LUA interpretor
    luaState = luaL_newstate();
    if (!luaState) {
        AFB_ApiError(apiHandle, "LUA_INIT: Fail to open lua interpretor");
        free(luaState);
        return 1;
    }

    // load auxiliary libraries
    luaL_openlibs(luaState);

    // redirect print to AFB_NOTICE
    luaL_newlib(luaState, afbFunction);
    lua_setglobal(luaState, "AFB");

    // set package.path lua variable use the CONTROL_PLUGIN_PATH as it could
    // have to find external lua packages in those directories
    size_t base_len = strlen(LUA_PATH_VALUE);
    size_t spath_len = strlen(CONTROL_PLUGIN_PATH);

    int token_nb = spath_len ? 1:0;
    char *spath = strdup(CONTROL_PLUGIN_PATH);
    char *sep = spath;
    while((sep = strchr(sep, ':')) != NULL) {
        token_nb++;
        sep++;
    }

    // token + the lua glob pattern which is 7 char length
    size_t total_len = base_len + spath_len + token_nb * 7 + 1;
    char *lua_str = malloc(total_len + 1);
    strncpy(lua_str, LUA_PATH_VALUE, total_len);
    for (i = 0; i < token_nb; i++) {
        sep = strsep(&spath, ":");
        strncat(lua_str, sep, total_len - strlen(lua_str));
        strncat(lua_str, LUA_GLOB_PATTERN, total_len - strlen(lua_str));
    }
    strncat(lua_str, "'", 2);

    if(luaL_dostring(luaState, lua_str))
        printf("Fail change package.path error=%s", lua_tostring(luaState, -1));

    free(spath);
    free(lua_str);

    // initialise static magic for context
#ifndef CTX_MAGIC_VALUE
    CTX_MAGIC = CtlConfigMagicNew();
#endif

    return 0;
}
