/*
    loop.cpp
    PECoTask
    2019-05-07
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/loop.h>
#include <signal.h>
#ifdef __APPLE__
#include <peco/cotask/kevent_wrapper.h>
#else
#include <peco/cotask/epoll_wrapper.h>
#endif

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <peco/cotask/mempage.h>
#include <peco/peutils.h>

// Only in gun
#ifdef __LIBPECO_USE_GNU__
#include <execinfo.h>
#endif

#include <thread>

extern "C" {
    int PECoTask_Autoconf() { return 0; }
}
namespace pe {
    namespace co {
        // The thread local loop point
        thread_local loop * __this_loop = NULL;
        // The main loop
        thread_local loop this_loop;

        ON_DEBUG(
            bool g_cotask_trace_flag = false;
            void enable_cotask_trace() {
                g_cotask_trace_flag = true;
            }
            bool is_cotask_trace_enabled() {
                return g_cotask_trace_flag;
            }
        )

        // Dump Stack Info
        void __dump_call_stack( FILE* fd ) {
        #ifdef __LIBPECO_USE_GNU__
            void * _stack[64];
            size_t _ssize = backtrace(_stack, 64);
            backtrace_symbols_fd(_stack, _ssize, fileno(fd));
        #endif
        }

        typedef void (*context_job_t)(void);
		// Run job wrapper
        #if FORCE_USE_UCONTEXT
        inline void __job_run( task_job_t* job )
        #else
        inline void __job_run( int sig )
        #endif
        {
        #ifndef FORCE_USE_UCONTEXT
            task* _ptask = __this_loop->get_running_task();
            auto job = _ptask->pjob;
            if ( !setjmp(_ptask->buf) ) return;
        #endif
            try {
                (*job)(); 
            } catch( const char * msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch( std::string& msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch( std::stringstream& msg ) {
                std::cerr << "exception: " << msg.str() << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch ( std::logic_error& ex ) {
                std::cerr << "logic error: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch ( std::runtime_error& ex ) {
                std::cerr << "runtime error: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch ( std::bad_exception& ex ) {
                std::cerr << "bad exception: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            } catch( ... ) {
                std::exception_ptr _ep = std::current_exception();
                if ( _ep ) {
                    try {
                        std::rethrow_exception(_ep);
                    } catch(const std::exception& e ) {
                        std::cerr << "caught exception: " << e.what();
                    }
                } else {
                    std::cerr << "unknow exception caught";
                }
                std::cerr << ", task info: " << std::endl;
                task_debug_info( __this_loop->get_running_task(), stderr );
            }
            #ifndef FORCE_USE_UCONTEXT
            // Long Jump back to the main context
            __this_loop->swap_to_main_loop();
            #endif
        }

        task* __create_task( task_job_t job ) {

            // The task should be put on the heap
            task * _ptask = (task *)malloc(sizeof(task));
            _ptask->id = (intptr_t)(_ptask);
            _ptask->exitjob = NULL;

            // Init the stack for the task
            // Get a 1MB Memory piece
            #ifdef LOOP_USE_MEMPAGE
            _ptask->stack = mu::mem::page::main.get_piece();
            #else
            _ptask->stack = (stack_ptr)malloc(mu::mem::page::piece_size);
            #endif

            // Save the job on heap
            _ptask->pjob = new task_job_t(job);

            #ifdef FORCE_USE_UCONTEXT

            // Init the context
            getcontext(&_ptask->ctx);
            _ptask->ctx.uc_stack.ss_sp = _ptask->stack;
            _ptask->ctx.uc_stack.ss_size = mu::mem::page::piece_size;
            _ptask->ctx.uc_stack.ss_flags = 0;
            _ptask->ctx.uc_link = __this_loop->get_main_context();
            makecontext(&_ptask->ctx, (context_job_t)&__job_run, 1, _ptask->pjob);

            #else

            _ptask->task_stack.ss_flags = 0;
            _ptask->task_stack.ss_size = mu::mem::page::piece_size;
            _ptask->task_stack.ss_sp = _ptask->stack;

            #endif

            // Init the status
            _ptask->status = task_status_pending;
            _ptask->is_event_id = false;

            // Set other flags
            _ptask->hold_sig[0] = -1;
            _ptask->hold_sig[1] = -1;
            _ptask->related_fd = -1;

            _ptask->arg = NULL;

            // Set the parent task
            _ptask->p_task = __this_loop->get_running_task();
            // I have no child task
            _ptask->c_task = NULL;
            // I have no newer sibling node
            _ptask->prev_task = NULL;
            // Init my next sibline node to NULL
            _ptask->next_task = NULL;

            // If im not the root task
            if ( _ptask->p_task != NULL ) {
                // My next task is the last child task of my parent
                _ptask->next_task = _ptask->p_task->c_task;
                // If im not the oldest
                if ( _ptask->next_task != NULL ) _ptask->next_task->prev_task = _ptask;
                // My Parent's newest task is me
                _ptask->p_task->c_task = _ptask;
            }

            memset(&_ptask->reserved_flags, 0, sizeof(_ptask->reserved_flags));

            return _ptask;
        }

        void __reset_task( task* ptask ) {
        #if FORCE_USE_UCONTEXT
            getcontext(&ptask->ctx);
            ptask->ctx.uc_stack.ss_sp = ptask->stack;
            ptask->ctx.uc_stack.ss_size = mu::mem::page::piece_size;
            ptask->ctx.uc_stack.ss_flags = 0;
            ptask->ctx.uc_link = __this_loop->get_main_context();

            // Save the job on heap
            makecontext(&ptask->ctx, (context_job_t)&__job_run, 1, ptask->pjob);
        #else

            // Reset the task stack to the beginning
            ptask->task_stack.ss_sp = ptask->stack;
            ptask->task_stack.ss_flags = 0;
            // make this task looks like a new one
            ptask->status = task_status_pending;
        #endif
        }

        task* __create_task_with_fd( int fd, task_job_t job ) {
            task * _ptask = __create_task( job );
            _ptask->id = (intptr_t)fd;
            _ptask->is_event_id = true;
            return _ptask;
        }

        void __destory_task( task * ptask ) {
            if ( ptask->exitjob ) {
                (*ptask->exitjob)();
                delete ptask->exitjob;
                ptask->exitjob = NULL;
            }
            delete ptask->pjob;
            if ( ptask->hold_sig[0] != -1 ) {
                ::close(ptask->hold_sig[0]);
            }
            if ( ptask->hold_sig[1] != -1 ) {
                ::close(ptask->hold_sig[1]);
            }
            if ( ptask->is_event_id ) {
                ::close(ptask->id);
            }

            #ifdef LOOP_USE_MEMPAGE
            mu::mem::page::main.release_piece(ptask->stack);
            #else
            free(ptask->stack);
            #endif

            // Clean the relationship
            while ( ptask->c_task != NULL ) {
                // tell my children i am die
                ptask->c_task->p_task = NULL;
                ptask->c_task = ptask->c_task->next_task;
            }
            // If I have an younger brother
            if ( ptask->prev_task != NULL ) {
                ptask->prev_task->next_task = ptask->next_task;
            }
            // If I have an elder brother
            if ( ptask->next_task != NULL ) {
                ptask->next_task->prev_task = ptask->prev_task;
            }

            free(ptask);
        }

        // Internal task switcher
        void loop::__switch_task( task* ptask ) {
            #ifndef FORCE_USE_UCONTEXT
            auto _tstatus = ptask->status;
            #endif
            if ( ptask->status != task_status_stopped ) {
                ptask->status = task_status_running;
            }

            running_task_ = ptask;

            #ifdef FORCE_USE_UCONTEXT
            // If the task finishs and quit, then will continue
            // the main loop because we set uc_link as the main ctx
            // when we create the task
            // otherwise, the yield function will change the
            // next_time of the task and insert it back to
            // the task cache.
            ON_DEBUG_COTASK(
                std::cout << CLI_BLUE << "X main => " << ptask->id << CLI_NOR << std::endl;
            )
            swapcontext(&main_ctx_, &ptask->ctx);
            #else

            // Set to use the target task's stack to handler the signal, 
            // which means context swap
            // New task
            if ( _tstatus == task_status_pending ) {
                // Raise the signal to switch to the stack
                // And the handler will call setjmp and return immediately
                sigaltstack(&ptask->task_stack, NULL);

                // register the signal to run job
                struct sigaction    sa;            
                sa.sa_handler = &__job_run;
                sa.sa_flags = SA_ONSTACK;
                sigemptyset(&sa.sa_mask);
                sigaction(SIGUSR1, &sa, NULL);

                raise(SIGUSR1);
            }
            // Save current main jmp's position and jump to the 
            // task stack
            if ( !setjmp(main_jmp_) ) {
                longjmp(ptask->buf, 1);
            }

            #endif
            // If the task's status is running, means the last task just
            // finished and quit, then check if the task is a oneshot task
            // if not, then decrease the repeat count and put back to
            // the task cache
            running_task_ = NULL;
            // last task has been finished, then check if it is loop
            if ( ptask->status == task_status::task_status_running ) {
                if ( ptask->repeat_count == 1 ) {
                    // Clear the task and release the memory
                    __destory_task(ptask);
                    --task_count_;
                    return;
                }
                if ( ptask->repeat_count != infinitive_task ) {
                    --ptask->repeat_count;
                }
                ptask->next_time += ptask->duration;
                __reset_task(ptask);
                this->insert_task(ptask);
            } else if ( ptask->status == task_status::task_status_stopped ) {
                // The task has been stopped by someone, quit it
                __destory_task(ptask);
                --task_count_;
                return;
            }
            // else, the task is paused, do nothing
        }

        // Default C'str
        loop::loop() : 
            core_fd_(-1), core_events_(NULL), 
            running_task_(NULL), task_count_(0), 
            running_(false), ret_code_(0) 
        { 
            #ifndef FORCE_USE_UCONTEXT

            if ( __this_loop != NULL ) {
                throw std::runtime_error("cannot create more than one loop in one thread");
            }
            __this_loop = this;

            #endif

            nearest_timeout_ = task_time_now();
        }
        // Default D'str
        loop::~loop() {
            if ( core_fd_ == -1 ) return;
            if ( running_task_ != NULL ) {
                __destory_task(running_task_);
                --task_count_;
            }
            #if COTASK_ON_SAMETIME
            for ( auto& lt : task_cache_ ) {
                for ( auto pt : lt.second ) {
                    __destory_task(pt);
                    --task_count_;
                }
            }
            #else
            for ( auto& pt : task_cache_ ) {
                __destory_task(pt.second);
                --task_count_;
            }
            #endif
            for ( auto& et : read_event_cache_ ) {
                ::close( (long)et.first );
                __destory_task(et.second);
                --task_count_;
            }
            for ( auto& et : write_event_cache_ ) {
                ::close( (long)et.first );
                __destory_task(et.second);
                --task_count_;
            }
            ::close(core_fd_);
            free(core_events_);
        }

		// Get current running task
        task* loop::get_running_task() { return running_task_; }

        // Get task count
        uint64_t loop::task_count() const { return task_count_; }
        // Get the return code
        int loop::return_code() const { return ret_code_; }

        #ifdef FORCE_USE_UCONTEXT
        // Get the main context
        ucontext_t* loop::get_main_context() { return &main_ctx_; }
        #else
        jmp_buf* loop::get_main_jmpbuf() { return &main_jmp_; }
        #endif
        
        // Just add a will formated task
        void loop::insert_task( task * ptask ) {
            #if COTASK_ON_SAMETIME
            auto _lt = task_cache_.find(ptask->next_time);
            if ( _lt == task_cache_.end() ) {
                task_cache_[ptask->next_time] = (task_list_t){ptask};
            } else {
                _lt->second.push_back(ptask);
            }
            #else
            task_cache_[ptask->next_time] = ptask;
            #endif
        }
        // Remove the task from timed-cache
        void loop::remove_task( task * ptask ) {
            auto _lt = task_cache_.find(ptask->next_time);
            if ( _lt == task_cache_.end() ) return;
            for ( auto _i = _lt->second.begin(); 
                _i != _lt->second.end(); ++_i )
            {
                if ( *_i != ptask ) continue;
                _lt->second.erase(_i);
                if ( _lt->second.size() == 0 ) {
                    task_cache_.erase(_lt);
                }
                return;
            }
        }
		// Yield current task and switch to main loop
		void loop::swap_to_main_loop() {
            ON_DEBUG_COTASK(
                std::cout << "X " << running_task_->id << " => main" << std::endl;
            )
            #ifdef FORCE_USE_UCONTEXT
            swapcontext(&running_task_->ctx, &main_ctx_);
            // setcontext(&main_ctx_);
            #else
            if ( !setjmp(running_task_->buf) ) {
                longjmp(main_jmp_, 1);
            }
            #endif
        }

        task * loop::do_job( task_job_t job ) {
            task *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();
            this->insert_task(_ptask);

            // Switch to main and run the task
            this_task::yield();
            return _ptask;
        }

        task * loop::do_job( long fd, task_job_t job ) {
            if ( fd == -1 ) return NULL;
            task *_ptask = __create_task_with_fd( fd, job );
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();
            this->insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        task * loop::do_loop( task_job_t job, duration_t interval ) {
            task *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = infinitive_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();
            _ptask->duration = interval;
            this->insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        task * loop::do_delay( task_job_t job, duration_t delay ) {
            task *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now() + delay;
            this->insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        // Event Monitor
        bool loop::monitor_task( int fd, task * ptask, event_type eventid, duration_t timedout ) {
            // Add to the core_events
            int _c_eid = (eventid == event_type::event_read ? core_event_read : core_event_write);
            if ( eventid == event_type::event_read ) {
                if ( read_event_cache_.find( fd ) != read_event_cache_.end() ) 
                    return false;
#ifndef __APPLE__
                if ( write_event_cache_.find( fd ) != write_event_cache_.end() ) {
                    _c_eid |= core_event_write;
                }
#endif
            } else {
                if ( write_event_cache_.find( fd ) != write_event_cache_.end() ) 
                    return false;
#ifndef __APPLE__
                if ( read_event_cache_.find( fd ) != read_event_cache_.end() ) {
                    _c_eid |= core_event_read;
                }
#endif
            }
            core_event_t _e;
            core_event_init(_e);
            core_event_prepare(_e, fd);
            core_make_flag(_e, core_flag_add);
            core_mask_flag(_e, core_flag_oneshot);
            if ( -1 == core_event_ctl_with_flag(core_fd_, fd, _e, _c_eid) ) 
                return false;
          
            // If the timedout is 0, which means the monitoring will stop
            // after next schedule loop. 
            if ( eventid == event_type::event_read ) {
                ptask->read_until = task_time_now() + timedout;
                ON_DEBUG_COTASK(
                    std::cout << "* MR <" << fd << ":" << ptask->id 
                        << "> for read event, timedout at "
                        << ptask->read_until.time_since_epoch().count() << std::endl;
                )
                read_event_cache_[ fd ] = ptask;
            } else {
                ptask->write_until = task_time_now() + timedout;
                ON_DEBUG_COTASK(
                    std::cout << "* MW <" << fd << ":" << ptask->id 
                        << "> for write event, timedout at "
                        << ptask->write_until.time_since_epoch().count() << std::endl;
                )
                write_event_cache_[ fd ] = ptask;
            }
            return true;
        }

        // Cancel all monitored item of a task
        void loop::cancel_monitor( task * ptask ) {
            if ( ptask == NULL ) return;

            ON_DEBUG_COTASK(
                std::cout << "a task will be cancelled: " << ptask->id << std::endl;
                task_debug_info( ptask );
            )
            if ( ptask->related_fd != -1 ) {
                auto _rit = read_event_cache_.find( ptask->related_fd );
                if ( _rit != read_event_cache_.end() && _rit->second == ptask ) {
                    ON_DEBUG_COTASK(
                        std::cout << "the task is waiting on " << ptask->related_fd 
                            << " for incoming, now remove it." << std::endl;
                    )
                    ptask->read_until = task_time_now();
                }
                auto _wit = write_event_cache_.find( ptask->id );
                if ( _wit != write_event_cache_.end() && _rit->second == ptask ) {
                    ON_DEBUG_COTASK(
                        std::cout << "the task is waiting on " << ptask->related_fd
                            << " for write buffer, now remove it." << std::endl;
                    )
                    ptask->write_until = task_time_now();
                }
            }
        }

        // Cancel all monitored task of specified fd
        void loop::cancel_fd( long fd ) {
            auto _rit = read_event_cache_.find( fd );
            if ( _rit != read_event_cache_.end() ) {
                _rit->second->read_until = task_time_now();
            }
            auto _wit = write_event_cache_.find( fd );
            if ( _wit != write_event_cache_.end() ) {
                _wit->second->write_until = task_time_now();
            }
        }

        // Timed task schedule
        void loop::__timed_task_schedule() {
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __timed_task_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )
            while ( task_cache_.size() > 0 && task_cache_.begin()->first <= _now ) {
                #if COTASK_ON_SAMETIME
                auto& _tl = task_cache_.begin()->second;

                while ( _tl.size() > 0 ) {
                    // Fetch and remove the first task
                    task *_ptask = *_tl.begin();

                    // !! Key line !!, remove from the cache each time
                    _tl.pop_front();
                    __switch_task( _ptask );
                }

                task_cache_.erase(task_cache_.begin());
                #else
                task *_ptask = task_cache_.begin()->second;
                task_cache_.erase( task_cache_.begin() );
                __switch_task(_ptask);
                #endif
            }
        }
        // Event Task schedule
        void loop::__event_task_schedule() {
            if ( 
                task_cache_.size() == 0 && 
                read_event_cache_.size() == 0 && 
                write_event_cache_.size() == 0 
            ) return;

            uint32_t _wait_time = 1000; // default 1000ms
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __event_task_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )
            if ( task_cache_.size() > 0 ) {
                auto _wait_to = task_cache_.begin()->first;
                if ( _wait_to > _now ) {
                    auto _ts = _wait_to - _now;
                    auto _ms = std::chrono::duration_cast< std::chrono::milliseconds >(_ts);
                    _wait_time = (uint32_t)_ms.count();
                } else {
                    _wait_time = 0;
                }
            }
            if ( _wait_time > 0 ) {
                if ( read_event_cache_.size() > 0 || write_event_cache_.size() > 0 ) {
                    // Check the nearest timedout time
                    if ( nearest_timeout_ > _now ) {
                        auto _ts = nearest_timeout_ - _now;
                        auto _ms = std::chrono::duration_cast< std::chrono::milliseconds >(_ts);
                        if ( (uint32_t)_ms.count() < _wait_time ) {
                            _wait_time = (uint32_t)_ms.count();
                        }
                    } else {
                        _wait_time = 0;
                    }
                }
            }
            int _count = core_event_wait(
                core_fd_, core_events_, CO_MAX_SO_EVENTS, _wait_time
            );
            ON_DEBUG_COTASK(
                std::cout << CLI_BLUE << "∑ E: " << _count << ", RC: " << read_event_cache_.size()
                    << ", WC: " << write_event_cache_.size() << CLI_NOR << std::endl;
            )
            if ( _count == -1 ) return;
            // If have any event, then do something
            // If not, then continue the loop
            task *_ptask = NULL;
            for ( int i = 0; i < _count; ++i ) {
                core_event_t *_pe = core_events_ + i;
                int _fd = (intptr_t)core_get_so(_pe);

                do {
                    // We only get outgoing event, do nothing in
                    // read event block
                    if ( core_is_outgoing(_pe) ) break;

                    // Find the task in the read cache
                    auto _rt = read_event_cache_.find(_fd);
                    if ( _rt == read_event_cache_.end() ) break;

                    _ptask = _rt->second;
                    if ( core_is_error_event(_pe) ) {
                        _ptask->signal = waiting_signals::bad_signal;
                    } else {
                        _ptask->signal = waiting_signals::receive_signal;
                    }
                    read_event_cache_.erase(_rt);
                    __switch_task(_ptask);
                } while ( false );

                do {
                    // We only get incoming event, do nothing in 
                    // write event block
                    if ( core_is_incoming(_pe) ) break;

                    // Find the task in the write cache
                    auto _wt = write_event_cache_.find(_fd);
                    if ( _wt == write_event_cache_.end() ) break;

                    _ptask = _wt->second;
                    if ( core_is_error_event(_pe) ) {
                        _ptask->signal = waiting_signals::bad_signal;
                    } else {
                        _ptask->signal = waiting_signals::receive_signal;
                    }
                    write_event_cache_.erase(_wt);
                    __switch_task(_ptask);
                } while ( false );

            #ifndef __APPLE__
                // For EPOLL, need to re-add the event.
                if ( read_event_cache_.find(_fd) != read_event_cache_.end() ) {
                    // Read not timedout, do it in the next runloop
                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, _fd);
                    core_make_flag(_e, core_flag_add);
                    core_mask_flag(_e, core_flag_oneshot);
                    core_event_ctl_with_flag(core_fd_, _fd, _e, core_event_read);
                }
                if ( write_event_cache_.find(_fd) != write_event_cache_.end() ) {
                    // Write not timedout
                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, _fd);
                    core_make_flag(_e, core_flag_add);
                    core_mask_flag(_e, core_flag_oneshot);
                    core_event_ctl_with_flag(core_fd_, _fd, _e, core_event_write);
                }
            #endif
            }
        }
        // Event Task Timedout check
        void loop::__event_task_timedout_check() {
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __event_task_timedout_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )

            if ( read_event_cache_.size() > 0 ) {
                this->nearest_timeout_ = read_event_cache_.begin()->second->read_until;
            } else {
                if ( write_event_cache_.size() > 0 ) {
                    this->nearest_timeout_ = write_event_cache_.begin()->second->write_until;
                }
            }

            if ( read_event_cache_.size() > 0 ) {
                auto _rt = read_event_cache_.begin();
                do {
                    task *_ptask = _rt->second;
                    if ( _ptask->read_until > _now ) {
                        if ( _ptask->read_until < nearest_timeout_ ) 
                            nearest_timeout_ = _ptask->read_until;
                        ++_rt; continue;
                    }

                    ON_DEBUG_COTASK(
                        std::cout << CLI_YELLOW << "≈ task has timedout on waiting incoming for fd: " 
                            << _rt->first << CLI_NOR << std::endl;
                        task_debug_info(_ptask);
                    )

                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, (int)_rt->first);
                    core_event_del(core_fd_, _e, (int)_rt->first, core_event_read);

                    _rt = read_event_cache_.erase(_rt);
                    _ptask->signal = waiting_signals::no_signal;
                    __switch_task(_ptask);
                } while ( _rt != read_event_cache_.end() );
            }
            if ( write_event_cache_.size() > 0 ) {
                auto _wt = write_event_cache_.begin();
                do {
                    task *_ptask = _wt->second;
                    if ( _ptask->write_until > _now ) {
                        if ( _ptask->write_until < nearest_timeout_ ) 
                            nearest_timeout_ = _ptask->write_until;
                        ++_wt; continue;
                    }

                    ON_DEBUG_COTASK(
                        std::cout << CLI_YELLOW << "≈ task has timedout on waiting write buffer for fd: " 
                            << _wt->first << CLI_NOR << std::endl;
                        task_debug_info(_ptask);
                    )

                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, (int)_wt->first);
                    core_event_del(core_fd_, _e, (int)_wt->first, core_event_write);

                    _wt = write_event_cache_.erase(_wt);
                    _ptask->signal = waiting_signals::no_signal;
                    __switch_task(_ptask);
                } while ( _wt != write_event_cache_.end() );
            }
        }

        // The entry point of the main loop
        void loop::run() {
            // Init the core_fd and core_events
            if ( core_fd_ != -1 ) return;

            // Initalize to ignore some sig
            // SIGPIPE
            signal(SIGPIPE, SIG_IGN);

            core_fd_create(core_fd_);
            core_events_ = (core_event_t *)calloc(CO_MAX_SO_EVENTS, sizeof(core_event_t));
 
            running_ = true;
            while (
                task_cache_.size() > 0 ||
                read_event_cache_.size() > 0 ||
                write_event_cache_.size() > 0
            ) {
                if ( running_ == false ) break;
                // Schedule all time-based task first
                this->__timed_task_schedule();

                // Event based task timedout checking
                this->__event_task_timedout_check();

                // Event based task schedule
                this->__event_task_schedule();
            }

            // bread from the loop, clear everything
            if ( running_task_ != NULL ) {
                __destory_task(running_task_);
                --task_count_;
            }
            #if COTASK_ON_SAMETIME
            for ( auto& lt : task_cache_ ) {
                for ( auto pt : lt.second ) {
                    __destory_task(pt);
                    --task_count_;
                }
            }
            #else
            for ( auto& pt : task_cache_ ) {
                __destory_task(pt.second);
                --task_count_;
            }
            #endif
            for ( auto& et : read_event_cache_ ) {
                ::close( (long)et.first );
                __destory_task(et.second);
                --task_count_;
            }
            for ( auto& et : write_event_cache_ ) {
                ::close( (long)et.first );
                __destory_task(et.second);
                --task_count_;
            }
            ::close(core_fd_);
            free(core_events_);
            core_fd_ = -1;
        }
        // Tell if current loop is running
        bool loop::is_running() const {
            return (core_fd_ != -1);
        }

        // Exit Current Loop
        void loop::exit( int code ) {
            ret_code_ = code;
            running_ = false;
            this_task::yield();
        }

        void task_debug_info( task* ptask, FILE *fp, int lv ) {
            __dump_call_stack(fp);

            if ( ptask == NULL ) {
                fprintf(fp, "%*s|= <null>\n", lv * 4, ""); return;
            }

            fprintf(fp, "%*s|= id: %ld\n", lv * 4, "", ptask->id);
            fprintf(fp, "%*s|= is_event: %s\n", lv * 4, "", (ptask->is_event_id ? "true" : "false"));
        #if FORCE_USE_UCONTEXT
            fprintf(fp, "%*s|= stack: %s\n", lv * 4, "", pe::utils::ptr_str(ptask->stack).c_str());
        #endif
            fprintf(fp, "%*s|= status: %s\n", lv * 4, "", 
                (ptask->status == task_status_pending ? "pending" : 
                    (ptask->status == task_status_paused ? "paused" : 
                        (ptask->status == task_status_stopped ? "stopped" : 
                            "running"))));
            fprintf(fp, "%*s|= signal: %s\n", lv * 4, "", (ptask->signal == no_signal ? "no_signal" : 
                (ptask->signal == bad_signal ? "bad_signal" : "receive_signal")));
            fprintf(fp, "%*s|= hold_sig: %d, %d\n", lv * 4, "", ptask->hold_sig[0], ptask->hold_sig[1]);
            fprintf(fp, "%*s|= related_fd: %ld\n", lv * 4, "", ptask->related_fd);
            if ( ptask->repeat_count == infinitive_task ) {
                fprintf(fp, "%*s|= repeat_count: infinitive\n", lv * 4, "");
            } else {
                fprintf(fp, "%*s|= repeat_count: %lu\n", lv * 4, "", ptask->repeat_count);
            }
            fprintf(fp, "%*s|= flags: %x\n", lv * 4, "", *(uint8_t *)&ptask->reserved_flags);
            if ( ptask->p_task != NULL ) {
                fprintf(fp, "%*s|= parent: \n", lv * 4, "");
                task_debug_info( ptask->p_task, fp, lv + 1 );
            }
        }
        // Force to exit a task
        void task_exit( task * ptask ) {
            if ( ptask == NULL || ptask == __this_loop->get_running_task() ) return;
            ptask->status = task_status_stopped;
            __this_loop->cancel_monitor(ptask);
            __this_loop->remove_task(ptask);
            ptask->next_time = task_time_now();
            ptask->repeat_count = 1;
            // Only put back a task not holding anything
            if ( ptask->related_fd == -1 ) {
                __this_loop->insert_task(ptask);
            }
        }
        // Go on a task
        void task_go_on( task * ptask ) {
            if ( ptask == NULL ) return;
            if ( ptask->hold_sig[1] == -1 ) return;
            ignore_result(write( ptask->hold_sig[1], "1", 1 ));
        }

        // Stop a task's holding
        void task_stop( task * ptask ) {
            if ( ptask == NULL ) return;
            if ( ptask->hold_sig[1] != -1 ) {
                ignore_result(write( ptask->hold_sig[1], "0", 1 ));
            }
            __this_loop->cancel_monitor(ptask);
        }
        // Register task's exit job
        void task_register_exit_job(task * ptask, task_job_t job) {
            if ( ptask->exitjob ) {
                delete ptask->exitjob;
            }
            ptask->exitjob = new task_job_t(job);
        }

        // Remove task exit job
        void task_remove_exit_job(task * ptask) {
            if ( ptask->exitjob == NULL ) return;
            delete ptask->exitjob;
            ptask->exitjob = NULL;
        }

        namespace this_task {
            // Get the task point
            task * get_task() {
                return __this_loop->get_running_task();
            }
            // Wrapper current task
            other_task wrapper() {
                return other_task(__this_loop->get_running_task());
            }
            // Get the task id
            task_id get_id() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return 0;
                return _ptask->id;
            }
            // Cancel loop inside
            void cancel_loop() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return;
                _ptask->repeat_count = 1;
            }
            // Force to switch to main context
            void yield() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return;

                // Will not switch task if it already been masked as stopped.
                if ( _ptask->status == task_status_stopped ) return;
                // Put current task to the end of the queue
                _ptask->status = pe::co::task_status_paused;
                // Update the next cpu time
                _ptask->next_time = std::chrono::high_resolution_clock::now();
                __this_loop->insert_task(_ptask);

                // Force to switch to main context
                __this_loop->swap_to_main_loop();
            }

            // Yield if the condition is true
            bool yield_if( bool cond ) { 
                if ( cond ) yield(); 
                pe::co::task * _ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return cond;
                return cond & (_ptask->status != task_status_stopped);
            }

            // Yield if the condition is false
            bool yield_if_not( bool cond ) {
                if ( !cond ) yield();
                pe::co::task * _ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return cond;
                return cond & (_ptask->status != task_status_stopped);
            }

            // Hold current task, yield, but not put back
            // to the timed based cache
            bool holding() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return false;
                if ( _ptask->hold_sig[0] == -1 ) {
                    while ( yield_if( pipe(_ptask->hold_sig) < 0 ) );
                }
                if ( _ptask->status == task_status_stopped ) return false;
                if ( _ptask->related_fd != -1 ) return false;   // Already monitoring on something
                _ptask->related_fd = _ptask->hold_sig[0];
                do {
                    _ptask->status = pe::co::task_status::task_status_paused;
                    if ( 
                        !__this_loop->monitor_task(
                            _ptask->hold_sig[0], _ptask, 
                            event_type::event_read, std::chrono::seconds(1)
                        )
                    ) {
                        return false;
                    }
                    __this_loop->swap_to_main_loop();
                } while ( _ptask->signal == pe::co::no_signal );
                _ptask->related_fd = -1;
                if ( _ptask->status == task_status_stopped ) return false;
                if ( _ptask->signal == pe::co::bad_signal ) return false;
                char _f;
                ignore_result(read( _ptask->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }
            // Hold the task until timedout
            bool holding_until( pe::co::duration_t timedout ) {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return false;
                if ( _ptask->hold_sig[0] == -1 ) {
                    while ( yield_if( pipe(_ptask->hold_sig) < 0 ) );
                }
                if ( _ptask->status == task_status_stopped ) return false;
                if ( _ptask->related_fd != -1 ) return false;   // Already monitoring on something
                _ptask->status = pe::co::task_status::task_status_paused;
                if ( 
                    !__this_loop->monitor_task(
                        _ptask->hold_sig[0], _ptask, 
                        event_type::event_read, timedout
                    )
                ) {
                    return false;
                }
                _ptask->related_fd = _ptask->hold_sig[0];
                __this_loop->swap_to_main_loop();
                _ptask->related_fd = -1;
                if ( _ptask->status == task_status_stopped ) return false;
                if ( _ptask->signal == pe::co::bad_signal ) return false;
                if ( _ptask->signal == pe::co::no_signal ) return false;
                char _f;
                ignore_result(read( _ptask->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }

            // Wait on event
            pe::co::waiting_signals wait_for_event(
                pe::co::event_type eventid, 
                pe::co::duration_t timedout
            ) {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return pe::co::bad_signal;
                if ( _ptask->status == task_status_stopped ) return bad_signal;
                if ( _ptask->related_fd != -1 ) return bad_signal;
                _ptask->status = pe::co::task_status::task_status_paused;
                if ( 
                    !__this_loop->monitor_task( 
                        (int)_ptask->id, _ptask, eventid, timedout 
                    ) 
                ) {
                    return pe::co::bad_signal;
                }
                _ptask->related_fd = (long)_ptask->id;
                __this_loop->swap_to_main_loop();
                _ptask->related_fd = -1;
                if ( _ptask->status == task_status_stopped ) return bad_signal;
                return _ptask->signal;
            }

            // Wait on event if match the condition
            bool wait_for_event_if(
                bool cond,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_for_event( eventid, timedout );
                return ( _signal != pe::co::bad_signal );
            }

            // Wait for other fd's event
            pe::co::waiting_signals wait_fd_for_event(
                long fd,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            ) {
                if ( fd == -1 ) return pe::co::bad_signal;
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return pe::co::bad_signal;
                if ( _ptask->status == task_status_stopped ) return bad_signal;
                _ptask->status = pe::co::task_status::task_status_paused;
                _ptask->related_fd = fd;
                if ( 
                    !__this_loop->monitor_task( 
                        fd, _ptask, eventid, timedout 
                    ) 
                ) {
                    _ptask->related_fd = -1;
                    return pe::co::bad_signal;
                }
                __this_loop->swap_to_main_loop();
                _ptask->related_fd = -1;
                if ( _ptask->status == task_status_stopped ) return bad_signal;
                return _ptask->signal;
            }
            bool wait_fd_for_event_if(
                bool cond,
                long fd,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_fd_for_event(fd, eventid, timedout);
                return ( _signal != pe::co::bad_signal );
            }
            // Wait For other task's event
            pe::co::waiting_signals wait_other_task_for_event(
                task* ptask,
                pe::co::event_type eventid,
                pe::co::duration_t timedout                
            ) {
                if ( ptask == NULL || ptask->is_event_id == false ) return pe::co::bad_signal;
                return wait_fd_for_event((long)ptask->id, eventid, timedout);
            }
            bool wait_other_task_for_event_if(
                bool cond,
                task* ptask,
                pe::co::event_type eventid,
                pe::co::duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_other_task_for_event(ptask, eventid, timedout);
                return ( _signal != pe::co::bad_signal );
            }

            // Sleep
            void sleep( pe::co::duration_t duration ) {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return;
                if ( _ptask->status == task_status_stopped ) return;

                // Put current task to the end of the queue
                _ptask->status = pe::co::task_status_paused;
                // Update the next cpu time
                _ptask->next_time = std::chrono::high_resolution_clock::now() + duration;
                __this_loop->insert_task(_ptask);

                // Force to switch to main context
                __this_loop->swap_to_main_loop();
            }
            // Tick time
            void begin_tick() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return;
                _ptask->lt_time = task_time_now();
            }
            double tick() {
                pe::co::task *_ptask = __this_loop->get_running_task();
                if ( _ptask == NULL ) return -1.f;
                auto _n = task_time_now();
                std::chrono::duration< double, std::milli > _t = _n - _ptask->lt_time;
                _ptask->lt_time = _n;
                return _t.count();
            }
            // Dump debug info to given FILE
            void debug_info( FILE * fp ) {
                pe::co::task *_ptask = __this_loop->get_running_task();
                task_debug_info(_ptask, fp, 0);
            }
        }

        // Other task wrapper
        other_task::other_task( task * t ) : _other(t) { }
        
        // Tell the holding task to go on
        void other_task::go_on() const { task_go_on(_other); }
        // Tell the holding task to wake up and quit
        void other_task::stop() const { task_stop(_other); }
        // Tell the other task to exit on next status checking
        void other_task::exit() const { task_exit(_other); }

        // Other task keeper
        other_task_keeper::other_task_keeper( const other_task& t )
            : ot_(t._other), ok_flag_(false) { }
        other_task_keeper::other_task_keeper( task* t ) 
            : ot_(t), ok_flag_(false) { }
        other_task_keeper::~other_task_keeper() {
            if ( ok_flag_ ) { task_go_on(ot_); }
            else { task_stop(ot_); }
        }
        void other_task_keeper::job_done() { ok_flag_ = true; }
        
        // task guard
        task_guard::task_guard( task* t ) : task_(t) { }
        task_guard::~task_guard() {
            if ( task_ != NULL ) task_exit(task_);
        }

        // task keeper
        task_keeper::task_keeper( task* t ) : task_(t) { }
        task_keeper::~task_keeper() {
            if ( task_ != NULL ) task_stop(task_);
        }

        // task pauser
        task_pauser::task_pauser( task* t ) : task_(t) { }
        task_pauser::~task_pauser() {
            if ( task_ != NULL ) task_go_on(task_);
        }

        namespace parent_task {
            
            // Get the parent task
            task * get_task() {
                task * _ttask = __this_loop->get_running_task();
                if ( _ttask == NULL ) return NULL;
                return _ttask->p_task;
            }

            // Go on
            void go_on() {
                task * _ptask = parent_task::get_task();
                task_go_on(_ptask);
            }

            // Tell the holding task to stop
            void stop() {
                task * _ptask = parent_task::get_task();
                task_stop(_ptask);
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
                task *_ptask = parent_task::get_task();
                if ( _ptask == NULL ) return;
                task_exit(_ptask);
                this_task::get_task()->p_task = NULL;
            }
        }
    }
}

#ifdef __APPLE__
#pragma clang diagnostic pop
#endif

// Push Chen

