/*
    condition.h
    PECoTask
    2019-11-24
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_TASK_CONDITION_H__
#define PE_CO_TASK_CONDITION_H__

#include <peco/cotask/cotask.hpp>
#include <peco/cotask/loop.h>

namespace pe {
    namespace co {

        // Dispatcher
        class dispatcher {
        private: 
            int                         c_rw_[2];
            task *                      waiting_task_;
            int                         notify_flag_;
            std::map< task*, bool >     pending_tasks_;

            // Internal Wait
            void wait_();
        public: 
            // Create and initialize the internal fd
            dispatcher( );
            // Destory and cancel all waiting task
            ~dispatcher( );

            // Wait for the event
            bool wait( );
            // Wait for the event until the timeout
            bool wait_until( std::chrono::nanoseconds t );

            // Notify one task
            void notify_one();
            // Notify all task
            void notify_all();
        };

        // Simple block condition
        class condition {
        public:
            typedef std::function< bool ( void ) >      cond_t;

        private:
            dispatcher          c_dp_;
        public: 
            // Create and initialize the internal fd
            condition();
            // Destory and cancel all waiting task
            ~condition();

            // Wait for the condition to become true
            bool wait( cond_t c );

            // Wait for the condition until the timeout
            bool wait_until( cond_t c, std::chrono::nanoseconds t );

            // After change the condition, and give the signal for waiting task
            void notify();
        };

        // Semaphore Type Condition
        class semaphore {
        private: 

            // Two fd for read and write
            int                 c_rw_[2];

            // Semaphore Count
            volatile uint32_t            c_count_;
            // Pending task reference count
            volatile uint32_t            c_ref_;
        public: 
            // Initialize the count 
            semaphore();
            // Destory and cancel all waiting task
            ~semaphore();

            // Check the count or wait until it is bigger than 0
            bool fetch();

            // Check the count or wait some time
            bool fetch_until( std::chrono::nanoseconds t );

            // Give a signal and increase the count
            void give();

            // Cancel all fetch task
            void cancel();
        };
    }
}

#endif

// Push Chen
