/*
    sysinfo.apple.cpp
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

#include "basic/sysinfo.h"
#include "basic/filemgr.h"

#if PECO_TARGET_APPLE

#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <sys/sysctl.h>
#include <vector>

namespace peco {

struct __i_cputime {
  uint64_t active_time = 0;
  uint64_t idle_time = 0;
  uint64_t total_time = 0;
  __i_cputime() = default;
  __i_cputime(processor_info_array_t cpu_info, int cpu_index) {
    active_time = (cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_USER] +
                   cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_SYSTEM] +
                   cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_NICE]);
    idle_time = cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_IDLE];
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
    uint32_t _ssize = _buf_size;
    if (_NSGetExecutablePath(_dirname_buffer, &_ssize) == 0) {
      proc_name = _dirname_buffer;
    }
    proc_name = full_file_name(proc_name);

    int _mib[2U] = {CTL_HW, HW_NCPU};
    size_t _size_ncpu = sizeof(cc);
    sysctl(_mib, 2U, &cc, &_size_ncpu, NULL, 0U);

    _mib[1] = HW_MEMSIZE;
    size_t _size_lmen = sizeof(t_mem);
    sysctl(_mib, 2U, &t_mem, &_size_lmen, NULL, 0U);

    // Init
    for (size_t i = 0; i < cc; ++i) {
      cpu_times.emplace_back(__i_cputime());
      cpu_loads.push_back(0);
    }

    // Get system uptime
    sys_snap_time = time(NULL);
    processor_info_array_t _cpu_info = NULL;
    mach_msg_type_number_t _ncpu_info;
    natural_t _ncpu_s = 0U;
    host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &_ncpu_s,
                        &_cpu_info, &_ncpu_info);
    for (size_t i = 0; i < cc; ++i) {
      cpu_times[i] = __i_cputime(_cpu_info, i);
      cpu_loads[i] =
        (float)cpu_times[i].active_time / (float)cpu_times[i].total_time;
    }
    size_t _cpu_info_size = sizeof(integer_t) * _ncpu_info;
    vm_deallocate(mach_task_self(), (vm_address_t)_cpu_info, _cpu_info_size);
    u_snap_time = time(NULL);
    cpu_usage = 0;
    active_time = __running_time();
  }

  uint64_t __running_time() {
    task_thread_times_info _tinfo;
    mach_msg_type_number_t _tsize = sizeof(_tinfo);
    task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t)&_tinfo,
              &_tsize);
    uint64_t _running_time =
      (_tinfo.user_time.seconds * 1000000 + _tinfo.user_time.microseconds +
       _tinfo.system_time.seconds * 1000000 + _tinfo.system_time.microseconds);

    task_basic_info_64 _binfo;
    mach_msg_type_number_t _bsize = sizeof(_binfo);
    task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&_binfo,
              &_bsize);
    uint64_t _terminated_time =
      (_binfo.user_time.seconds * 1000000 + _binfo.user_time.microseconds +
       _binfo.system_time.seconds * 1000000 + _binfo.system_time.microseconds);

    // Update used memory
    u_mem = _binfo.resident_size;
    return _running_time + _terminated_time;
  }
  void update() {
    // Update system cpu loads
    do {
      time_t _now = time(NULL);
      if (_now == sys_snap_time)
        break; // do not need to update
      cpu_total_time = 0;
      processor_info_array_t _cpu_info = NULL;
      mach_msg_type_number_t _ncpu_info;
      natural_t _ncpu_s = 0U;
      host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &_ncpu_s,
                          &_cpu_info, &_ncpu_info);
      for (size_t i = 0; i < cc; ++i) {
        __i_cputime _ct(_cpu_info, i);
        uint64_t _active_time = (_ct.active_time - cpu_times[i].active_time);
        uint64_t _total_time = (_ct.total_time - cpu_times[i].total_time);
        cpu_total_time += _ct.total_time;
        cpu_loads[i] =
          ((float)_active_time / (float)_total_time) / (_now - sys_snap_time);
        cpu_times[i] = _ct;
      }
      size_t _cpu_info_size = sizeof(integer_t) * _ncpu_info;
      vm_deallocate(mach_task_self(), (vm_address_t)_cpu_info, _cpu_info_size);
      sys_snap_time = _now;
    } while (false);

    // Update self cpu usage
    do {
      time_t _now = time(NULL);
      if (_now == u_snap_time)
        break;
      uint64_t _atime = __running_time();
      uint64_t _total_time = (_now - u_snap_time) * 1000000;
      uint64_t _active_time = _atime - active_time;
      cpu_usage = (float)((double)_active_time / (double)_total_time);
      u_snap_time = _now;
      active_time = _atime;
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
  mach_port_t host_port = mach_host_self();
  mach_msg_type_number_t host_size =
    sizeof(vm_statistics_data_t) / sizeof(integer_t);
  vm_size_t pagesize;
  vm_statistics_data_t vm_stat;

  host_page_size(host_port, &pagesize);
  host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
  return vm_stat.free_count * pagesize;
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
