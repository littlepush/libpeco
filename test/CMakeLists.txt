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

function(add_test_target file_name)
  get_filename_component(target_name ${file_name} NAME_WE)

  message(STATUS "add target: ${target_name}")
  add_executable(${target_name} ${file_name})
  target_link_libraries(${target_name} peco)

  target_compile_options(${target_name} PRIVATE
    "$<$<CONFIG:Debug>:${PECO_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${PECO_CXX_FLAGS_RELEASE}>"
  )
  if (NOT ${PECO_HOST_WINDOWS})
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS "${PECO_LINK_FLAGS}")
  endif()

  set(_dir_output_ ${CMAKE_BINARY_DIR}/Output)
  set(_dir_debug_ ${CMAKE_BINARY_DIR}/Debug)
  set(_dir_release_ ${CMAKE_BINARY_DIR}/Release)
  
  set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${_dir_output_})
  set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
  set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})
  
  set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_dir_output_})
  set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
  set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})
  
  set_target_properties(${target_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${_dir_output_})
  set_target_properties(${target_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${_dir_release_})
  set_target_properties(${target_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${_dir_debug_})
endfunction()

# Add all test files
file(GLOB_RECURSE test_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(file ${test_files})
  message(STATUS "find: ${file}")
  add_test_target(${file})
endforeach()
