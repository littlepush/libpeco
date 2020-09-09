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

unset(LIBPECO_CONET_SRC CACHE)
unset(LIBPECO_CONET_PUBLIC CACHE)

file(GLOB_RECURSE PECO_CONET_UTILS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp")
file(GLOB_RECURSE PECO_CONET_UTILS_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h")

file(GLOB_RECURSE PECO_CONET_PROTO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/protos/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/protos/*.cpp")
file(GLOB_RECURSE PECO_CONET_PROTO_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/protos/*.h")

file(GLOB_RECURSE PECO_CONET_CONNS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/conns/*.h*" "${CMAKE_CURRENT_SOURCE_DIR}/conns/*.cpp")
file(GLOB_RECURSE PECO_CONET_CONNS_PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/conns/*.h*")

set(LIBPECO_CONET_SRC ${PECO_CONET_UTILS_SRC} ${PECO_CONET_PROTO_SRC} ${PECO_CONET_CONNS_SRC} CACHE STRING "")
set(LIBPECO_CONET_PUBLIC ${PECO_CONET_UTILS_PUBLIC} ${PECO_CONET_PROTO_PUBLIC} ${PECO_CONET_CONNS_PUBLIC} CACHE STRING "")
