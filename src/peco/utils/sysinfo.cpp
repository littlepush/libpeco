/*
    sysinfo.cpp
    PEUtils
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

#include <peco/utils/sysinfo.h>

#if PZC_TARGET_LINUX
#include <sys/sysinfo.h>
#elif PZC_TARGET_MAC
#include <mach/processor_info.h>
#include <mach/mach_host.h>
#include <mach/task_info.h>
#include <mach/task.h>
#import <sys/sysctl.h>
#include <mach/mach.h>
#endif

namespace pe { namespace utils { namespace sys {

#if PZC_TARGET_LINUX
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
#endif

    struct __i_cputime {
        uint64_t            active_time;
        uint64_t            idle_time;
        uint64_t            total_time;
        __i_cputime() : active_time(0), idle_time(0), total_time(0) { }
#if PZC_TARGET_LINUX
        template < typename Iterator >
        __i_cputime(Iterator begin, Iterator end) : active_time(0), idle_time(0), total_time(0) {
            Iterator it = begin;
            for ( int i = 0; i <= (int)GUEST_TIME_NICE && it != end; ++i, ++it ) {
                if ( i == (int)IDLE_TIME || i == (IOWAIT_TIME) ) idle_time += (stoll(*it));
                else active_time += (stoll(*it));
            }
            total_time = idle_time + active_time;
        }
#elif PZC_TARGET_MAC
        __i_cputime(processor_info_array_t cpu_info, int cpu_index) {
            active_time = (
                cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_USER] + 
                cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_SYSTEM] + 
                cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_NICE]);
            idle_time = cpu_info[CPU_STATE_MAX * cpu_index + CPU_STATE_IDLE];
            total_time = idle_time + active_time;
        }
#endif
    };

    struct __inner_sys_info {
        size_t                      cc;
        uint64_t                    t_mem;
        uint64_t                    u_mem;

        std::vector<__i_cputime>    cpu_times;
        uint64_t                    cpu_total_time;
        time_t                      sys_snap_time;
        time_t                      u_snap_time;
        uint64_t                    active_time;
        std::vector<float>          cpu_loads;
        float                       cpu_usage;

        __inner_sys_info() {
#if PZC_TARGET_LINUX
            cc = (size_t)sysconf(_SC_NPROCESSORS_ONLN);
            struct sysinfo _i;
            sysinfo(&_i);
            t_mem = _i.mem_unit * _i.totalram;
#elif PZC_TARGET_MAC
            int _mib[2U] = { CTL_HW, HW_NCPU };
            size_t _size_ncpu = sizeof(cc);
            sysctl(_mib, 2U, &cc, &_size_ncpu, NULL, 0U);

            _mib[1] = HW_MEMSIZE;
            size_t _size_lmen = sizeof(t_mem);
            sysctl(_mib, 2U, &t_mem, &_size_lmen, NULL, 0U);
#endif

            // Init
            for ( size_t i = 0; i < cc; ++i ) {
                cpu_times.emplace_back(__i_cputime());
                cpu_loads.push_back(0);
            }

            // Get system uptime
#if PZC_TARGET_LINUX
            sys_snap_time = _i.uptime;
#elif PZC_TARGET_MAC
            sys_snap_time = time(NULL);
            processor_info_array_t _cpu_info = NULL;
            mach_msg_type_number_t _ncpu_info;
            natural_t _ncpu_s = 0U;
            host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &_ncpu_s, &_cpu_info, &_ncpu_info);
            for ( size_t i = 0; i < cc; ++i ) {
                cpu_times[i] = __i_cputime(_cpu_info, i);
                cpu_loads[i] = (float)cpu_times[i].active_time / (float)cpu_times[i].total_time;
            }
            size_t _cpu_info_size = sizeof(integer_t) * _ncpu_info;
            vm_deallocate(mach_task_self(), (vm_address_t)_cpu_info, _cpu_info_size);
#else
            sys_snap_time = 0;
#endif
            u_snap_time = time(NULL);
            cpu_usage = 0;
#if PZC_TARGET_LINUX
            active_time = 0;
            __stat();
#elif PZC_TARGET_MAC
            active_time = __running_time();
#else
            active_time = 0;
#endif
        }

#if PZC_TARGET_LINUX
        std::vector<string> __stat() {
            ifstream _sfstat("/proc/self/stat");
            stringstream _iss;
            _iss << _sfstat.rdbuf();
            _sfstat.close();
            string _stat_str = _iss.str();
            auto _scom = pe::utils::split(_stat_str, " ");
            // Update Used memory
            u_mem = (size_t)stol(_scom[23]) * (size_t)getpagesize();
            return _scom;
        }
#elif PZC_TARGET_MAC
        uint64_t __running_time() {
            task_thread_times_info _tinfo;
            mach_msg_type_number_t _tsize = sizeof(_tinfo);
            task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t)&_tinfo, &_tsize);
            uint64_t _running_time = (
                _tinfo.user_time.seconds * 1000000 + _tinfo.user_time.microseconds + 
                _tinfo.system_time.seconds * 1000000 + _tinfo.system_time.microseconds
            );
            
            task_basic_info_64 _binfo;
            mach_msg_type_number_t _bsize = sizeof(_binfo);
            task_info(mach_task_self(), TASK_BASIC_INFO_64, (task_info_t)&_binfo, &_bsize);
            uint64_t _terminated_time = (
                _binfo.user_time.seconds * 1000000 + _binfo.user_time.microseconds + 
                _binfo.system_time.seconds * 1000000 + _binfo.system_time.microseconds
            );

            // Update used memory
            u_mem = _binfo.resident_size;
            return _running_time + _terminated_time;
        }
#endif
        void update() {
            // Update system cpu loads
            do {
                time_t _now = time(NULL); 
                if ( _now == sys_snap_time ) break; // do not need to update
                cpu_total_time = 0;
    #if PZC_TARGET_LINUX
                ifstream _fcpu("/proc/stat");
                string _cpuline;
                while ( std::getline(_fcpu, _cpuline) ) {
                    if ( pe::utils::is_string_start(_cpuline, "cpu") == false ) continue;
                    if ( pe::utils::is_string_start(_cpuline, "cpu ") == true ) continue;

                    auto _cpu_time = pe::utils::split(_cpuline, " ");
                    uint64_t _core_index = (uint64_t)stoi(_cpu_time[0].substr(3));  // remove 'cpu'
                    __i_cputime _ct((++_cpu_time.begin()), _cpu_time.end());
                    uint64_t _active_time = (_ct.active_time - cpu_times[_core_index].active_time);
                    uint64_t _idle_time = (_ct.idle_time - cpu_times[_core_index].idle_time);
                    uint64_t _total_time = _active_time + _idle_time;
                    cpu_total_time += _ct.total_time;
                    cpu_loads[_core_index] = (((float)_active_time) / ((float)_total_time)) / (_now - sys_snap_time);
                    cpu_times[_core_index] = _ct;
                }
                _fcpu.close();
    #elif PZC_TARGET_MAC
                processor_info_array_t _cpu_info = NULL;
                mach_msg_type_number_t _ncpu_info;
                natural_t _ncpu_s = 0U;
                host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &_ncpu_s, &_cpu_info, &_ncpu_info);
                for ( size_t i = 0; i < cc; ++i ) {
                    __i_cputime _ct(_cpu_info, i);
                    uint64_t _active_time = (_ct.active_time - cpu_times[i].active_time);
                    uint64_t _total_time = (_ct.total_time - cpu_times[i].total_time);
                    cpu_total_time += _ct.total_time;
                    cpu_loads[i] = ((float)_active_time / (float)_total_time) / (_now - sys_snap_time);
                    cpu_times[i] = _ct;
                }
                size_t _cpu_info_size = sizeof(integer_t) * _ncpu_info;
                vm_deallocate(mach_task_self(), (vm_address_t)_cpu_info, _cpu_info_size);
    #endif
                sys_snap_time = _now;
            } while ( false );

            // Update self cpu usage
            do {
    #if PZC_TARGET_LINUX
                int64_t _now = cpu_total_time;
    #else
                time_t _now = time(NULL); 
    #endif
                if ( _now == u_snap_time ) break;
    #if PZC_TARGET_LINUX
                auto _stat = __stat();
                uint64_t _atime = stoll(_stat[13]) + stoll(_stat[14]) + stoll(_stat[15]) + stoll(_stat[16]);
                uint64_t _total_time = _now - u_snap_time;
    #elif PZC_TARGET_MAC
                uint64_t _atime = __running_time();
                uint64_t _total_time = (_now - u_snap_time) * 1000000;
    #endif
                uint64_t _active_time = _atime - active_time;
    #if PZC_TARGET_LINUX
                cpu_usage = (float)((double)_active_time / (double)_total_time);
    #else
                cpu_usage = (float)((double)_active_time / (double)_total_time);
    #endif
                u_snap_time = _now;
                active_time = _atime;
            } while ( false );
        }
    };

    // Global sysinfo cache and initialize
    static __inner_sys_info g_sysinfo;

    // Invoke before get and sysinfo
    void update_sysinfo() { g_sysinfo.update(); }

    // Get cpu count
    size_t cpu_count() { return g_sysinfo.cc; }

    // Get CPU Load
    std::vector< float > cpu_loads() { return g_sysinfo.cpu_loads; }

    // Get self's cpu usage
    float cpu_usage() { return g_sysinfo.cpu_usage; }

    // Total Memory
    uint64_t total_memory() { return g_sysinfo.t_mem; }

    // Free Memory
    uint64_t free_memory() {
#if PZC_TARGET_LINUX
        ifstream _fmem("/proc/meminfo");
        uint64_t _free_mem = 0;
        uint64_t _cached_mem = 0;
        string _line;
        while ( std::getline(_fmem, _line) ) {
            auto _kv = pe::utils::split(_line, ": \t");
            if ( _kv[0] == "MemFree" ) {
                _free_mem = stoll(_kv[1]);
            } else if ( _kv[0] == "Cached" ) {
                _cached_mem = stoll(_kv[1]);
            }
            if ( _free_mem != 0 && _cached_mem != 0 ) break;
        }
        _fmem.close();
        return _free_mem + _cached_mem;
#elif PZC_TARGET_MAC
        mach_port_t host_port = mach_host_self();
        mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
        vm_size_t pagesize;
        vm_statistics_data_t vm_stat;
        
        host_page_size(host_port, &pagesize);
        host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
        return vm_stat.free_count * pagesize;
#else
        return 0;
#endif
    }

    // Memory Usage
    uint64_t memory_usage() { return g_sysinfo.u_mem; }
    
}}}

// Push Chen
//
