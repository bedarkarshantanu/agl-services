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
 *
 * reference:
 *   amixer contents; amixer controls;
 *   http://www.tldp.org/HOWTO/Alsa-sound-6.html
 */

#ifndef FILESCAN_UTILS_H
#define FILESCAN_UTILS_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <afb/afb-binding.h>

#if((AFB_BINDING_VERSION == 0) && defined(AFB_BINDING_WANT_DYNAPI))
    extern  afb_dynapi *AFB_default;
    #define AFB_DEBUG(...) AFB_DYNAPI_DEBUG(AFB_default, __VA_ARGS__)
#endif

#include <json-c/json.h>

#ifndef CONTROL_MAXPATH_LEN
  #define CONTROL_MAXPATH_LEN 255
#endif

/**
 * @brief enum describing which mode to use in ScanForConfig function
 *
 */
typedef enum {
  CTL_SCAN_FLAT=0, /**< Simple flat search */
  CTL_SCAN_RECURSIVE=1, /**< Recursive search */
} CtlScanDirModeT;

/**
 * @brief Get rid of the binder name prefix 'afbd-'
 *
 * @param name will be typically the full binder name
 *
 * @return const char*
 */
const char *GetMiddleName(const char *name);

/**
 * @brief Get the Binder Name without the prefix set by the AGL appfw 'afbd-'
 *
 * @return const char* the Binder name without the prefix.
 */
const char *GetBinderName();

/**
 * @brief Scan a directory searching all files matching pattern:
 * 'prefix*extention'.
 *
 * @param searchPath directory where to begin the searching
 * @param mode either or not the search will be recursive.
 * @param prefix file prefix that will be looking for
 * @param extention file extention that will be looking for
 *
 * @return json_object* a json_object array of object with 2 parts a 'fullpath'
 * describing the fullpath to reach the file and 'filename' containing the
 * matched files.
 */
json_object* ScanForConfig (const char* searchPath, CtlScanDirModeT mode, const char *prefix, const char *extension);

/**
 * @brief Get the Binding root directory file descriptor object
 *
 * @param dynapi : Could be NULL if you don't use dynamic api
 *
 * @return char* string representing the path to binding root directory.
 */
char *GetBindingDirPath(struct afb_dynapi *dynapi);

#ifdef __cplusplus
    }
#endif

#endif /* FILESCAN_UTILS_H */
