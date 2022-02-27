/*
    sysinfo.h
    libpeco
    2019-06-04
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#ifndef PECO_SYS_INFO_H__
#define PECO_SYS_INFO_H__

#include "peco/pecostd.h"
#include <vector>

namespace peco {

// Get cpu count
size_t cpu_count();

// Get CPU Load
std::vector< float > cpu_loads();

// Get self's cpu usage
float cpu_usage();

// Total Memory
uint64_t total_memory();

// Free Memory
uint64_t free_memory();

// Memory Usage
uint64_t memory_usage();

// Get current process's name
const std::string & process_name();

} // namespace peco

#endif

// Push Chen
//
