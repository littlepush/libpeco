/*
 * Push Chen
 * @brief Parent Task API
*/
#pragma once

#ifndef PE_CO_TASK_PARENT_TASK_H__
#define PE_CO_TASK_PARENT_TASK_H__

#include <peco/cotask/loop.h>

namespace pe {
namespace co {
namespace parent_task {

// Get parent task
task_t get_task();

// Go on
void go_on();

// Tell the holding task to stop
void stop();

// A lock will move the ownership when copy & move
class move_lock {
public:
    ~move_lock();
    move_lock();
    move_lock( const move_lock& ml );
    move_lock( move_lock&& ml );
    move_lock& operator = ( const move_lock& ml );
    move_lock& operator = ( move_lock&& ml );
protected:
    mutable bool owner_ = true;
};

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

} // namespace parent_task
} // namespace co
} // namespace pe
#endif

// Push Chen
