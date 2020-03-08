/*
	cotask.hpp
	PECoTask
	2019-05-07
	Push Chen

	Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_COTASK_COTASK_HPP__
#define PE_COTASK_COTASK_HPP__

#include <peco/utils/mustd.hpp>

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

// For old libpeco version, forct to use ucontext
#if defined(__LIBPECO_USE_GNU__) && LIBPECO_STANDARD_VERSION < 0x00020001
#define FORCE_USE_UCONTEXT 1
// When enabled ucontext and we are using gnu lib, can use ucontext
#elif defined(__LIBPECO_USE_GNU__) && defined(ENABLE_UCONTEXT) && ENABLE_UCONTEXT == 1
#define FORCE_USE_UCONTEXT 1
#endif

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef FORCE_USE_UCONTEXT
#include <ucontext.h>
#else
#include <setjmp.h>
#endif

#include <chrono>
#include <functional>

namespace pe {
    namespace co {

        // The task id
        typedef intptr_t        task_id;

        // Stack Type
        typedef void *          stack_ptr;

		// Repeat Count
        typedef size_t          repeat_t;
        enum {
            oneshot_task    = 1,
            infinitive_task = (size_t)-1
        };

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

		// The task object
        struct task {
            // The id of the task, usually is the address of the task
            // If we bind a file descriptor on this task, the id is the fd.
            task_id             id;
            // Is event id
            bool                is_event_id;

            // The stack for the job
            stack_ptr           stack;
        #ifdef FORCE_USE_UCONTEXT
            // Context for the job
            ucontext_t          ctx;
        #else
            stack_t             task_stack;
            jmp_buf             buf;
        #endif
            // The real running job's point
            task_job_t*         pjob;

            // User defined arguments
            void *              arg;

            // Status of the task
            task_status         status;
            // Status of the last monitoring event
            waiting_signals     signal;

            // Flags for holding
            int                 hold_sig[2];
            long                related_fd;

            // Next firing time of the task
            // for a oneshot job, is the creatation time
            task_time_t         next_time;
            task_time_t         read_until;
            task_time_t         write_until;
            duration_t          duration;
            // Repeat count of the task
            repeat_t            repeat_count;

            // Parent task
            task*               p_task;
            // Child task list head
            task*               c_task;
            // Task list
            task*               prev_task;
            task*               next_task;

            // Tick
            task_time_t         lt_time;

            // On Task Destory
            task_job_t*         exitjob;

            // Reserved flags
            struct {
                uint8_t         flag0 : 1;
                uint8_t         flag1 : 1;
                uint8_t         flag2 : 1;
                uint8_t         flag3 : 1;
                uint8_t         flag4 : 1;
                uint8_t         flag5 : 1;
                uint8_t         flag6 : 1;
                uint8_t         flag7 : 1;
            } reserved_flags;
        };
    }
}

#endif

// Push Chen
