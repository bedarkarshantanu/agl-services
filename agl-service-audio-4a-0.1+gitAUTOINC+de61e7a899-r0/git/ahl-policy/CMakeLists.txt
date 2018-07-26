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


# Add target to project dependency list
#PROJECT_TARGET_ADD(ahl-policy)

    # Define project Targets
	#    ADD_LIBRARY(${TARGET_NAME} STATIC ahl-policy.c)


	#if($ENV{AK_DEMO})    
	#add_definitions(-DAK_POLICY_DEMO)
	#endif()

    # Define target includes
	#TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
	#    PUBLIC ${GLIB_PKG_INCLUDE_DIRS}
	#)

    # Library dependencies (include updates automatically)
	#TARGET_LINK_LIBRARIES(${TARGET_NAME}
	#    ahl-utilities
	#    afb-utilities
	#)

    # Define target includes for this target client
	#TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
	#    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
	#)
