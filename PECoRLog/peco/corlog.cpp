/*
    corlog.cpp
    PECoRLog
    2019-12-09
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/corlog.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <linux/limits.h>
#endif

extern "C" {
    int PECoRLog_Autoconf() { return 0; }
}

#include <sstream>
#include <iomanip>
#include <chrono>

namespace pe { namespace co {

    // Init a log object with leven and dumper
    rlog_obj::rlog_obj( const std::string& llv, log_dumper dumper )
        : log_lv_(llv), dumper_(dumper)
    { }

    // Check if meet the new line flag and dump to the redis server
    void rlog_obj::check_and_dump_line_() {
        if ( dumper_ ) dumper_(ss_.str());
        // Reset the string buffer
        ss_.str("");
        ss_.clear();
    }
    // Check if is new log line and dump the time
    void rlog_obj::check_and_dump_time_() {
        if ( ss_.tellp() != std::streampos(0) ) return;

        time_t _seconds = time(NULL);
        auto _now = task_time_now();
        auto _milliseconds = (
            _now.time_since_epoch() - 
            std::chrono::duration_cast< std::chrono::seconds >(_now.time_since_epoch())
        ) / std::chrono::milliseconds(1);
        struct tm _tm = *localtime(&_seconds);
        ss_ << '[' << 
            std::setfill('0') << std::setw(4) << _tm.tm_year + 1900 << "-" <<
            std::setfill('0') << std::setw(2) << _tm.tm_mon + 1 << "-" <<
            std::setfill('0') << std::setw(2) << _tm.tm_mday << " " <<
            std::setfill('0') << std::setw(2) << _tm.tm_hour << ":" <<
            std::setfill('0') << std::setw(2) << _tm.tm_min << ":" <<
            std::setfill('0') << std::setw(2) << _tm.tm_sec << "," <<
            std::setfill('0') << std::setw(3) << (int)_milliseconds << "][" <<
            log_lv_ << "]";
    }

    // Singleton
    rlog& rlog::singleton() {
        static rlog _r; return _r;
    }

    rlog::rlog() : rg_(nullptr), line_limit_(2000) { 
        const size_t _buf_size = PATH_MAX + 1;
        char _dirname_buffer[PATH_MAX + 1];
        #ifdef __APPLE__
        uint32_t _ssize = _buf_size;
        if ( _NSGetExecutablePath(_dirname_buffer, &_ssize) == 0 ) {
            proc_name_ = _dirname_buffer;
        }
        #else
        int _ret = int(readlink("/proc/self/exe", _dirname_buffer, _buf_size - 1));
        if ( _ret != -1 ) {
            _dirname_buffer[_ret] = 0;
            proc_name_ = _dirname_buffer;
        }
        #endif
        proc_name_ = utils::full_filename(proc_name_);
        log_list_ = proc_name_ + "-" + \
            std::to_string((long)getpid()) + "-" + \
            std::to_string((long)time(NULL));
    }

    // Create a redis group with connection count 1
    bool rlog::connect_log_server(const std::string& redis_server) {

        if ( singleton().rg_ != nullptr ) return false;

        shared_ptr< net::redis::group > _rg = std::make_shared< net::redis::group >( redis_server, 1 );
        if ( !_rg->lowest_load_connector().connection_test() ) {
            return false;
        } 
        _rg->query("RPUSH", singleton().proc_name_ + "-logs", singleton().log_list_);

        singleton().rg_ = _rg;
        return true;
    }
    // Change the log line limit
    void rlog::set_line_limit( size_t limit ) {
        singleton().line_limit_ = limit;
    }
    // Dump a line to the redis server
    void rlog::dump_log_line( std::string&& line ) {
        if ( singleton().rg_ == nullptr ) {
            std::cerr << line + "\n";
            return;
        }
        singleton().rg_->query("LPUSH", singleton().log_list_, line);
        singleton().rg_->query("LTRIM", singleton().log_list_, 0, singleton().line_limit_);
    }

    rlog_obj rlog::debug("debug", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::info("info", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::notice("notice", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::warning("warning", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::error("error", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::critical("critical", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::alert("alert", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });
    rlog_obj rlog::emergancy("emergancy", [](std::string&& line) {
        rlog::dump_log_line(std::move(line));
    });

}}

// Push Chen
//
