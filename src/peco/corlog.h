/*
    corlog.h
    PECoRLog
    2019-12-09
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_RLOG_CORLOG_H__
#define PE_CO_RLOG_CORLOG_H__

extern "C" {
    int PECoRLog_Autoconf();
}

#include <peco/cotask.h>
#include <peco/conet.h>

using namespace pe;
using namespace pe::co;

namespace pe { namespace co {

    // Internal Log Line Object
    class rlog_obj {
    public: 
        // Dump a log line string to server
        typedef std::function< void ( std::string&& ) >     log_dumper;
    protected:
        std::string         log_lv_;
        std::stringstream   ss_;
        log_dumper          dumper_;

    protected: 
        // Check if meet the new line flag and dump to the redis server
        void check_and_dump_line_();

        // Check if is new log line and dump the time
        void check_and_dump_time_();

    public: 

        // Init a log object with leven and dumper
        rlog_obj( const std::string& llv, log_dumper dumper );

        template < typename T >
        rlog_obj& operator << ( const T& value ) {
            check_and_dump_time_();
            ss_ << value;
            return *this;
        }

        typedef std::basic_ostream<
            std::stringstream::char_type, 
            std::stringstream::traits_type
        >                                               cout_type;
        typedef cout_type& (*stander_endline)(cout_type&);

        inline rlog_obj& operator << ( stander_endline fend) {
            // fend(ss_);
            check_and_dump_line_();
            return *this;
        }
    };

    // Main Log Manager
    class rlog {
        // Internal redis group
        std::shared_ptr< net::redis::group >        rg_;
        size_t                                      line_limit_;
        std::string                                 proc_name_;
        std::string                                 log_list_;

        // Singleton
        static rlog& singleton();

        // Default C'str
        rlog();
    public: 

        // Create a redis group with connection count 1
        static bool connect_log_server(const std::string& redis_server);

        // Change the log line limit
        static void set_line_limit( size_t limit );

        // Dump a line to the redis server
        static void dump_log_line( std::string&& line );

        static rlog_obj         debug;
        static rlog_obj         info;
        static rlog_obj         notice;
        static rlog_obj         warning;
        static rlog_obj         error;
        static rlog_obj         critical;
        static rlog_obj         alert;
        static rlog_obj         emergancy;
    };

}}

#endif

// Push Chen
//
