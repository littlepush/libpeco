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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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

            // User defined arguments
            void *              arg;

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
