/*
    mustd.hpp
    mumake
    2016-02-3
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

#ifndef PZC_LIB_C_PZCSTD_HPP__
#define PZC_LIB_C_PZCSTD_HPP__

#include <peco/utils/mustd.h>

/*
    C++ Standard Files
 */
#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <functional>
#include <ctime>
#include <chrono>
#include <mutex>
#include <sstream>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <stdexcept>

#if defined(__GLIBC__) || defined (__GLIBCPP__) || defined (__clang__)
#define __LIBPECO_USE_GNU__
#endif

/* Name space */
using namespace std;
using namespace std::chrono;

namespace mu
{
    template< typename T >
    class copy2move {
        T           data_;
    public:
        copy2move<T>(T&& rdata) : data_(rdata) {}
        copy2move<T>(const copy2move<T>& ref) 
            : data_(move(const_cast< copy2move<T>& >(ref).data_)) {}
        copy2move<T>(copy2move<T>&& rref) : data_(move(rref.data_)) {}
        copy2move<T>& operator = (const copy2move<T>& ref) {
            if ( this != &ref ) data_ = move(const_cast< copy2move<T>& >(ref).data_);
            return *this;
        }
        copy2move<T>& operator = (copy2move<T>&& rref) {
            if ( this != &rref ) data_ = move(rref.data_);
            return *this;
        }
        explicit operator T () const { return data_; }
        const string& str() const { return data_; }
        const char* c_str() const { return data_.c_str(); }
        size_t size() const { return data_.size(); }
    };

    typedef copy2move<string>   c2mString;

    template< typename T >
    class copy2swap {
        T           data_;
    public:
        copy2swap<T>(T&& rdata) {
            data_.swap(rdata);
        }
        copy2swap<T>(const copy2swap<T>& ref) {
            data_.swap(const_cast< copy2swap<T>& >(ref).data_);
        }
        copy2swap<T>(copy2swap<T>&& rref) {
            data_.swap(rref.data_);
        }
        copy2swap<T>& operator = (const copy2swap<T>& ref) {
            if ( this != &ref ) {
                data_.swap(const_cast< copy2swap<T>& >(ref).data_);
            }
            return *this;
        }
        copy2swap<T>& operator = (copy2swap<T>&& rref) {
            if ( this != &rref ) {
                data_ = swap(rref.data_);
            }
            return *this;
        }
        explicit operator T () const { return data_; }
        const string& str() const { return data_; }
        const char* c_str() const { return data_.c_str(); }
        size_t size() const { return data_.size(); }
    };

    typedef copy2swap<string>   c2sString;
}

#endif

// MeetU Information and Technology Inc.
// Push Chen
// @littlepush
