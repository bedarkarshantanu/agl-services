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
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "filescan-utils.h"
#include "ctl-config.h"


// Load control config file

int CtlConfigMagicNew() {
  static int InitRandomDone=0;

  if (!InitRandomDone) {
    struct timeval tv;
    InitRandomDone=1;
    gettimeofday(&tv,NULL);
    srand ((int)tv.tv_usec);
  }

  return ((long)rand());
}

json_object* CtlConfigScan(const char *dirList, const char *prefix) {
    char controlFile[CONTROL_MAXPATH_LEN];
    const char *binderName = GetBinderName();

    controlFile[CONTROL_MAXPATH_LEN - 1] = '\0';

    if(prefix && prefix[0] != '\0') {
        strncpy(controlFile, prefix, CONTROL_MAXPATH_LEN - 1);
        strncat(controlFile, "-", CONTROL_MAXPATH_LEN - strlen(controlFile) - 1);
        strncat(controlFile, binderName, CONTROL_MAXPATH_LEN - strlen(controlFile) - 1);
    }
    else {
        strncpy(controlFile, binderName, CONTROL_MAXPATH_LEN - 1);
    }
    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, ".json");

    return responseJ;
}

char* ConfigSearch(AFB_ApiT apiHandle, json_object *responseJ) {
    // We load 1st file others are just warnings
    size_t p_length;
    char *filepath = NULL;
    const char *filename;
    const char*fullpath;

    for (int index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        int err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ApiError(apiHandle, "CTL-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
        }

        if (index == 0) {
            p_length = strlen(fullpath) + 1 + strlen(filename);
            filepath = malloc(p_length + 1);

            strncpy(filepath, fullpath, p_length);
            strncat(filepath, "/", p_length - strlen(filepath));
            strncat(filepath, filename, p_length - strlen(filepath));
        }
        else {
            AFB_ApiWarning(apiHandle, "CTL-INIT JSON file found but not used : %s/%s", fullpath, filename);
        }
    }

    json_object_put(responseJ);
    return filepath;
}

char* CtlConfigSearch(AFB_ApiT apiHandle, const char *dirList, const char *prefix) {
    // search for default dispatch config file
    json_object* responseJ = CtlConfigScan (dirList, prefix);

    if(responseJ)
        return ConfigSearch(apiHandle, responseJ);

    return NULL;
}

static int DispatchRequireOneApi(AFB_ApiT apiHandle, json_object * bindindJ) {
    const char* requireBinding = json_object_get_string(bindindJ);
    int err = AFB_RequireApi(apiHandle, requireBinding, 1);
    if (err) {
        AFB_ApiWarning(apiHandle, "CTL-LOAD-CONFIG:REQUIRE Fail to get=%s", requireBinding);
    }

    return err;
}

/**
 * @brief Best effort to initialise everything before starting
 * Call afb_require_api at the first call or if there was an error because
 * the CtlConfigExec could be called anywhere and not in binding init.
 * So you could call this function at init time.
 *
 * @param apiHandle : a afb_daemon api handle, see AFB_ApiT in afb_definitions.h
 * @param requireJ : json_object array of api name required.
 */
void DispatchRequireApi(AFB_ApiT apiHandle, json_object * requireJ) {
    static int init = 0, err = 0;
    int idx;

    if ( (! init || err) && requireJ) {
        if (json_object_get_type(requireJ) == json_type_array) {
            for (idx = 0; idx < json_object_array_length(requireJ); idx++) {
                err += DispatchRequireOneApi(apiHandle, json_object_array_get_idx(requireJ, idx));
            }
        } else {
            err += DispatchRequireOneApi(apiHandle, requireJ);
        }
    }

    init = 1;
}

