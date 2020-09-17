/*
 * Push Chen
 * @brief Parent Task API
*/
#include <peco/cotask/parent_task.h>
#include <peco/cotask/deftask.h>

namespace pe {
namespace co {
namespace parent_task {

// Get the parent task
task_t get_task() {
    task * _ttask = (task *)this_task::get_task();
    if ( _ttask == NULL ) return NULL;
    return (task_t)_ttask->p_task;
}

// Go on
void go_on() { task_go_on(get_task()); }

// Tell the holding task to stop
void stop() { task_stop(get_task()); }

move_lock::~move_lock() {
    if ( owner_ ) {
        // Send a signal if im the owner of this lock
        parent_task::go_on();
    }
}
move_lock::move_lock() { }
move_lock::move_lock( const move_lock& ml ) {
    owner_ = true;
    ml.owner_ = false;
}
move_lock::move_lock( move_lock&& ml ) {
    owner_ = true;
    ml.owner_ = false;
}
move_lock& move_lock::operator = ( const move_lock& ml ) {
    if ( this == &ml ) return *this;
    owner_ = true;
    ml.owner_ = false;
    return *this;
}
move_lock& move_lock::operator = ( move_lock&& ml ) {
    if ( this == &ml ) return *this;
    owner_ = true;
    ml.owner_ = false;
    return *this;
}

// Simple guard for parent task,
// if the ok_flag is true, will invoke go_on when destroy
// otherwise, invoke stop
guard::guard() : ok_flag(false) {}
guard::~guard() {
    if ( ok_flag ) { parent_task::go_on(); }
    else { parent_task::stop(); }
}

// set the ok_flag to true
void guard::job_done() { ok_flag = true; }

// Helper
helper::helper() : sig_sent(false) { }
helper::~helper() {
    if ( sig_sent ) return;
    parent_task::stop();
}

// Send a signal to the parent task
void helper::can_go_on() {
    sig_sent = true;
    parent_task::go_on();
}
void helper::need_stop() {
    sig_sent = true;
    parent_task::stop();
}

// Tell the parent task to exit, and detach self
void exit() {
    task_t _ptask = parent_task::get_task();
    if ( _ptask == NULL ) return;
    task_exit(_ptask);
}

} // namespace parent_task
} // namespace co
} // namespace pe

// Push Chen
