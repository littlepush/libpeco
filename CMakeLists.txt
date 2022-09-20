# MIT License

# Copyright (c) 2019 Push Chen

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.10.0)

if (${CMAKE_VERSION} VERSION_GREATER "3.13.0")
  cmake_policy(SET CMP0079 NEW)
endif()

cmake_policy(SET CMP0042 NEW)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# RTC Master Project
project(libpeco)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Group sources into different source group according to the file path
function(peco_group_sources target begin_dir)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${begin_dir}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

if (NOT DEFINED PECO_BUILD_TASK)
  set(INCLUDE_MODULE_TASK ON)
else()
  set(INCLUDE_MODULE_TASK ${PECO_BUILD_TASK})
endif()
if (${INCLUDE_MODULE_TASK})
  if (NOT DEFINED PECO_BUILD_NET)
    set(INCLUDE_MODULE_NET ON)
  else()
    set(INCLUDE_MODULE_NET ${PECO_BUILD_NET})
  endif()

  if(${INCLUDE_MODULE_NET})
    if (NOT DEFINED PECO_BUILD_NET_EXTENSIONS)
      set(INCLUDE_MODULE_NET_EXTENSIONS ON)
    else()
      set(INCLUDE_MODULE_NET_EXTENSIONS ${PECO_BUILD_NET_EXTENSIONS})
    endif()
  else()
    set(INCLUDE_MODULE_NET_EXTENSIONS OFF)
  endif()
else()
  set(INCLUDE_MODULE_NET OFF)
endif()

# Check current platform
set(PECO_HOST_WINDOWS FALSE)
set(PECO_HOST_MACOS FALSE)
set(PECO_HOST_LINUX FALSE)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  set(PECO_HOST_WINDOWS TRUE)
  message(STATUS "Compile host: Windows")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
  set(PECO_HOST_MACOS TRUE)
  message(STATUS "Compile host: MacOS")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  set(PECO_HOST_LINUX TRUE)
  message(STATUS "Compile host: Linux")
else()
  message(WARNING "Cannot recognized compile host, set default to Linux")
  set(PECO_HOST_LINUX TRUE)
endif()

set(PECO_PLATFORM_ANDROID FALSE)
set(PECO_PLATFORM_LINUX FALSE)
set(PECO_PLATFORM_WINDOWS FALSE)
set(PECO_PLATFORM_APPLE FALSE)

if (ANDROID)
  set(PECO_PLATFORM_ANDROID TRUE)
elseif(PECO_HOST_WINDOWS)
  set(PECO_PLATFORM_WINDOWS TRUE)
elseif(PECO_HOST_MACOS)
  set(PECO_PLATFORM_APPLE TRUE)
else()
  set(PECO_PLATFORM_LINUX TRUE)
endif()

if (PECO_PLATFORM_WINDOWS)
  set(PECO_CXX_FLAGS_DEBUG "/MP;/W3;/ZI;/TP;/DWIN32;/bigobj;/Od;/MTd;/DDEBUG=1")
  set(PECO_CXX_FLAGS_RELEASE "/MP;/W3;/ZI;/TP;/DWIN32;/bigobj;/DNDEBUG=1;/DRELEASE=1;/MT;/Ox")

  if ( CMAKE_CL_64 )
    set(PECO_CXX_FLAGS_DEBUG "${PECO_CXX_FLAGS_DEBUG};/DWIN64")
    set(PECO_CXX_FLAGS_RELEASE "${PECO_CXX_FLAGS_RELEASE}/DWIN64")
  endif()

  add_definitions(NOMINMAX=1)
else()
  set(PECO_CXX_FLAGS_DEBUG "-fPIC;-fmerge-all-constants;-O0;-g;-DDEBUG=1;-Wall;-Werror;-Wno-unneeded-internal-declaration")
  set(PECO_CXX_FLAGS_RELEASE "-fPIC;-fmerge-all-constants;-O3;-DNDEBUG=1;-DRELEASE=1;-Wall;-Werror;-Wno-unneeded-internal-declaration")
  set(PECO_LINK_FLAGS "-lresolv")
  if (PECO_BUILD_STATIC)
    set(PECO_LINK_FLAGS "${PECO_LINK_FLAGS} -static")
  endif()

  if (PECO_HOST_LINUX AND (NOT PECO_PLATFORM_ANDROID))
    set(PECO_LINK_FLAGS "${PECO_LINK_FLAGS} -pthread")
  endif()
endif()

# Add Source
add_subdirectory(src)

# Group the source
peco_group_sources(peco "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT DEFINED PECO_ENABLE_TEST)
  set(_PECO_BUILD_ENABLE_TEST ON)
else()
  set(_PECO_BUILD_ENABLE_TEST ${PECO_ENABLE_TEST})
endif()
option(PECO_BUILD_TEST "Build test target, default is ON" ${_PECO_BUILD_ENABLE_TEST})
if (${PECO_BUILD_TEST})
  add_subdirectory(test)
endif()
