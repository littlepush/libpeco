/*
    taskadapter.h
    PECoTask
    2019-10-13
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_TASK_TASKADAPTER_H__
#define PE_CO_TASK_TASKADAPTER_H__

#include <peco/cotask/cotask.hpp>
#include <peco/cotask/loop.h>
#include <peco/cotask/condition.h>

namespace pe {
    namespace co {

        struct taskadapter {
            task *                      t;
            std::queue< task_job_t >    job_q;
            semaphore *                 sem;

            // Create a task with the adapter
            taskadapter( task_job_t job = nullptr );
            taskadapter( long fd, task_job_t job = nullptr );

            // The task will stop when the adapter been destoried.
            virtual ~taskadapter( );

            // Go on next step
            void step( task_job_t job );
        };
    }
}

#endif

// Push Chen
