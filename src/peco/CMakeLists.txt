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

add_subdirectory("utils")
add_subdirectory("cotask")
add_subdirectory("conet")
add_subdirectory("corlog")

option(PECO_BUILD_STATIC "Build Static Library, default is OFF" OFF)
if (PECO_BUILD_STATIC)
    set(PECO_LIB_TYPE STATIC)
else()
    set(PECO_LIB_TYPE SHARED)
endif()

add_library(peco ${PECO_LIB_TYPE} ${LIBPECO_SRC_LIST} ${LIBPECO_UTIL_SRC} ${LIBPECO_COTASK_SRC} ${LIBPECO_CONET_SRC} ${LIBPECO_CORLOG_SRC})
set_target_properties(peco PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(peco PUBLIC ${LIBPECO_INCLUDE_PATH})

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
    set(PECO_CXX_FLAGS_DEBUG "/MP;/W3;/ZI;/TP;/std:c++14;/DWIN32;/bigobj;/Od;/MTd;/DDEBUG=1")
    set(PECO_CXX_FLAGS_RELEASE "/MP;/W3;/ZI;/TP;/std:c++14;/DWIN32;/bigobj;/DNDEBUG=1;/DRELEASE=1;/MT;/Ox")

    if ( CMAKE_CL_64 )
        set(PECO_CXX_FLAGS_DEBUG "${PECO_CXX_FLAGS_DEBUG};/DWIN64")
        set(PECO_CXX_FLAGS_RELEASE "${PECO_CXX_FLAGS_RELEASE}/DWIN64")
    endif()
else()
    set(PECO_CXX_FLAGS_DEBUG "-fPIC;-fmerge-all-constants;-O0;-g;-DDEBUG=1;-std=c++14;-Wall;-Werror;-Wno-unneeded-internal-declaration")
    set(PECO_CXX_FLAGS_RELEASE "-fPIC;-fmerge-all-contants;-O3;-DNDEBUG=1;-DRELEASE=1;-std=c++14;-Wall;-Werror;-Wno-unneeded-internal-declaration")
    set(PECO_LINK_FLAGS "-lresolv")
    if (PECO_BUILD_STATIC)
        set(PECO_LINK_FLAGS "${PECO_LINK_FLAGS} -static")
    endif()

    if (PECO_HOST_LINUX AND (NOT PECO_PLATFORM_ANDROID))
        set(PECO_LINK_FLAGS "${PECO_LINK_FLAGS} -pthread")
    endif()
    set_target_properties(peco PROPERTIES LINK_FLAGS "${PECO_LINK_FLAGS}")
endif()

target_compile_options(peco PRIVATE
    "$<$<CONFIG:Debug>:${PECO_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${PECO_CXX_FLAGS_RELEASE}>"
)

find_package(OpenSSL REQUIRED)
target_link_libraries(peco PRIVATE OpenSSL::Crypto)
target_link_libraries(peco PRIVATE OpenSSL::SSL)

find_package(ZLIB REQUIRED)
target_link_libraries(peco PRIVATE ZLIB::ZLIB)

set(_dir_output_ ${CMAKE_BINARY_DIR}/Output)
set(_dir_debug_ ${CMAKE_BINARY_DIR}/Debug)
set(_dir_release_ ${CMAKE_BINARY_DIR}/Release)

set_target_properties(peco PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_dir_output_})
set_target_properties(peco PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
set_target_properties(peco PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})

set_target_properties(peco PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_dir_output_})
set_target_properties(peco PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
set_target_properties(peco PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})

set_target_properties(peco PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${_dir_output_})
set_target_properties(peco PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
set_target_properties(peco PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})

# Add Include Path
target_include_directories(peco PUBLIC ${LIBPECO_INCLUDE_PATH})
