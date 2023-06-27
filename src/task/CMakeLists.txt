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

unset(LIBPECO_COTASK_SRC CACHE)
unset(LIBPECO_COTASK_PUBLIC CACHE)

file(GLOB PECO_COTASK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.h*" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB PECO_COTASK_IMPL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/impl/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/impl/*.cpp")
file(GLOB PECO_COTASK_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

if (${INCLUDE_TASK_SHARED})
  file(GLOB PECO_COTASK_SHARED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shared/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/shared/*.cpp")
  file(GLOB PECO_COTASK_SHARED_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/shared/*.h")
else()
  set(PECO_COTASK_SHARED_SRC "")
  set(PECO_COTASK_SHARED_PUBLIC "")
endif()

if(${PECO_PLATFORM_WINDOWS})
  file(GLOB PECO_COTASK_PLATFROM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/windows/*.cpp")
elseif(${PECO_PLATFORM_APPLE})
  file(GLOB PECO_COTASK_PLATFROM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/apple/*.cpp")
elseif(${PECO_PLATFORM_LINUX} OR ${PECO_PLATFORM_ANDROID})
  file(GLOB PECO_COTASK_PLATFROM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/linux/*.cpp")
else()
  set(PECO_COTASK_PLATFROM_SRC "")
endif()

set(LIBPECO_COTASK_SRC ${PECO_COTASK_SRC} ${PECO_COTASK_PLATFROM_SRC} ${PECO_COTASK_IMPL_SRC} ${PECO_COTASK_SHARED_SRC} CACHE STRING "")
set(LIBPECO_COTASK_PUBLIC ${PECO_COTASK_PUBLIC} ${PECO_COTASK_SHARED_PUBLIC} CACHE STRING "")
