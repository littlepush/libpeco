/*
    sysinfo.win.cpp
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

#if PECO_TARGET_WIN

#include <Pdh.h>
#include <pdhmsg.h>
#include <sysinfoapi.h>
#include <tchar.h>
#include <windows.h>
#pragma comment(lib, "pdh.lib")

namespace peco {

struct __inner_sys_info {
  size_t cc = 0;
  uint64_t t_mem = 0;
  uint64_t u_mem = 0;
  uint64_t f_mem = 0;

  HCOUNTER *processor_times = NULL;
  std::vector<float> cpu_loads;
  uint64_t cpu_total_time = 0;
  time_t sys_snap_time = 0;
  ULONGLONG proc_time = 0;
  ULONGLONG active_time = 0;
  float cpu_usage = 0.f;

  std::string proc_name;

  LPTSTR counter_list_buffer = NULL;
  DWORD counter_list_size = 0;
  LPTSTR instance_list_buffer = NULL;
  DWORD instance_list_size = 0;
  HANDLE load_event = NULL;
  HQUERY query_handler = NULL;

  ~__inner_sys_info() {
    if (query_handler) {
      PdhCloseQuery(query_handler);
      query_handler = NULL;
    }
    if (load_event) {
      CloseHandle(load_event);
      load_event = NULL;
    }
    if (instance_list_buffer) {
      free(instance_list_buffer);
      instance_list_buffer = nullptr;
    }
    if (counter_list_buffer) {
      free(counter_list_buffer);
      counter_list_buffer = nullptr;
    }
    if (processor_times) {
      delete [] processor_times;
      processor_times = NULL;
    }
  }

  __inner_sys_info() {
    char file_name[MAX_PATH];
    GetModuleFileName(NULL, file_name, MAX_PATH);
    proc_name = file_name;
    proc_name = full_file_name(proc_name);

    // Create event to monitor each cores
    load_event = CreateEvent(NULL, FALSE, FALSE, TEXT("PECoMonitor"));
    // Determine the required buffer size for the data.
    PDH_STATUS pdh_status =
      PdhEnumObjectItems(NULL,                 // real time source
                         NULL,                 // local machine
                         TEXT("Processor"),    // object to enumerate
                         counter_list_buffer,  // pass NULL and 0
                         &counter_list_size,   // to get length required
                         instance_list_buffer, // buffer size
                         &instance_list_size,  //
                         PERF_DETAIL_WIZARD,   // counter detail level
                         0);
    if (pdh_status != PDH_MORE_DATA) {
      return;
    }

    // Allocate the buffers and try the call again.
    counter_list_buffer = (LPTSTR)malloc(counter_list_size * sizeof(TCHAR));
    instance_list_buffer = (LPTSTR)malloc(instance_list_size * sizeof(TCHAR));

    if (counter_list_buffer == NULL || instance_list_buffer == NULL) {
      return;
    }
    pdh_status = PdhEnumObjectItems(
      NULL, NULL, TEXT("Processor"), counter_list_buffer, &counter_list_size,
      instance_list_buffer, &instance_list_size, PERF_DETAIL_WIZARD, 0);

    if (pdh_status != ERROR_SUCCESS) {
      return;
    }

    // Walk the instance list. The list can contain one
    // or more null-terminated strings. The last string
    // is followed by a second null-terminator.
    for (LPTSTR this_instance = instance_list_buffer; *this_instance != 0;
         this_instance += lstrlen(this_instance) + 1) {
      if (0 != _tcscmp(this_instance, TEXT("_Total"))) {
        // it's not the toalizer, so count it
        cc++;
      }
    }

    // Alloc processor time counter storage
    processor_times = new HCOUNTER[cc];
    pdh_status = PdhOpenQuery(NULL, 1, &query_handler);
    if (pdh_status != ERROR_SUCCESS) {
      _tprintf(TEXT("PdhOpenQuery Failed: 0x%8.8X\n"), pdh_status);
    }
    for (int n = 0; n < cc; ++n) {
      cpu_loads.push_back(0.f);
      TCHAR counter_path[255] = {'\0'};
      _stprintf_s(counter_path, 255, TEXT("\\Processor(%d)\\%% Processor Time"), n);
      pdh_status =
        PdhAddCounter(query_handler, counter_path, n, &processor_times[n]);
      _tprintf(TEXT("add counter: \"%s\"\n"), counter_path);
      if (pdh_status != ERROR_SUCCESS) {
        _tprintf(TEXT("Couldn't add counter \"%s\": 0x%8.8X\n"), counter_path, pdh_status);
      }
    }
    pdh_status = PdhCollectQueryDataEx(query_handler, 2, load_event);
    if (pdh_status != ERROR_SUCCESS) {
      _tprintf(TEXT("PdhCollectQueryDataEx Failed: 0x%8.8X\n"), pdh_status);
    }
    this->update();
  }

  bool __get_system_cpu_time(ULONGLONG &total_time, ULONGLONG &idle_time) {
    FILETIME ft_sys_idle, ft_sys_kernel, ft_sys_user;
    if (!::GetSystemTimes(&ft_sys_idle, &ft_sys_kernel, &ft_sys_user)) {
      return false;
    }

    ULARGE_INTEGER sys_kernel, sys_user, sys_idle;
    sys_kernel.HighPart = ft_sys_kernel.dwHighDateTime;
    sys_kernel.LowPart = ft_sys_kernel.dwLowDateTime;
    sys_user.HighPart = ft_sys_user.dwHighDateTime;
    sys_user.LowPart = ft_sys_user.dwLowDateTime;
    sys_idle.HighPart = ft_sys_idle.dwHighDateTime;
    sys_idle.LowPart = ft_sys_idle.dwLowDateTime;
    // idle time also included in the sysKernel time.
    total_time = sys_kernel.QuadPart + sys_user.QuadPart;
    idle_time = sys_idle.QuadPart;

    return true;
  }
  bool __get_process_cpu_time(HANDLE hProcess, ULONGLONG &running_time) {
    FILETIME ft_proc_create, ft_proc_exit, ft_proc_kernel, ft_proc_user;
    if (!GetProcessTimes(hProcess, &ft_proc_create, &ft_proc_exit,
                         &ft_proc_kernel, &ft_proc_user)) {
      return false;
    }
    ULARGE_INTEGER proc_kernel, proc_user;
    proc_kernel.HighPart = ft_proc_kernel.dwHighDateTime;
    proc_kernel.LowPart = ft_proc_kernel.dwLowDateTime;
    proc_user.HighPart = ft_proc_user.dwHighDateTime;
    proc_user.LowPart = ft_proc_user.dwLowDateTime;
    running_time = proc_kernel.QuadPart + proc_user.QuadPart;
    return true;
  }

  void update() {
    time_t _now = time(NULL);
    if (_now == sys_snap_time)
      return;
    sys_snap_time = _now;

    // Update memory info
    MEMORYSTATUSEX _mem_stat;
    _mem_stat.dwLength = sizeof(_mem_stat);
    ::GlobalMemoryStatusEx(&_mem_stat);
    t_mem = _mem_stat.ullTotalPhys;
    u_mem = (uint64_t)(_mem_stat.dwMemoryLoad * t_mem / 100);
    f_mem = _mem_stat.ullAvailPhys;

    // Get App CPU Usage
    ULONGLONG cpu_time = 0;
    ULONGLONG cpu_idle_time = 0;
    __get_system_cpu_time(cpu_time, cpu_idle_time);
    ULONGLONG cpu_diff = cpu_time - proc_time;
    proc_time = cpu_time; // backup for next sampling
    ULONGLONG running_time = 0;

    __get_process_cpu_time(::GetCurrentProcess(), running_time);
    ULONGLONG proc_diff = running_time - active_time;
    active_time = running_time; // backup for next sampling
    if (cpu_diff > 0) {
      cpu_usage = (float)((double)proc_diff / (double)cpu_diff);
    }

    if (query_handler != NULL && load_event != NULL) {
      DWORD counter_type = 0;
      for (size_t n = 0; n < cc; ++n) {
        PDH_FMT_COUNTERVALUE v_proc;
        auto status = PdhGetFormattedCounterValue(
            processor_times[n], PDH_FMT_DOUBLE,
            &counter_type, &v_proc);
        if (status != ERROR_SUCCESS) {
          continue;
          // _tprintf(TEXT("0: Error 0x%8.8X\n"), status);
        }
        cpu_loads[n] = (float)v_proc.doubleValue / 100.f;
      }
    }
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
  g_inner_sys_info().update();
  return g_inner_sys_info().f_mem;
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

