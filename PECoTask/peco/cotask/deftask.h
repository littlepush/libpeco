/*
	deftask.h
	PECoTask
	2019-05-07
    Update: 2020-03-11
	Push Chen

	Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_COTASK_DEFTASK_H__
#define PE_COTASK_DEFTASK_H__

namespace pe {
    namespace co {

        // Stack Type
        typedef void *          stack_ptr;

		// Repeat Count
        typedef size_t          repeat_t;
        enum {
            oneshot_task    = 1,
            infinitive_task = (size_t)-1
        };

		// The task object
        struct task {
            // The id of the task, usually is the address of the task
            // If we bind a file descriptor on this task, the id is the fd.
            task_id             id;
            // Is event id
            bool                is_event_id;
            // The stack for the job
            stack_ptr           stack;
        #ifdef PECO_USE_UCONTEXT
            // Context for the job
            ucontext_t          ctx;
        #endif
            // The real running job's point
            task_job_t*         pjob;

            // User defined arguments
            void *              arg;
            // User defined mask
            uint32_t            user_flag_mask0;
            uint32_t            user_flag_mask1;

            // Status of the task
            task_status         status;
            // Status of the last monitoring event
            waiting_signals     signal;

            // Flags for holding
            int                 hold_sig[2];
            long                related_fd;

            // Next firing time of the task
            // for a oneshot job, is the creatation time
            task_time_t         wait_until;
            task_time_t         read_until;
            task_time_t         write_until;
            duration_t          duration;
            // Repeat count of the task
            repeat_t            repeat_count;

            // Parent task
            task*               p_task;
            task*               c_task;
            task*               next_task;
            task*               prev_task;

            task*               next_timed_task;
            task*               next_read_task;
            task*               next_write_task;
            task*               next_timedout_task;

            // Tick
            task_time_t         lt_time;

            // On Task Destory
            task_job_t*         exitjob;        
        };
    }
}

#endif

// Push Chen
