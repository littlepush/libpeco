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

#include <peco/cotask/cotask.hpp>
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

// The max cocurrent socket events. change this in the make file with -DCO_MAX_SO_EVENTS=xxx
#ifndef CO_MAX_SO_EVENTS
#define CO_MAX_SO_EVENTS		25600
#endif

#include <peco/cotask/mempage.h>

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

    #ifndef COTASK_ON_SAMETIME
    #define COTASK_ON_SAMETIME  1
    #endif

		/*
		 * The main loop of the coroutine schedule
		 * */
        class loop {
        #if COTASK_ON_SAMETIME
            typedef std::list< task * >     task_list_t;
        #endif
        protected: 
			// The main file descriptor to be used on wait core_events
			int 					core_fd_;
			core_event_t 			*core_events_;

            // Task counter
            uint64_t                task_count_;

            // Return Code
            bool                    running_;
            int                     ret_code_;

			// All time-based tasks
            #if COTASK_ON_SAMETIME
			std::map< task_time_t, task_list_t >	task_cache_;
            #else
            std::map< task_time_t, task * >         task_cache_;
            #endif

            // All event-based tasks
            std::map< intptr_t, task * >    read_event_cache_;
            std::map< intptr_t, task * >    write_event_cache_;
            task_time_t                     nearest_timeout_;

            // Internal task switcher
            void __switch_task( task* ptask );

            // Timed task schedule
            void __timed_task_schedule();
            // Event Task schedule
            void __event_task_schedule();
            // Event Task Timedout check
            void __event_task_timedout_check();
        public: 

            // Default C'str and D'str
            loop();
            ~loop();

            // No copy and move
            loop( const loop& ) = delete;
            loop( loop&& ) = delete;
            loop& operator = ( const loop& ) = delete;
            loop& operator = ( loop&& ) = delete;

			// Get current running task
            task* get_running_task();

            // Get task count
            uint64_t task_count() const;

            // Get the return code
            int return_code() const;

            // Just add a will formated task
            void insert_task( task * ptask );

            // Remove the task from timed-cache
            void remove_task( task * ptask );

			// Create a oneshot task
            task * do_job( task_job_t job );

            // Do Job with a given file descriptor
            task * do_job( long fd, task_job_t job );

			// Create a loop task
            task * do_loop( task_job_t job, duration_t interval );

			// Create a delay task
            task * do_delay( task_job_t job, duration_t delay );

            // Event Monitor
            bool monitor_task(
                int fd,
                task * ptask, 
                event_type eventid, 
                duration_t timedout = std::chrono::seconds(3)
            );

            // Cancel all monitored item of a task
            void cancel_monitor( task * ptask );

            // Cancel all monitored task of specified fd
            void cancel_fd( long fd );

            // The entry point of the main loop
            void run();

            // Exit Current Loop
            void exit( int code = 0 );

            // Tell if current loop is running
            bool is_running() const;
        };

        // Get current thread's run loop
        extern thread_local loop this_loop;
        
        // Dump the debug info of a task
        void task_debug_info( task* ptask, FILE *fp = stdout, int lv = 0);

        // Force to exit a task
        void task_exit( task * ptask );

        // Go on a task
        void task_go_on( task * ptask );

        // Stop a task's holding
        void task_stop( task * ptask );

        // Other task wrapper
        struct other_task {
            task*       _other;
            other_task( task * t );

            // Tell the holding task to go on
            void go_on() const;

            // Tell the holding task to wake up and quit
            void stop() const;

            // Tell the other task to exit on next status checking
            void exit() const;
        };

        // Register task's exit job
        void task_register_exit_job(task * ptask, task_job_t job);

        // Remove task exit job
        void task_remove_exit_job(task * ptask);

        // Other task keeper
        class other_task_keeper {
            task *                  ot_;
            bool                    ok_flag_;
        public:
            other_task_keeper( const other_task& t );
            other_task_keeper( task * t );
            ~other_task_keeper();

            // Tell the other task go on, 
            // if not invoke this method, will stop the 
            // other task when d'str this keeper
            void job_done();
        };

        // task guard
        class task_guard {
            task *      task_;
        public: 
            task_guard( task* t );
            ~task_guard();
        };

        class task_keeper {
            task *      task_;
        public: 
            task_keeper( task* t );
            ~task_keeper();
        };

        class task_pauser {
            task *      task_;
        public: 
            task_pauser( task* t );
            ~task_pauser();
        };

        namespace this_task {

            // Get the task point
            task * get_task();

            // Wrapper current task
            other_task wrapper();

            // Get the task id
            task_id get_id();

            // Get task status
            task_status status();

            // Get task last waiting signal
            waiting_signals last_signal();

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
            bool holding_until( pe::co::duration_t timedout );

            // Wait on event
            pe::co::waiting_signals wait_for_event(
                pe::co::event_type eventid, 
                pe::co::duration_t timedout
            );
            // Wait on event if match the condition
            bool wait_for_event_if(
                bool cond,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            );

            // Wait for other fd's event
            pe::co::waiting_signals wait_fd_for_event(
                long fd,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            );
            bool wait_fd_for_event_if(
                bool cond,
                long fd,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            );
            // Wait For other task's event
            pe::co::waiting_signals wait_other_task_for_event(
                task* ptask,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            );
            bool wait_other_task_for_event_if(
                bool cond,
                task* ptask,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            );

            // Sleep
            void sleep( pe::co::duration_t duration );

            // Tick time
            void begin_tick();
            double tick();

            // Dump debug info to given FILE
            void debug_info( FILE * fp = stdout );
        }

        // The parent task events
        namespace parent_task {
            // Get the parent task
            task * get_task();

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
