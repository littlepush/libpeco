/*
 * Push Chen
 * @brief Task Queue
*/

#include <peco/cotask/loop.h>
#include <peco/cotask/taskqueue.h>

namespace pe {
namespace co {

// Create a new task queue
// use the given fd to create the task
taskqueue::taskqueue( long fd = -1 ) : running_(false), fd_(fd), task_(nullptr) { }

// Stop the queue
taskqueue::~taskqueue() {
    this->stop();
}

// Start the queue
void taskqueue::start() {
    if ( running_ ) return;
    running_ = true;
    if ( fd_ == -1 ) {
        task_ = loop::main.do_job([this] (){
            this->runloop();
            this->running_ = false;
        });
    } else {
        task_ = loop::main.do_job(fd_, [this]() {
            this->runloop();
            this->running_ = false;
        });
    }
}

// stop the queue
void taskqueue::stop() {
    if ( running_ == false ) return;
    s_.cancel();
    task_stop(task_);
    task_ = nullptr;
}

// Put a job in the tail of the queue
void taskqueue::step( task_job_t job ) {
    if ( running_ == false ) return;
    jq_.emplace(job);
    s_.give();
}

// wait for the job return
void taskqueue::promise( std::function<void()> job ) {
    if ( ! running_ ) return;
    do {
        parent_task::move_lock ml;
        this->step([ml, job]() {
            job();
        });
    } while ( false );
    std::ignore_result(this_task::holding());
}

void taskqueue::runloop() {
    while ( 
        (this_task::get_status() == task_status_running) && 
        (s_.fetch()) 
    ) {
        task_job_t job = jq_.front();
        jq_.pop();
        // Do job
        job();
    }
}

} // namespace co
} // namespace pe

// Push Chen
