/*
    process.h
    PECoTask
    2019-10-31
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_TASK_PROCESS_H__
#define PE_CO_TASK_PROCESS_H__

#include <peco/cotask/loop.h>

namespace pe {
    namespace co {
        class process {
            int     c2p_out_[2];
            int     c2p_err_[2];
            int     c2p_in_[2];
            std::vector< std::string >  cmd_;
        public: 
            // On Process output's callback
            typedef std::function< void (std::string&&) >   output_t;

        public: 
            process( );
            process( const std::string& cmd );
            ~process( );

            // No Copy and move
            process( const process& r ) = delete;
            process( process&& r ) = delete;
            process& operator = ( const process& r ) = delete;
            process& operator = ( process&& r ) = delete;

            // Append args
            void append( const std::string& arg );
            process& operator << ( const std::string& arg );

            // Bind the output callback
            output_t stdout_cb;
            output_t stderr_cb;

            // Write input to the process
            void input( std::string&& data );

            // Send EOF
            void send_eof();

            // Tell if current process is still running
            bool running() const;

            // Run and return the result
            int run();
        };

        // Export Pipe Functions to child process
#define PIPE_READ_FD   0
#define PIPE_WRITE_FD  1

        void parent_pipe_for_read( int p[2] );
        void parent_pipe_for_write( int p[2] );
        void child_dup_for_read( int p[2], int fno );
        void child_dup_for_write( int p[2], int fno );

        bool pipe_is_pending( long p );
        std::string pipe_read( long p );
    }
}

#endif

// Push Chen