int CtlConfigExec(AFB_ApiT apiHandle, CtlConfigT *ctlConfig) {

    DispatchRequireApi(apiHandle, ctlConfig->requireJ);
#ifdef CONTROL_SUPPORT_LUA
    // load static LUA utilities
    LuaConfigExec(apiHandle);
#endif

    // Loop on every section and process config
    int errcount=0;
    for (int idx = 0; ctlConfig->sections[idx].key != NULL; idx++) {

        if (!ctlConfig->sections[idx].loadCB)
            AFB_ApiNotice(apiHandle, "CtlConfigLoad: notice empty section '%s'", ctlConfig->sections[idx].key);
        else if (json_object_object_get_ex(ctlConfig->configJ, ctlConfig->sections[idx].key, NULL))
            errcount += ctlConfig->sections[idx].loadCB(apiHandle, &ctlConfig->sections[idx], NULL);
    }

    return errcount;
}

CtlConfigT *CtlLoadMetaData(AFB_ApiT apiHandle, const char* filepath) {
    json_object *ctlConfigJ;
    CtlConfigT *ctlHandle=NULL;
    int err;

    // Load JSON file
    ctlConfigJ = json_object_from_file(filepath);
    if (!ctlConfigJ) {
        AFB_ApiError(apiHandle, "CTL-LOAD-CONFIG Not invalid JSON %s ", filepath);
        return NULL;
    }

    AFB_ApiInfo(apiHandle, "CTL-LOAD-CONFIG: loading config filepath=%s", filepath);

    json_object *metadataJ;
    int done = json_object_object_get_ex(ctlConfigJ, "metadata", &metadataJ);
    if (done) {
        ctlHandle = calloc(1, sizeof (CtlConfigT));
        err = wrap_json_unpack(metadataJ, "{ss,ss,ss,s?s,s?o,s?s,s?s !}",
                "uid", &ctlHandle->uid,
                "version", &ctlHandle->version,
                "api", &ctlHandle->api,
                "info", &ctlHandle->info,
                "require", &ctlHandle->requireJ,
                "author", &ctlHandle->author,
                "date", &ctlHandle->date);
        if (err) {
            AFB_ApiError(apiHandle, "CTL-LOAD-CONFIG:METADATA Missing something uid|api|version|[info]|[require]|[author]|[date] in:\n-- %s", json_object_get_string(metadataJ));
            free(ctlHandle);
            return NULL;
        }
    }

    ctlHandle->configJ = ctlConfigJ;
    return ctlHandle;
}

void wrap_json_array_add(void* array, json_object *val) {
    json_object_array_add(array, (json_object*)val);
}

json_object* LoadAdditionalsFiles(AFB_ApiT apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ);

json_object* CtlUpdateSectionConfig(AFB_ApiT apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ, json_object *filesJ) {

    json_object *sectionArrayJ;
    char *oneFile = NULL;
    const char *bindingPath = GetBindingDirPath(apiHandle);

    if(! json_object_is_type(sectionJ, json_type_array)) {
        sectionArrayJ = json_object_new_array();
        if(json_object_object_length(sectionJ) > 0)
            json_object_array_add(sectionArrayJ, sectionJ);
    }
    else sectionArrayJ = sectionJ;

    json_object_get(sectionJ);
    json_object_object_del(ctlHandle->configJ, key);
    json_object_object_add(ctlHandle->configJ, key, sectionArrayJ);

    if (json_object_get_type(filesJ) == json_type_array) {
        int length = (int)json_object_array_length(filesJ);
        for (int idx=0; idx < length; idx++) {
            json_object *oneFileJ = json_object_array_get_idx(filesJ, idx);
            json_object *responseJ = ScanForConfig(CONTROL_CONFIG_PATH ,CTL_SCAN_RECURSIVE, json_object_get_string(oneFileJ), ".json");
            responseJ = responseJ ? responseJ:
                ScanForConfig(bindingPath, CTL_SCAN_RECURSIVE, json_object_get_string(oneFileJ), ".json");
            if(!responseJ) {
                AFB_ApiError(apiHandle, "No config files found in search path. No changes has been made\n -- %s\n -- %s", CONTROL_CONFIG_PATH, bindingPath);
                return sectionArrayJ;
            }
            oneFile = ConfigSearch(apiHandle, responseJ);
            if (oneFile) {
                json_object *newSectionJ, *newFileJ = json_object_from_file(oneFile);
                json_object_object_get_ex(newFileJ, key, &newSectionJ);
                json_object_get(newSectionJ);
                json_object_put(newFileJ);
                LoadAdditionalsFiles(apiHandle, ctlHandle, key, newSectionJ);
                json_object_object_get_ex(ctlHandle->configJ, key, &sectionArrayJ);
                wrap_json_optarray_for_all(newSectionJ, wrap_json_array_add, sectionArrayJ);
            }
        }
    } else {
        json_object *responseJ = ScanForConfig(CONTROL_CONFIG_PATH ,CTL_SCAN_RECURSIVE, json_object_get_string(filesJ), ".json");
        responseJ = responseJ ? responseJ:
            ScanForConfig(bindingPath, CTL_SCAN_RECURSIVE, json_object_get_string(filesJ), ".json");
        if(!responseJ) {
            AFB_ApiError(apiHandle, "No config files found in search path. No changes has been made\n -- %s\n -- %s", CONTROL_CONFIG_PATH, bindingPath);
            return sectionArrayJ;
        }
        oneFile = ConfigSearch(apiHandle, responseJ);
        json_object *newSectionJ = json_object_from_file(oneFile);
        LoadAdditionalsFiles(apiHandle, ctlHandle, key, newSectionJ);
        wrap_json_optarray_for_all(newSectionJ, wrap_json_array_add, sectionArrayJ);
    }

    free(oneFile);
    return sectionArrayJ;
}

