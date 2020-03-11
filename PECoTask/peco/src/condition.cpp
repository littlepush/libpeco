/*
    condition.cpp
    PECoTask
    2019-11-24
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/condition.h>

#if PZC_TARGET_LINUX
// Use eventfd in linux
#include <sys/eventfd.h>
#endif

#include <peco/peutils.h>

namespace pe {
    namespace co {
        dispatcher::dispatcher( ) : waiting_task_(NULL), notify_flag_(0) {
        #if PZC_TARGET_LINUX
            c_rw_[0] = eventfd(0, 0);
            // Copy the first, not dup
            c_rw_[1] = c_rw_[0];
        #else
            pipe(c_rw_);
        #endif
            waiting_task_ = loop::main.do_job([this]() {
                this_task::set_user_flag0(1);
                this->wait_();
            });
        }
        // Destory and cancel all waiting task
        dispatcher::~dispatcher( ) {
            task_set_user_flag0(waiting_task_, 0);
            // waiting_task_->reserved_flags.flag0 = 0;
            tasks_cancel_fd(c_rw_[0]);
        #if PZC_TARGET_LINUX
            ::close(c_rw_[0]);
        #else
            ::close(c_rw_[0]);
            ::close(c_rw_[1]);
        #endif
            c_rw_[0] = -1;
            c_rw_[1] = -1;
            while ( task_get_user_flag0(waiting_task_) != 1 ) {
            // while ( waiting_task_->reserved_flags.flag1 != 1 ) {
                this_task::yield();
            }
        }

        void dispatcher::wait_() {
            while ( this_task::get_user_flag0() ) {
                while ( this_task::get_user_flag0() ) {
                    waiting_signals _sig = this_task::wait_fd_for_event(
                        c_rw_[0], event_read, std::chrono::milliseconds(100)
                    );
                    if ( this_task::get_user_flag0() == 0 ) {
                        break;
                    }
                    if ( _sig == bad_signal ) {
                        // Cancel all
                        for ( auto& tkv : pending_tasks_ ) {
                           task_stop(tkv.first);
                        }
                        pending_tasks_.clear();
                        break;
                    }
                    if ( _sig == no_signal ) continue;
                    uint64_t _u = 0;
                    // Fetch the notification
                    ignore_result(::read(c_rw_[0], &_u, sizeof(uint64_t)));

                    if ( notify_flag_ == 1 ) {
                        // Notify one
                        auto _tb = pending_tasks_.begin();
                        task_go_on(_tb->first);
                        pending_tasks_.erase(_tb);
                    } else {
                        // Notify all
                        for ( auto& tkv : pending_tasks_ ) {
                            task_go_on(tkv.first);
                        }
                        pending_tasks_.clear();
                    }
                    break;
                }
                // Reset the notify flag
                notify_flag_ = 0;
            }
            this_task::set_user_flag0(1);
        }

        // Wait for the event
        bool dispatcher::wait( ) {
            pending_tasks_[this_task::get_task()] = true;
            return this_task::holding();
        }
        // Wait for the event until the timeout
        bool dispatcher::wait_until( std::chrono::nanoseconds t ) {
            task_t _ptask = this_task::get_task();
            pending_tasks_[_ptask] = true;
            bool _r = this_task::holding_until(t);
            if ( task_get_waiting_signal(_ptask) == pe::co::no_signal ) {
                // Force to erase current task cause its timeout
                pending_tasks_.erase(this_task::get_task());
            }
            return _r;
        }

        // Notify one task
        void dispatcher::notify_one() {
            notify_flag_ = 1;
            uint64_t _c = 1;
            ignore_result(::write(c_rw_[1], &_c, sizeof(uint64_t)));
        }
        // Notify all task
        void dispatcher::notify_all() {
            notify_flag_ = 2;
            uint64_t _c = 1;
            ignore_result(::write(c_rw_[1], &_c, sizeof(uint64_t)));
        }

        condition::condition() { }

        condition::~condition() { }

        // Wait for the condition to become true
        bool condition::wait( cond_t c ) {
            if ( c == nullptr ) return c_dp_.wait();
            else {
                if ( c() == true ) return true;
                return c_dp_.wait() && c();
            }
        }

        // Wait for the condition until the timeout
        bool condition::wait_until( cond_t c, std::chrono::nanoseconds t ) {
            if ( c == nullptr ) return c_dp_.wait_until(t);
            else {
                if ( c() == true ) return true;
                return c_dp_.wait_until(t) && c();
            }
        }

        // After change the condition, and give the signal for all waiting task
        void condition::notify() {
            c_dp_.notify_all();
        }

        // Initialize the count 
        semaphore::semaphore() : c_count_(0), c_ref_(0) {
        #if PZC_TARGET_LINUX
            c_rw_[0] = eventfd(0, 0);
            // Copy the first, not dup
            c_rw_[1] = c_rw_[0];
        #else
            pipe(c_rw_);
        #endif
        }
        // Destory and cancel all waiting task
        semaphore::~semaphore() {
            this->cancel();
        }

        // Check the count or wait until it is bigger than 0
        bool semaphore::fetch() {
            // Direct return if the count is greater than 0
            if ( c_count_ > 0 ) {
                --c_count_; return true;
            }

            // Increase the reference count
            ++c_ref_;
            waiting_signals _sig = no_signal;
            do {
                _sig = this_task::wait_fd_for_event(
                    c_rw_[0], event_read, std::chrono::seconds(1)
                );
                if ( _sig == bad_signal ) {
                    // Bad Signal not because the semaphore been destoried
                    if ( this_task::get_status() == task_status_stopped ) {
                        --c_ref_;
                    }
                    return false;
                }
                // Closed
                if ( c_rw_[0] == -1 ) return false;
            } while ( _sig == no_signal );

            // Force to get the data
            uint64_t _u = 0;
            ignore_result(::read(c_rw_[0], &_u, sizeof(uint64_t)));

            // Decrease the reference count and the left count
            --c_ref_;
            --c_count_;
            return true;
        }

        // Check the count or wait some time
        bool semaphore::fetch_until( std::chrono::nanoseconds t ) {
            // Direct return if the count is greater than 0
            if ( c_count_ > 0 ) {
                --c_count_; return true;
            }

            // Increase the reference count
            ++c_ref_;

            waiting_signals _sig = this_task::wait_fd_for_event(
                c_rw_[0], event_read, t);
            if ( _sig == receive_signal ) {
                uint64_t _u = 0;
                ignore_result(::read(c_rw_[0], &_u, sizeof(uint64_t)));

                --c_ref_;
                --c_count_;
                return true;
            } else if ( _sig == no_signal ) {
                --c_ref_;
            } else {
                // Bad Signal not because the semaphore been destoried
                if ( this_task::get_status() == task_status_stopped ) {
                    --c_ref_;
                }
            }
            return false;
        }

        // Give a signal and increase the count
        void semaphore::give() {
            ++c_count_;
            // If no one is pending, do not need to send any signal
            if ( c_ref_ == 0 ) return;

            uint64_t _c = 1;
            ignore_result(::write(c_rw_[1], &_c, sizeof(uint64_t)));
        }

        // Cancel all fetch task
        void semaphore::cancel() {
            if ( c_rw_[0] == -1 ) return;

            tasks_cancel_fd(c_rw_[0]);
        #if PZC_TARGET_LINUX
            ::close(c_rw_[0]);
        #else
            ::close(c_rw_[0]);
            ::close(c_rw_[1]);
        #endif
            c_rw_[0] = -1;
            c_rw_[1] = -1;
        }
    }
}

// Push Chen
