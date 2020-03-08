/*
    taskadapter.cpp
    PECoTask
    2019-10-13
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/taskadapter.h>

namespace pe {
    namespace co {

        void __stepqueue( taskadapter * ta ) {
            task* _tt = this_task::get_task();
            std::shared_ptr< semaphore > _psem(new semaphore);
            ta->sem = _psem.get();

            ON_DEBUG_COTASK(
                std::cout << "in __stepqueue for task @" << _tt->id << std::endl;
            )

            // 
            do {
                this_task::yield();
                if ( _tt->status == task_status_stopped ) break;

                // Go Step if has any
                // Continue to do, ignore the flag
                while ( ta->job_q.size() > 0 ) {
                    ON_DEBUG_COTASK(
                        std::cout << "do step in task @" << _tt->id << std::endl;
                    )
                    task_job_t _j = ta->job_q.front();
                    ta->job_q.pop();
                    _j();
                    this_task::yield();
                    if ( _tt->status == task_status_stopped ) break;
                }

                // Break if the task has already been stopped
                if ( _tt->status == task_status_stopped ) break;
            } while ( _psem->fetch() );

            ON_DEBUG_COTASK(
                std::cout << "stop step queue for taskadapter bind with @" << _tt->id << std::endl;
            )
        }

        taskadapter::taskadapter( task_job_t job ) 
            : t(NULL), sem(NULL)
        {
            // Create the pipe
            if ( job != nullptr ) job_q.push(job);
            t = this_loop.do_job([this]() {
                __stepqueue(this);
            });
            ON_DEBUG_COTASK(
                std::cout << "create taskadapter with task id: " << t->id << std::endl;
            )
        }

        taskadapter::taskadapter( long fd, task_job_t job ) 
            : t(NULL), sem(NULL)
        {
            // Create the pipe
            if ( job != nullptr ) job_q.push(job);
            t = this_loop.do_job(fd, [this]() {
                __stepqueue(this);
            });
            ON_DEBUG_COTASK(
                std::cout << "create taskadapter with fd: " << fd << std::endl;
            )
        }

        taskadapter::~taskadapter( ) {
            ON_DEBUG_COTASK(
                std::cout << "destory taskadapter with task id: " << t->id << std::endl;
            )
            sem->cancel();
            t->status = task_status_stopped;
        }

        void taskadapter::step( task_job_t job ) {
            if ( job == nullptr ) return;
            this->job_q.push( job );
            this_task::yield_if( sem == NULL );
            sem->give();
        }
    }
}

// Push Chen