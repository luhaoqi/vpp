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

set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND MEMIF_HEADERS
  libmemif.h
  memif.h
)

list(APPEND MEMIF_PRIVATE_HEADERS
  memif_private.h
  socket.h
)

list(APPEND MEMIF_SOURCES
  main.c
  socket.c
)

include_directories(${HEADERS_DIR})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
add_library(memif SHARED ${MEMIF_SOURCES})
target_link_libraries(memif ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(memif PUBLIC MEMIF_CACHELINE_SIZE=${LIBMEMIF_CACHELINE_SIZE})

find_library(LIB_BSD bsd)
if(LIB_BSD)
  add_compile_definitions(HAS_LIB_BSD)
  target_link_libraries(memif ${LIB_BSD})
endif()

# Create the static library (`.a`)
add_library(memif_static STATIC ${MEMIF_SOURCES})
target_link_libraries(memif_static PRIVATE ${CMAKE_THREAD_LIBS_INIT})  # Link static libraries to required libraries
target_compile_definitions(memif_static PUBLIC MEMIF_CACHELINE_SIZE=${LIBMEMIF_CACHELINE_SIZE})
target_include_directories(memif_static PRIVATE ${HEADERS_DIR})  # Include headers for static library

# Create the object library 
add_library(memif_object OBJECT ${MEMIF_SOURCES})
target_link_libraries(memif_object PRIVATE ${CMAKE_THREAD_LIBS_INIT})  
target_compile_definitions(memif_object PUBLIC MEMIF_CACHELINE_SIZE=${LIBMEMIF_CACHELINE_SIZE})
target_include_directories(memif_object PRIVATE ${HEADERS_DIR})  


foreach(file ${MEMIF_HEADERS})
  get_filename_component(dir ${file} DIRECTORY)
     install(
	FILES ${file}
	DESTINATION include/${lib}/${dir}
	COMPONENT libmemif-dev
      )
endforeach()

# 安装库（共享和静态）
install(TARGETS memif DESTINATION lib COMPONENT libmemif)
install(TARGETS memif_static DESTINATION lib COMPONENT libmemif)