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

if (${INCLUDE_MODULE_TASK})
  add_subdirectory("task")
endif()

if (${INCLUDE_MODULE_NET})
  # Add Net and Log Module
  add_subdirectory("net")

  if (${INCLUDE_MODULE_NET_EXTENSIONS})
    add_subdirectory("extensions")
  endif()
endif()

if (NOT DEFINED PECO_STATIC)
  set(_PECO_BUILD_STATIC_OPTION OFF)
else()
  set(_PECO_BUILD_STATIC_OPTION ${PECO_STATIC})
endif()
option(PECO_BUILD_STATIC "Build Static Library, default is OFF" ${_PECO_BUILD_STATIC_OPTION})
if (PECO_BUILD_STATIC)
  set(PECO_LIB_TYPE STATIC)
else()
  set(PECO_LIB_TYPE SHARED)
endif()

add_library(peco ${PECO_LIB_TYPE} 
  ${LIBPECO_SRC_LIST} 
  ${LIBPECO_UTILS_SRC} 
  ${LIBPECO_COTASK_SRC} 
  ${LIBPECO_CONET_SRC} 
  ${LIBPECO_EXTENSIONS_SRC}
)
set_target_properties(peco PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET peco PROPERTY CXX_STANDARD 14)


if (${INCLUDE_TASK_SHARED})
  target_compile_definitions(peco PUBLIC "-DPECO_ENABLE_SHARETASK=1")
else()
  target_compile_definitions(peco PUBLIC "-DPECO_ENABLE_SHARETASK=0")
endif()

target_compile_options(peco PRIVATE
  "$<$<CONFIG:Debug>:${PECO_CXX_FLAGS_DEBUG}>"
  "$<$<CONFIG:Release>:${PECO_CXX_FLAGS_RELEASE}>"
)

if (NOT ${PECO_HOST_WINDOWS})
  set_target_properties(peco PROPERTIES LINK_FLAGS "${PECO_LINK_FLAGS}")
endif()

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
