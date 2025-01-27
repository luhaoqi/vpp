# Copyright (c) 2017 Cisco and/or its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(COMMON_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/common)

set(COMMON_SOURCE_FILES
    common/common.c
    common/sender.c
    common/responder.c
    common/packet_handler.c
    common/icmp_proto.c
)

list(APPEND EXAMPLES_LIST
  loopback/main.c
  icmp_responder/main.c
  test_app/main.c
)

foreach (EXAMPLE_SRC ${EXAMPLES_LIST})
  string(FIND ${EXAMPLE_SRC} "/" INDEX)
  string(SUBSTRING ${EXAMPLE_SRC} 0 ${INDEX} EXECUTABLE)
  add_executable(${EXECUTABLE} ${COMMON_SOURCE_FILES} ${EXAMPLE_SRC} )
  target_include_directories(${EXECUTABLE} PRIVATE $<BUILD_INTERFACE:${COMMON_HEADERS}>)
  target_link_libraries(${EXECUTABLE} memif ${CMAKE_THREAD_LIBS_INIT})
endforeach()

list(APPEND BW_LIST
  bwtest/master.c
  bwtest/worker.c
)

foreach (BW_SRC ${BW_LIST})
  string(FIND ${BW_SRC} "." END_INDEX)
  string(FIND ${BW_SRC} "/" INDEX)
  math(EXPR INDEX "${INDEX} + 1")
  math(EXPR LEN "${END_INDEX} - ${INDEX}")
  string(SUBSTRING ${BW_SRC} ${INDEX} ${LEN} EXECUTABLE)
  add_executable(${EXECUTABLE} ${COMMON_SOURCE_FILES} ${BW_SRC} )
  target_include_directories(${EXECUTABLE} PRIVATE $<BUILD_INTERFACE:${COMMON_HEADERS}>)

  # target_link_libraries(${EXECUTABLE} memif ${CMAKE_THREAD_LIBS_INIT})
  # target_link_libraries(${EXECUTABLE} PRIVATE memif_static)  # 静态链接 memif
  # list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_BINARY_DIR}/build/src")
  target_link_libraries(${EXECUTABLE} PRIVATE memif_object)  

  target_compile_options(${EXECUTABLE} PRIVATE -g)
endforeach()
