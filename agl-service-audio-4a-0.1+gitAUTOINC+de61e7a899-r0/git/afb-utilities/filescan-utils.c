/*
 * Copyright (C) 2016-2018 "IoT.bzh"
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
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "filescan-utils.h"

static int ScanDir(char* searchPath, CtlScanDirModeT mode, size_t extentionLen,
    const char* prefix, const char* extention, json_object* responseJ)
{
    int found = 0;
    DIR* dirHandle;
    struct dirent* dirEnt;
    dirHandle = opendir(searchPath);
    if (!dirHandle) {
        AFB_DEBUG("CONFIG-SCANNING dir=%s not readable", searchPath);
        return 0;
    }

    //AFB_NOTICE ("CONFIG-SCANNING:ctl_listconfig scanning: %s", searchPath);
    while ((dirEnt = readdir(dirHandle)) != NULL) {

        // recursively search embedded directories ignoring any directory starting by '.' or '_'
        if (dirEnt->d_type == DT_DIR && mode == CTL_SCAN_RECURSIVE) {
            char newpath[CONTROL_MAXPATH_LEN];
            if (dirEnt->d_name[0] == '.' || dirEnt->d_name[0] == '_')
                continue;

            strncpy(newpath, searchPath, sizeof(newpath));
            strncat(newpath, "/", sizeof(newpath) - strlen(newpath) - 1);
            strncat(newpath, dirEnt->d_name, sizeof(newpath) - strlen(newpath) - 1);
            found += ScanDir(newpath, mode, extentionLen, prefix, extention, responseJ);
            continue;
        }

        // Unknown type is accepted to support dump filesystems
        if (dirEnt->d_type == DT_REG || dirEnt->d_type == DT_UNKNOWN) {

            // check prefix and extention
            ssize_t extentionIdx = strlen(dirEnt->d_name) - extentionLen;
            if (extentionIdx <= 0)
                continue;
            if (prefix && strncasecmp(dirEnt->d_name, prefix, strlen(prefix)))
                continue;
            if (extention && strcasecmp(extention, &dirEnt->d_name[extentionIdx]))
                continue;

            struct json_object* pathJ = json_object_new_object();
            json_object_object_add(pathJ, "fullpath", json_object_new_string(searchPath));
            json_object_object_add(pathJ, "filename", json_object_new_string(dirEnt->d_name));
            json_object_array_add(responseJ, pathJ);
            found++;
        }
    }
    closedir(dirHandle);
    return found;
}

// List Avaliable Configuration Files
json_object* ScanForConfig(const char* searchPath, CtlScanDirModeT mode, const char* prefix, const char* extention)
{
    json_object* responseJ = json_object_new_array();
    char* dirPath;
    char* dirList;
    size_t extentionLen = 0;
    int count = 0;

    if (!searchPath)
        return responseJ;

    dirList = strdup(searchPath);

    if (extention)
        extentionLen = strlen(extention);

    // loop recursively on dir
    for (dirPath = strtok(dirList, ":"); dirPath && *dirPath; dirPath = strtok(NULL, ":")) {
        count += ScanDir(dirPath, mode, extentionLen, prefix, extention, responseJ);
    }
    if (count == 0) {
        json_object_put(responseJ);
        free(dirList);
        return NULL;
    }
    free(dirList);
    return (responseJ);
}

const char* GetMiddleName(const char* name)
{
    char* fullname = strdup(name);

    for (int idx = 0; fullname[idx] != '\0'; idx++) {
        if (fullname[idx] == '-') {
            int start;
            start = idx + 1;
            for (int jdx = start;; jdx++) {
                if (fullname[jdx] == '-' || fullname[jdx] == '.' || fullname[jdx] == '\0') {
                    fullname[jdx] = '\0';
                    return &fullname[start];
                }
            }
            break;
        }
    }
    return "";
}

const char* GetBinderName()
{
    static char* binderName = NULL;

    if (binderName)
        return binderName;

    binderName = getenv("AFB_BINDER_NAME");
    if (!binderName) {
        static char psName[17];
        // retrieve binder name from process name afb-name-trailer
        prctl(PR_GET_NAME, psName, NULL, NULL, NULL);
        binderName = (char*)GetMiddleName(psName);
    }

    return binderName;
}

char* GetBindingDirPath(struct afb_dynapi* dynapi)
{
    // A file description should not be greater than 999.999.999
    char fd_link[CONTROL_MAXPATH_LEN];
    char retdir[CONTROL_MAXPATH_LEN];
    ssize_t len;

#if((AFB_BINDING_VERSION == 0 || AFB_BINDING_VERSION == 3) && defined(AFB_BINDING_WANT_DYNAPI))
    if (!dynapi)
        return NULL;
    sprintf(fd_link, "/proc/self/fd/%d", afb_dynapi_rootdir_get_fd(dynapi));
#else
    sprintf(fd_link, "/proc/self/fd/%d", afb_daemon_rootdir_get_fd());
#endif

    if ((len = readlink(fd_link, retdir, sizeof(retdir) - 1)) == -1) {
        perror("lstat");
        strncpy(retdir, "/tmp", CONTROL_MAXPATH_LEN - 1);
    } else {
        retdir[len] = '\0';
    }

    return strndup(retdir, sizeof(retdir));
}