json_object* LoadAdditionalsFiles(AFB_ApiT apiHandle, CtlConfigT *ctlHandle, const char *key, json_object *sectionJ)
{
    json_object *filesJ, *filesArrayJ = json_object_new_array();
    if (json_object_get_type(sectionJ) == json_type_array) {
        int length = (int)json_object_array_length(sectionJ);
        for (int idx=0; idx < length; idx++) {
            json_object *obj = json_object_array_get_idx(sectionJ, idx);
            int hasFiles = json_object_object_get_ex(obj, "files", &filesJ);
            if(hasFiles) {
                // Clean files key as we don't want to make infinite loop
                json_object_get(filesJ);
                json_object_object_del(obj, "files");
                if(json_object_is_type(filesJ, json_type_array))
                    wrap_json_array_for_all(filesJ, wrap_json_array_add, filesArrayJ);
                else
                    json_object_array_add(filesArrayJ, filesJ);
            }
        }
    } else {
        int hasFiles = json_object_object_get_ex(sectionJ, "files", &filesJ);
        if(hasFiles) {
            // Clean files key as we don't want to make infinite loop
            json_object_get(filesJ);
            json_object_object_del(sectionJ, "files");
            if(json_object_is_type(filesJ, json_type_array))
                filesArrayJ = filesJ;
            else
                json_object_array_add(filesArrayJ, filesJ);
        }
    }

    if(json_object_array_length(filesArrayJ) > 0)
        sectionJ = CtlUpdateSectionConfig(apiHandle, ctlHandle, key, sectionJ, filesArrayJ);
    json_object_put(filesArrayJ);
    return sectionJ;
}

int CtlLoadSections(AFB_ApiT apiHandle, CtlConfigT *ctlHandle, CtlSectionT *sections) {
    int err;

#ifdef CONTROL_SUPPORT_LUA
    err= LuaConfigLoad(apiHandle);
    if (err)
        return 1;
#endif

    err = 0;
    ctlHandle->sections = sections;
    for (int idx = 0; sections[idx].key != NULL; idx++) {
        json_object * sectionJ;
        int done = json_object_object_get_ex(ctlHandle->configJ, sections[idx].key, &sectionJ);
        if (done) {
            json_object* updatedSectionJ = LoadAdditionalsFiles(apiHandle, ctlHandle, sections[idx].key, sectionJ);
            err += sections[idx].loadCB(apiHandle, &sections[idx], updatedSectionJ);
        }
    }
    if (err)
        return 1;

    return 0;
}
