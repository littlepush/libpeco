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

unset(LIBPECO_INCLUDE_PATH CACHE)
unset(LIBPECO_SRC_LIST CACHE)
unset(LIBPECO_PUBLIC_HEADER CACHE)

set(LIBPECO_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "")

file(GLOB PECO_PECO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB PECO_PECO_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set(LIBPECO_SRC_LIST "${CMAKE_CURRENT_SOURCE_DIR}/peco.h;${PECO_PECO_SRC}" CACHE STRING "")
set(LIBPECO_PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/peco.h;${PECO_PECO_PUBLIC}" CACHE STRING "")

if (${INCLUDE_MODULE_TASK})
  set(PECO_PUBLIC_HEADER_INCLUDE_TASK "#include \"task.h\"")
else()
  set(PECO_PUBLIC_HEADER_INCLUDE_TASK "")
endif()

if(${INCLUDE_MODULE_NET})
  set(PECO_PUBLIC_HEADER_INCLUDE_NET "#include \"net.h\"")
  if(${INCLUDE_MODULE_NET_EXTENSIONS})
    set(PECO_PUBLIC_HEADER_INCLUDE_NET_EXT "#include \"net_ext.h\"")
  else()
    set(PECO_PUBLIC_HEADER_INCLUDE_NET_EXT "")
  endif()
else()
  set(PECO_PUBLIC_HEADER_INCLUDE_NET "")
  set(PECO_PUBLIC_HEADER_INCLUDE_NET_EXT "")
endif()

# Add Source
add_subdirectory("basic")

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

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/peco.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/peco.h")

# Config for install
set(LIBPECO_PUBLIC_HEADERS 
  ${LIBPECO_PUBLIC_HEADER} 
  ${LIBPECO_COTASK_PUBLIC} 
  ${LIBPECO_CONET_PUBLIC} 
  ${LIBPECO_UTILS_PUBLIC}
  ${LIBPECO_EXTENSIONS_PUBLIC})

install(TARGETS peco)
foreach(header ${LIBPECO_PUBLIC_HEADERS})
  file(RELATIVE_PATH relative_file "${CMAKE_CURRENT_SOURCE_DIR}" ${header})
  get_filename_component(dir "${relative_file}" PATH)
  install(FILES ${header} DESTINATION "include/${dir}")
endforeach()
