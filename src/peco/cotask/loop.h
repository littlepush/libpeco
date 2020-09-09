/*
    loop.h
    PECoTask
    2019-05-07
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_COTASK_LOOP_H__
#define PE_COTASK_LOOP_H__

// Linux Thread, pit_t
#ifndef __APPLE__
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#define gettid()    syscall(__NR_gettid)
#else
#include <libkern/OSAtomic.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid()    syscall(SYS_gettid)
#endif

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#define SO_NETWORK_NOSIGNAL           MSG_NOSIGNAL
#define SO_NETWORK_IOCTL_CALL         ioctl
#define SO_NETWORK_CLOSESOCK          ::close

#if __APPLE__
    #undef  SO_NETWORK_NOSIGNAL
    #define SO_NETWORK_NOSIGNAL           0
#endif
// In No-Windows
#ifndef FAR
#define FAR
#endif

#ifndef __APPLE__
#include <sys/epoll.h>
#else
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#ifndef SOCK_NONBLOCK
    #define SOCK_NONBLOCK   O_NONBLOCK
#endif
#endif
#include <sys/un.h>
#include <fcntl.h>

#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <peco/utils/mustd.hpp>

// The max cocurrent socket events. change this in the make file with -DCO_MAX_SO_EVENTS=xxx
#ifndef CO_MAX_SO_EVENTS
#define CO_MAX_SO_EVENTS		25600
#endif

#ifdef DEBUG
#define ON_DEBUG(...)       __VA_ARGS__
#else
#define ON_DEBUG(...)
#endif

#ifndef CLI_RED
#define CLI_RED "\033[1;31m"
#define CLI_GREEN "\033[1;32m"
#define CLI_YELLOW "\033[1;33m"
#define CLI_BLUE "\033[1;34m"
#define CLI_COFFEE "\033[1;35m"
#define CLI_NOR "\033[0m"

#define CLI_COLOR(c, ...)       (c) << __VA_ARGS__ << CLI_NOR
#endif

namespace pe {
    namespace co {

#ifdef DEBUG
    void enable_cotask_trace();
    bool is_cotask_trace_enabled();

    #define ON_DEBUG_COTASK(...)    if ( is_cotask_trace_enabled() ) { __VA_ARGS__ }
#else
    #define ON_DEBUG_COTASK(...)
#endif

    #if __APPLE__
        typedef struct kevent               core_event_t;
    #else
        typedef struct epoll_event          core_event_t;
    #endif    

    #ifdef ENABLE_THREAD_LOOP
    #define __thread_attr                   thread_local
    #else
    #define __thread_attr
    #endif

        // The task id
        typedef intptr_t                                task_id;
        // The Task Handler
        typedef void *                                  task_t;
		// Time
        typedef std::chrono::high_resolution_clock      hr_time_t;
        typedef std::chrono::time_point< hr_time_t >    task_time_t;
        typedef std::chrono::nanoseconds                duration_t;
        #define task_time_now   std::chrono::high_resolution_clock::now

		// Job
        typedef std::function< void(void) >             task_job_t;

		// A task's status
        typedef enum {
            // Pending task, not been started yet.
            task_status_pending         = 0,
            // Running 
            task_status_running         = 1,
            // Paused, maybe yield
            task_status_paused          = 2,
            // Stop a task, any loop should check this status
            task_status_stopped         = 3
        } task_status;

        // Task Waiting Status
        typedef enum {
            // The event now waiting did receive the notification
            receive_signal              = 0,
            // No signal after the timeout duration
            no_signal                   = 1,
            // Some error occurred
            bad_signal                  = 2
        } waiting_signals;

        // Task waiting event
        typedef enum {
            // The file descriptor is ready for reading
            event_read                  = 0x01,
            // The file descriptor is ready for writing
            event_write                 = 0x02
        } event_type;

		/*
		 * The main loop of the coroutine schedule
		 * */
        class loop {
        protected: 
            // Return Code
            bool                    running_;
            int                     ret_code_;

            // No copy and move
            loop( const loop& ) = delete;
            loop( loop&& ) = delete;
            loop& operator = ( const loop& ) = delete;
            loop& operator = ( loop&& ) = delete;
        public: 
            // Default C'str and D'str
            loop();
            ~loop();

            // Get task count
            uint64_t task_count() const;

            // Get the return code
            int return_code() const;

			// Create a oneshot task
            task_t do_job( task_job_t job );

            // Do Job with a given file descriptor
            task_t do_job( long fd, task_job_t job );

			// Create a loop task
            task_t do_loop( task_job_t job, duration_t interval );

			// Create a delay task
            task_t do_delay( task_job_t job, duration_t delay );

            // The entry point of the main loop
            void run();

            // Exit Current Loop
            void exit( int code = 0 );

            // Tell if current loop is running
            bool is_running() const;

            // The main loop
            static __thread_attr loop   main;
        };

        #define this_loop       loop::main

        // Dump the debug info of a task
        void task_debug_info( task_t ptask, FILE *fp = stdout, int lv = 0);

        // Cancel all bounded task of given fd
        void tasks_cancel_fd( long fd );

        // Force to exit a task
        void task_exit( task_t ptask );

        // Go on a task, when a task is blocked by 'holding', 
        // make the 'holding' return true
        void task_go_on( task_t ptask );

        // Stop a task's holding, make 'holding' return false
        void task_stop( task_t ptask );

        // Exit all children and self
        void task_recurse_exit( task_t ptask );

        // Get task's ID
        task_id task_get_id( task_t ptask );

        // Set/Get Task Argument
        void task_set_arg( task_t ptask, void * arg );
        void * task_get_arg( task_t ptask );

        // Set/Get Task User Flags
        void task_set_user_flag0( task_t ptask, uint32_t flag );
        uint32_t task_get_user_flag0( task_t ptask );
        void task_set_user_flag1( task_t ptask, uint32_t flag );
        uint32_t task_get_user_flag1( task_t ptask );

        // Get task's status
        task_status task_get_status( task_t ptask );

        // Get task's waiting_signal
        waiting_signals task_get_waiting_signal( task_t ptask );
        
        // Register task's exit job
        void task_register_exit_job(task_t ptask, task_job_t job);

        // Remove task exit job
        void task_remove_exit_job(task_t ptask);

        // task guard, wrapper for task_exit
        class task_guard {
            task_t      task_;
        public: 
            task_guard( task_t t );
            ~task_guard();
        };

        // wrapper for task_stop
        class task_keeper {
            task_t      task_;
        public: 
            task_keeper( task_t t );
            ~task_keeper();
        };

        // Wrapper for task_go_on
        class task_pauser {
            task_t      task_;
        public: 
            task_pauser( task_t t );
            ~task_pauser();
        };

        // If not invoke job_done, will stop the task when desroy
        class task_helper {
            task_t      task_;
        public:
            task_helper( task_t t );
            ~task_helper();
            void job_done();
        };
 
        namespace this_task {

            // Get the task point
            task_t get_task();

            // Get the task id
            task_id get_id();

            // Recurse Exit
            void recurse_exit();

            // Set/Get Task Argument
            void set_arg( void * arg );
            void * get_arg( );

            // Set/Get Task User Flags
            void set_user_flag0( uint32_t flag );
            uint32_t get_user_flag0( );
            void set_user_flag1( uint32_t flag );
            uint32_t get_user_flag1( );

            // Get task's status
            task_status get_status( );

            // Get task's waiting_signal
            waiting_signals get_waiting_signal( );

            // Cancel loop inside
            void cancel_loop();

            // yield, force to switch co
            void yield();

            // Yield if the condition is true
            bool yield_if( bool cond );

            // Yield if the condition is false
            bool yield_if_not( bool cond );

            // Hold current task, yield, but not put back
            // to the timed based cache
            bool holding();

            // Hold the task until timedout
            bool holding_until( duration_t timedout );

            // Wait on event
            waiting_signals wait_for_event(
                event_type eventid, 
                duration_t timedout
            );
            // Wait on event if match the condition
            bool wait_for_event_if(
                bool cond,
                event_type eventid,
                duration_t timedout
            );

            // Wait for other fd's event
            waiting_signals wait_fd_for_event(
                long fd,
                event_type eventid,
                duration_t timedout
            );
            bool wait_fd_for_event_if(
                bool cond,
                long fd,
                event_type eventid,
                duration_t timedout
            );
            // Wait For other task's event
            waiting_signals wait_other_task_for_event(
                task_t ptask,
                event_type eventid,
                duration_t timedout
            );
            bool wait_other_task_for_event_if(
                bool cond,
                task_t ptask,
                event_type eventid,
                duration_t timedout
            );

            // Sleep
            void sleep( duration_t duration );

            // Tick time
            void begin_tick();
            double tick();

            // Dump debug info to given FILE
            void debug_info( FILE * fp = stdout );
        }

        // The parent task events
        namespace parent_task {
            // Get the parent task
            task_t get_task();

            // Go on
            void go_on();

            // Tell the holding task to stop
            void stop();

            struct guard {
                bool            ok_flag;

                // Simple guard for parent task,
                // if the ok_flag is true, will invoke go_on when destroy
                // otherwise, invoke stop
                guard();
                ~guard();

                // set the ok_flag to true
                void job_done();
            };

            // Helper
            struct helper {
                bool            sig_sent;

                helper();
                ~helper();

                // Send a signal to the parent task
                void can_go_on();
                void need_stop();
            };

            // Tell the parent task to exit, and detach self
            void exit();
        }
   }
}

#endif

// Push Chen
