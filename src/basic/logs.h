/*
    logs.h
    PECo
    2021-05-28
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

#ifndef PECO_LOGS_H__
#define PECO_LOGS_H__

#include "pecostd.h"
#include <functional>
#include <sstream>
#include <string>

namespace peco {

// Internal Log Line Object
class log_obj {
public:
  // Dump a log line string to server
  typedef std::function<void(std::string &&)> log_dumper;

protected:
  std::string log_lv_;
  std::stringstream ss_;
  log_dumper dumper_;

protected:
  // Check if meet the new line flag and dump to the redis server
  void check_and_dump_line_();

  // Check if is new log line and dump the time
  void check_and_dump_time_();

public:
  // Init a log object with leven and dumper
  log_obj(const std::string &llv, log_dumper dumper);

  template <typename T> log_obj& operator<<(const T &value) {
    check_and_dump_time_();
    ss_ << value;
    return *this;
  }

  typedef std::basic_ostream<std::stringstream::char_type,
                             std::stringstream::traits_type>
    cout_type;
  typedef cout_type &(*stander_endline)(cout_type &);

  inline log_obj& operator<<(stander_endline fend) {
    check_and_dump_line_();
    return *this;
  }
};

// Main Log Manager
class log {
public:
  typedef std::function<void(std::string &&)> log_writer_t;

protected:
  // Internal redis group
  log_writer_t writer_;

  // Singleton
  static log &singleton();

  // Default C'str
  log();

  // Dump a line to the output
  void dump_log_line(std::string &&line);

public:
  // Set customized log writer
  static void bind_writer(log_writer_t writer);

public:
  static log_obj debug;
  static log_obj info;
  static log_obj notice;
  static log_obj warning;
  static log_obj error;
  static log_obj critical;
  static log_obj alert;
  static log_obj emergancy;
};

} // namespace peco

#endif

// Push Chen
//
