/*
    eventqueue.h
    PECoTask
    2019-11-13
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_TASK_EVENTQUEUE_H__
#define PE_CO_TASK_EVENTQUEUE_H__

#include <peco/cotask/loop.h>
#include <peco/cotask/taskadapter.h>

namespace pe {
    namespace co {

        template < typename _EventItem, typename _ResultItem = _EventItem > 
        class eventqueue {
            typedef struct {
                _EventItem          e;
                _ResultItem*        ret;
                task_t              ot;
            } event_wrapper_t;

            // The internal event queue
            std::list< event_wrapper_t >        eq_;
            task_t                              loop_task_;

        public: 
            typedef std::function< bool (_EventItem&&, _ResultItem&) >  task_handler_t;
        public: 
            eventqueue() : loop_task_(NULL) { }
            ~eventqueue() { }

            // Begin the internal block loop to fetch and process the task
            // This method should be invoke in the loop working task
            void begin_loop( task_handler_t h ) {
                if ( loop_task_ != 0 ) return;
                loop_task_ = this_task::get_task();

                while ( this_task::holding() ) {
                    if ( eq_.size() == 0 ) continue;
                    auto _ew = eq_.front();
                    _ResultItem _lres;
                    bool _r = h(std::move(_ew.e), _lres);
                    // If the pending task has quit during we handle the event
                    if ( eq_.size() == 0 ) continue;
                    if ( eq_.front().ot != _ew.ot ) continue;
                    // Now we still hold the task
                    // Copy the result
                    (*_ew.ret) = std::move(_lres);
                    if ( _r ) task_go_on(_ew.ot);
                    else task_stop(_ew.ot);
                    eq_.pop_front();
                }

                // Tell every pending query to stop
                while ( eq_.size() > 0 ) {
                    task_stop(eq_.front().ot);
                    eq_.pop_front();
                }
            }

            // Stop Loop
            void stop_loop( ) {
                if ( loop_task_ == 0 ) return;
                task_stop( loop_task_ );
                loop_task_ = 0;
            }

            bool ask( _EventItem&& eitem, _ResultItem& ret ) {
                if ( loop_task_ == NULL ) return false;
                eq_.emplace_back((event_wrapper_t){
                    std::move(eitem),
                    &ret,
                    this_task::get_task()
                });
                task_go_on( loop_task_ );
                bool _r = this_task::holding();
                if ( !_r ) {
                    auto _it = find_if(eq_.begin(), eq_.end(), [](const event_wrapper_t& ew) {
                        return ew.ot == this_task::get_task();
                    });
                    if ( _it != eq_.end() ) eq_.erase(_it);
                }
                return _r;
            }
        };

        // Hold and async dispatch event queue
        template< typename _EventItem, typename _KeyItem, typename _ResultItem = _EventItem >
        class asyncqueue {
        public: 
            typedef std::function< _KeyItem( const _EventItem& ) >      get_key_t;
            typedef std::function< bool (_EventItem&&) >                do_req_t;
            typedef std::function< bool (_KeyItem&, _ResultItem& r) >   do_resp_t;
            typedef std::function< bool ( _ResultItem& ) >              until_t;
        protected:
            typedef struct {
                _EventItem          e;
                _ResultItem*        ret;
                bool                os; // One shot
                task_t              ot;
            } event_wrapper_t;

            // The internal event queue
            std::list< event_wrapper_t >            eq_;
            std::map< _KeyItem, event_wrapper_t >   hm_;
            taskadapter*                            adapter_;
            do_req_t                                do_require_;
            do_resp_t                               get_resp_;
            get_key_t                               get_key_;

        protected:
            void make_require_( ) {
                if ( eq_.size() == 0 ) return;
                
                auto& _ew = eq_.front();
                _KeyItem _k = get_key_(_ew.e);
                if ( do_require_(std::move(_ew.e)) ) {
                    hm_.emplace(std::make_pair(_k, std::move(_ew)));
                } else {
                    task_stop(_ew.ot);
                }
                eq_.pop_front();
            }

        public: 

            asyncqueue( ) : adapter_(NULL) { }
            ~asyncqueue() { }

            void begin_loop( get_key_t gk, do_req_t req, do_resp_t resp ) {
                get_key_ = gk; do_require_ = req; get_resp_ = resp;

                _KeyItem _k;
                _ResultItem _r;

                // Create the adapter at the beginning of loop
                adapter_ = new taskadapter;

                while ( get_resp_( _k, _r ) ) {
                    auto _it = hm_.find(_k);
                    if ( _it == hm_.end() ) continue;   // ignore?
                    *_it->second.ret = std::move(_r);
                    task_go_on(_it->second.ot);
                    // If the request is continues, do not remove it
                    if ( _it->second.os ) continue;
                    _it = hm_.find(_k);
                    if ( _it != hm_.end() ) hm_.erase(_it);
                    // hm_.erase(_it);
                }

                // Delete the adapter after loop
                // Safe delete incanse yield at d'str of task adapter
                taskadapter *_padapter = adapter_;
                adapter_ = NULL;
                delete _padapter;

                // Tell every pending query to stop
                while ( eq_.size() > 0 ) {
                    task_stop(eq_.front().ot);
                    eq_.pop_front();
                }
                for ( auto& kv : hm_ ) {
                    task_stop(kv.second.ot);
                }
                // Wait until all ask task quit.
                while ( hm_.size() > 0 ) this_task::yield();
            }

            bool ask( 
                _EventItem&& eitem,
                _ResultItem& ret, 
                duration_t timedout = std::chrono::seconds(3) 
            ) {
                if ( adapter_ == NULL ) return false;
                _KeyItem _k = get_key_(eitem);
                eq_.emplace_back((event_wrapper_t){
                    eitem,
                    &ret,
                    false,
                    this_task::get_task()
                });
                adapter_->step([this]() {
                    this->make_require_();
                });
                bool _r = this_task::holding_until(timedout);
                if ( !_r ) {
                    // Remove
                    auto _it = find_if(eq_.begin(), eq_.end(), [](const event_wrapper_t& e) {
                        return ( e.ot == this_task::get_task() );
                    });
                    // the job hasn't been executed
                    if ( _it != eq_.end() ) {
                        eq_.erase(_it);
                    } else {
                        auto _hit = hm_.find(_k);
                        if ( _hit != hm_.end() ) hm_.erase(_hit);
                    }
                }
                return _r;
            }

            bool askuntil( _EventItem&& eitem, _ResultItem& ret, until_t u ) {
                if ( adapter_ == NULL ) return false;
                _KeyItem _k = get_key_(eitem);
                eq_.emplace_back((event_wrapper_t){
                    eitem,
                    &ret,
                    true,
                    this_task::get_task()
                });
                adapter_->step([this]() {
                    this->make_require_();
                });
                while ( this_task::holding() ) {
                    // if not need to continue, return
                    // Bug Fix!, Force to remove the handler even
                    // when we do not need to continue
                    if ( !u(ret) ) break;
                }
                // Holding failed, break from the while loop, try to remove
                // Remove
                auto _it = find_if(eq_.begin(), eq_.end(), [](const event_wrapper_t& e) {
                    return ( e.ot == this_task::get_task() );
                });
                // the job hasn't been executed
                if ( _it != eq_.end() ) {
                    eq_.erase(_it);
                } else {
                    auto _hit = hm_.find(_k);
                    if ( _hit != hm_.end() ) hm_.erase(_hit);
                }
                // If break from the loop, means task failed, stop signal got
                return false;
            }
        };
    }
}

#endif

// Push Chen
