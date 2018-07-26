###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

# Include LUA only when requested
if(CONTROL_SUPPORT_LUA)
   message(STATUS "Notice: LUA Controler Support Selected")
   set(CTL_LUA_SOURCE ctl-lua.c ctl-timer.c ctl-lua-utils.c)
   ADD_COMPILE_OPTIONS(-DCONTROL_SUPPORT_LUA)
else(CONTROL_SUPPORT_LUA)
   message(STATUS "Warning: LUA Without Support ")
endif(CONTROL_SUPPORT_LUA)

# Add target to project dependency list
PROJECT_TARGET_ADD(ctl-utilities)

    # Define project Targets
    ADD_LIBRARY(${TARGET_NAME} STATIC ctl-action.c ctl-config.c ctl-onload.c ctl-plugin.c ctl-control.c ctl-event.c ${CTL_LUA_SOURCE})

    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        afb-helpers
        ${link_libraries}
    )

    # Define target includes for this target client
    TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    )
