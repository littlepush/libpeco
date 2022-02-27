/*
    sysinfo.linux.cpp
    libpeco
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

#include "peco/utils/sysinfo.h"
#include "peco/utils/filemgr.h"
#include "peco/utils/stringutil.h"

#if PECO_TARGET_LINUX || PECO_TARGET_ANDROID

#include <fstream>
#include <linux/limits.h>
#include <sys/sysinfo.h>
#include <sstream>

namespace peco {

enum CPU_TIME {
  USER_MODE,
  USER_MODE_NICE,
  SYSTEM_MODE,
  IDLE_TIME,
  IOWAIT_TIME,
  IRG_TIME,
  SOFT_IRG_TIME,
  STEAL_TIME,
  GUEST_TIME,
  GUEST_TIME_NICE
};

struct __i_cputime {
  uint64_t active_time = 0;
  uint64_t idle_time = 0;
  uint64_t total_time = 0;
  __i_cputime() = default;
  template <typename Iterator> __i_cputime(Iterator begin, Iterator end) {
    Iterator it = begin;
    for (int i = 0; i <= (int)GUEST_TIME_NICE && it != end; ++i, ++it) {
      if (i == (int)IDLE_TIME || i == (IOWAIT_TIME))
        idle_time += (std::stoll(*it));
      else
        active_time += (std::stoll(*it));
    }
    total_time = idle_time + active_time;
  }
};

struct __inner_sys_info {
  size_t cc = 0;
  uint64_t t_mem = 0;
  uint64_t u_mem = 0;

  std::vector<__i_cputime> cpu_times;
  uint64_t cpu_total_time = 0;
  time_t sys_snap_time = 0;
  time_t u_snap_time = 0;
  uint64_t active_time = 0;
  std::vector<float> cpu_loads;
  float cpu_usage = 0.f;

  std::string proc_name;

  __inner_sys_info() {
    const size_t _buf_size = PATH_MAX + 1;
    char _dirname_buffer[PATH_MAX + 1];
    int _ret = int(readlink("/proc/self/exe", _dirname_buffer, _buf_size - 1));
    if (_ret != -1) {
      _dirname_buffer[_ret] = 0;
      proc_name = _dirname_buffer;
    }
    proc_name = full_file_name(proc_name);

    cc = (size_t)sysconf(_SC_NPROCESSORS_ONLN);
    struct sysinfo _i;
    sysinfo(&_i);
    t_mem = _i.mem_unit * _i.totalram;

    // Init
    for (size_t i = 0; i < cc; ++i) {
      cpu_times.emplace_back(__i_cputime());
      cpu_loads.push_back(0);
    }

    // Get system uptime
    sys_snap_time = _i.uptime;
    u_snap_time = time(NULL);
    cpu_usage = 0;
    active_time = 0;
    __stat();
  }

  std::vector<std::string> __stat() {
    std::ifstream _sfstat("/proc/self/stat");
    std::stringstream _iss;
    _iss << _sfstat.rdbuf();
    _sfstat.close();
    std::string _stat_str = _iss.str();
    auto _scom = peco::split(_stat_str, " ");
    // Update Used memory
    if (_scom.size() > 23) {
      u_mem = (size_t)stol(_scom[23]) * (size_t)getpagesize();
    }
    return _scom;
  }

  void update() {
    // Update system cpu loads
    do {
      time_t _now = time(NULL);
      if (_now == sys_snap_time)
        break; // do not need to update
      cpu_total_time = 0;
      std::ifstream _fcpu("/proc/stat");
      std::string _cpuline;
      while (std::getline(_fcpu, _cpuline)) {
        if (peco::is_string_start(_cpuline, "cpu") == false)
          continue;
        if (peco::is_string_start(_cpuline, "cpu ") == true)
          continue;

        auto _cpu_time = peco::split(_cpuline, " ");
        if (_cpu_time.size() > 1) {
          uint64_t _core_index =
            (uint64_t)std::stoi(_cpu_time[0].substr(3)); // remove 'cpu'
          __i_cputime _ct((++_cpu_time.begin()), _cpu_time.end());
          uint64_t _active_time =
            (_ct.active_time - cpu_times[_core_index].active_time);
          uint64_t _idle_time =
            (_ct.idle_time - cpu_times[_core_index].idle_time);
          uint64_t _total_time = _active_time + _idle_time;
          cpu_total_time += _ct.total_time;
          cpu_loads[_core_index] =
            (((float)_active_time) / ((float)_total_time)) /
            (_now - sys_snap_time);
          cpu_times[_core_index] = _ct;
        }
      }
      _fcpu.close();
      sys_snap_time = _now;
    } while (false);

    // Update self cpu usage
    do {
      int64_t _now = cpu_total_time;
      if (_now == u_snap_time)
        break;
      auto _stat = __stat();
      if (_stat.size() > 16) {
        uint64_t _atime = std::stoll(_stat[13]) + std::stoll(_stat[14]) +
                          std::stoll(_stat[15]) + std::stoll(_stat[16]);
        uint64_t _total_time = _now - u_snap_time;
        uint64_t _active_time = _atime - active_time;
        cpu_usage = (float)((double)_active_time / (double)_total_time);
        u_snap_time = _now;
        active_time = _atime;
      }
    } while (false);
  }
};

// Global sysinfo cache and initialize
static __inner_sys_info &g_inner_sys_info() {
  static __inner_sys_info *g_info = new __inner_sys_info;
  return *g_info;
}

// Get cpu count
size_t cpu_count() { return g_inner_sys_info().cc; }

// Get CPU Load
std::vector<float> cpu_loads() {
  g_inner_sys_info().update();
  return g_inner_sys_info().cpu_loads;
}

// Get self's cpu usage
float cpu_usage() {
  g_inner_sys_info().update();
  return g_inner_sys_info().cpu_usage;
}

// Total Memory
uint64_t total_memory() { return g_inner_sys_info().t_mem; }

// Free Memory
uint64_t free_memory() {
  std::ifstream _fmem("/proc/meminfo");
  uint64_t _free_mem = 0;
  uint64_t _cached_mem = 0;
  std::string _line;
  while (std::getline(_fmem, _line)) {
    auto _kv = peco::split(_line, ": \t");
    if (_kv[0] == "MemFree") {
      _free_mem = std::stoll(_kv[1]);
    } else if (_kv[0] == "Cached") {
      _cached_mem = std::stoll(_kv[1]);
    }
    if (_free_mem != 0 && _cached_mem != 0)
      break;
  }
  _fmem.close();
  return _free_mem + _cached_mem;
}

// Memory Usage
uint64_t memory_usage() {
  g_inner_sys_info().update();
  return g_inner_sys_info().u_mem;
}

// Get current process's name
const std::string &process_name() { return g_inner_sys_info().proc_name; }

} // namespace peco

#endif

// Push Chen
//
