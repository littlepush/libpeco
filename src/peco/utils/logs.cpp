/*
    logs.cpp
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

#include "peco/utils/logs.h"
#include "peco/utils/filemgr.h"
#include "peco/utils/sysinfo.h"

#include <chrono>
#include <iomanip>

namespace peco {

// Init a log object with level and dumper
log_obj::log_obj(const std::string &llv, log_dumper dumper)
  : log_lv_(llv), dumper_(dumper) {}

// Check if meet the new line flag and dump to the redis server
void log_obj::check_and_dump_line_() {
  if (dumper_)
    dumper_(ss_.str());
  // Reset the string buffer
  ss_.str("");
  ss_.clear();
}
// Check if is new log line and dump the time
void log_obj::check_and_dump_time_() {
  if (ss_.tellp() != std::streampos(0))
    return;

  std::time_t _seconds = std::time(NULL);
  auto _now = std::chrono::steady_clock::now();
  auto _milliseconds =
    (_now.time_since_epoch() - std::chrono::duration_cast<std::chrono::seconds>(
                                 _now.time_since_epoch())) /
    std::chrono::milliseconds(1);
  std::tm _tm{};
#if PECO_TARGET_WIN
  localtime_s(&_tm, &_seconds);
#else
  localtime_r(&_seconds, &_tm);
#endif
  ss_ << '[' << std::setfill('0') << std::setw(4) << _tm.tm_year + 1900 << "-"
      << std::setfill('0') << std::setw(2) << _tm.tm_mon + 1 << "-"
      << std::setfill('0') << std::setw(2) << _tm.tm_mday << " "
      << std::setfill('0') << std::setw(2) << _tm.tm_hour << ":"
      << std::setfill('0') << std::setw(2) << _tm.tm_min << ":"
      << std::setfill('0') << std::setw(2) << _tm.tm_sec << ","
      << std::setfill('0') << std::setw(3) << (int)_milliseconds << "]["
      << log_lv_ << "]";
}

// Singleton
log &log::singleton() {
  static log *_r = new log;
  return *_r;
}

log::log() {
  // Default writer is write to std out
  writer_ = [](std::string &&line) { std::cout << line << std::endl; };
}

// Dump a line to the redis server
void log::dump_log_line(std::string &&line) {
  if (line.size() == 0) return;
  if (writer_) {
    writer_(std::move(line));
  }
}

// Set customized log writer
void log::bind_writer(log_writer_t writer) {
  log::singleton().writer_ = writer;
}

log_obj log::debug("debug", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::info("info", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::notice("notice", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::warning("warning", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::error("error", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::critical("critical", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::alert("alert", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});
log_obj log::emergancy("emergancy", [](std::string &&line) {
  log::singleton().dump_log_line(std::move(line));
});

} // namespace peco

// Push Chen
//
