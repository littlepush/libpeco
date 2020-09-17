/*
 * Push Chen
 * @brief Task Queue
*/
#pragma once

#ifndef PE_CO_TASK_TASK_QUEUE_H__
#define PE_CO_TASK_TASK_QUEUE_H__

#include <peco/cotask/loop.h>
#include <peco/cotask/parent_task.h>
#include <peco/cotask/condition.h>
#include <peco/peutils.h>
#include <queue>

namespace pe {
namespace co {

class taskqueue : public std::enable_shared_from_this<taskqueue> {
public:
    // Create a new task queue
    // use the given fd to create the task
    taskqueue( long fd = -1 );

    // Stop the queue
    ~taskqueue();

    // disable copy & move
    taskqueue( const taskqueue& ) = delete;
    taskqueue( taskqueue&& ) = delete;
    taskqueue& operator =( const taskqueue& ) = delete;
    taskqueue& operator =( taskqueue&& ) = delete;

    // Start the queue
    void start();

    // stop the queue
    void stop();

    // Put a job in the tail of the queue
    void step( task_job_t job );

    // wait for the job return
    template < typename RetValue >
    RetValue promise( std::function< RetValue() > job ) {
        RetValue ret;
        if ( ! running_ ) return ret;
        do {
            parent_task::move_lock ml;
            this->step([ml, job, &ret]() {
                ret = job();
            });
        } while ( false );
        // Hold 
        std::ignore_result(this_task::holding());
        return ret;
    }

    // Override for void promise
    void promise( std::function< void() > job );

protected:
    // Inner runloop
    void runloop();
protected:
    bool running_ = true;
    long fd_ = -1;
    task_t task_ = nullptr;
    semaphore s_;
    std::queue< task_job_t > jq_;
};

} // namespace co
} // namespace pe

#endif

// Push Chen
