/*
    loop.cpp
    PECoTask
    2019-05-07
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

// For old libpeco version, forct to use ucontext
#if (LIBPECO_STANDARD_VERSION < 0x00020001)
#define PECO_USE_UCONTEXT   1
// When enabled ucontext and we are using gnu lib, can use ucontext
#elif defined(ENABLE_UCONTEXT) && (ENABLE_UCONTEXT == 1)
#define PECO_USE_UCONTEXT   1
#endif

#ifndef PECO_USE_UCONTEXT
#if defined(DISABLE_THREAD_STACK) && (DISABLE_THREAD_STACK == 1)
#define PECO_USE_SIGUSR1    1
#endif
#endif

#ifdef PECO_USE_UCONTEXT
#ifdef __APPLE__
#define _XOPEN_SOURCE
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <sys/ucontext.h>
#endif
#include <ucontext.h>
#endif

#include <peco/cotask/loop.h>
#include <peco/cotask/deftask.h>

#include <signal.h>
#ifdef __APPLE__
#include <peco/cotask/kevent_wrapper.h>
#else
#include <peco/cotask/epoll_wrapper.h>
#endif

#ifndef TASK_STACK_SIZE
// Because some lib we use, we may need at least 1mb stack
#define TASK_STACK_SIZE     1048576      // 1MB
#endif

#ifndef STACK_RESERVED_SIZE
#define STACK_RESERVED_SIZE            16384    // 16KB
#endif

#include <peco/peutils.h>
#include <execinfo.h>

#ifndef PECO_USE_UCONTEXT
#include <setjmp.h>
#endif

extern "C" {
    int PECoTask_Autoconf() { return 0; }
}
namespace pe {
    namespace co {
        // The main loop
        __thread_attr loop loop::main;
        #ifdef PECO_USE_UCONTEXT
        __thread_attr ucontext_t    __main_context;
        #else
        __thread_attr jmp_buf       __main_context;
        #endif
        #define R_MAIN_CTX          (__main_context)
        #define P_MAIN_CTX          (&__main_context)

        // Internal Core FD and Events Cache
        __thread_attr int 					    __core_fd = -1;
        __thread_attr core_event_t *              __core_events = NULL;

        __thread_attr task *      __running_task = NULL;
        __thread_attr task *      __timed_task_root = NULL;
        __thread_attr task *      __read_task_root = NULL;
        __thread_attr task *      __write_task_root = NULL;
        __thread_attr task *      __timedout_task_root = NULL;
        __thread_attr uint32_t    __timed_task_count = 0;
        __thread_attr uint32_t    __read_task_count = 0;
        __thread_attr uint32_t    __write_task_count = 0;
        __thread_attr uint32_t    __timedout_task_count = 0;
        __thread_attr uint32_t    __all_task_count = 0;

        ON_DEBUG(
            bool g_cotask_trace_flag = false;
            void enable_cotask_trace() {
                g_cotask_trace_flag = true;
            }
            bool is_cotask_trace_enabled() {
                return g_cotask_trace_flag;
            }
        )

        // Dump Task Info
        void __task_debug_info( task * task, FILE* fp = stdout, int lv = 0 );

        typedef void (*context_job_t)(void);
		// Run job wrapper
#ifdef PECO_USE_UCONTEXT
        void __job_run( task_job_t* job ) { 
#else
    #ifdef PECO_USE_SIGUSR1
        void __job_run( int sig ) {
            job = __running_task->pjob;
            if ( 0 == setjmp(__running_task->ctx) ) return;
    #else
        void *__job_run( void* ptask ) {
            job = (task *)ptask->pjob;
            if ( 0 == setjmp(((task *)ptask)->ctx) ) return (void *)0;
    #endif
#endif
            try {
                (*job)(); 
            } catch( const char * msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
            } catch( std::string& msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
            } catch( std::stringstream& msg ) {
                std::cerr << "exception: " << msg.str() << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
            } catch ( std::logic_error& ex ) {
                std::cerr << "logic error: " << ex.what() << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
            } catch ( std::runtime_error& ex ) {
                std::cerr << "runtime error: " << ex.what() << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
            } catch ( std::bad_exception& ex ) {
                std::cerr << "bad exception: " << ex.what() << ", task info: " << std::endl;
                __task_debug_info( __running_task, stderr );
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
                __task_debug_info( __running_task, stderr );
            }

        #ifndef PECO_USE_UCONTEXT
            // Jmp Back to the main context
            longjmp(R_MAIN_CTX);
          #ifndef PECO_USE_SIGUSR1
            return (void *)0;
          #endif
        #endif
        }

        task* __inner_create_task( task_job_t job ) {

            // The task should be put on the heap
            task * _ptask = (task *)malloc(sizeof(task));
            _ptask->id = (intptr_t)(_ptask);
            _ptask->exitjob = NULL;
            _ptask->refcount = 0;

            // Init the stack for the task
            // Get a 1MB Memory piece
            _ptask->stack = (stack_ptr)malloc(TASK_STACK_SIZE);

            // Save the job on heap
            _ptask->pjob = new task_job_t(job);

        #ifdef PECO_USE_UCONTEXT
            // Init the context
            getcontext(&_ptask->ctx);
            _ptask->ctx.uc_stack.ss_sp = _ptask->stack;
            _ptask->ctx.uc_stack.ss_size = TASK_STACK_SIZE - STACK_RESERVED_SIZE;
            _ptask->ctx.uc_stack.ss_flags = 0;
            _ptask->ctx.uc_link = P_MAIN_CTX;

            makecontext(&_ptask->ctx, (context_job_t)&__job_run, 1, _ptask->pjob);
        #else

            #ifdef PECO_USE_SIGUSR1

            _ptask->task_stack.ss_flags = 0;
            _ptask->task_stack.ss_size = TASK_STACK_SIZE - STACK_RESERVED_SIZE;
            _ptask->task_stack.ss_sp = _ptask->stack;

            #else

            pthread_attr_init(&_ptask->task_attr);
            pthread_attr_setstack(&_ptask->task_attr, _ptask->stack, TASK_STACK_SIZE - STACK_RESERVED_SIZE);

            #endif

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
            _ptask->p_task = __running_task;
            _ptask->user_flag_mask0 = 0;
            _ptask->user_flag_mask1 = 0;

            // Set the parent task
            _ptask->p_task = __running_task;
            // I have no child task
            _ptask->c_task = NULL;
            // I have no newer sibling node
            _ptask->prev_task = NULL;
            // Init my next sibline node to NULL
            _ptask->next_task = NULL;

            _ptask->next_timed_task = NULL;
            _ptask->next_read_task = NULL;
            _ptask->next_write_task = NULL;
            _ptask->next_timedout_task = NULL;

            // If im not the root task
            if ( _ptask->p_task != NULL ) {
                // My next task is the last child task of my parent
                _ptask->next_task = _ptask->p_task->c_task;
                // If im not the oldest
                if ( _ptask->next_task != NULL ) _ptask->next_task->prev_task = _ptask;
                // My Parent's newest task is me
                _ptask->p_task->c_task = _ptask;
            }

            __all_task_count += 1;
            return _ptask;
        }

        void __reset_task( task* ptask ) {
        #ifdef PECO_USE_UCONTEXT
            getcontext(&ptask->ctx);
            ptask->ctx.uc_stack.ss_sp = ptask->stack;
            ptask->ctx.uc_stack.ss_size = TASK_STACK_SIZE - STACK_RESERVED_SIZE;
            ptask->ctx.uc_stack.ss_flags = 0;
            ptask->ctx.uc_link = P_MAIN_CTX;

            // Save the job on heap
            makecontext(&ptask->ctx, (context_job_t)&__job_run, 1, ptask->pjob);
        #else
            #ifdef PECO_USE_SIGUSR1
            ptask->task_stack.ss_sp = ptask->stack;
            ptask->task_stack.s_flags = 0;
            #endif

            // Make this task looks like a new one
            ptask->status = task_status_pending;

        #endif
        }

        task* __create_task( task_job_t job ) {
            task * _ptask = __inner_create_task(job);
            ON_DEBUG_COTASK(
                std::cout << "created a task: " << std::endl;
                __task_debug_info(_ptask);
            )

            return _ptask;
        }

        task* __create_task_with_fd( int fd, task_job_t job ) {
            task * _ptask = __create_task( job );
            _ptask->id = (intptr_t)fd;
            _ptask->is_event_id = true;

            ON_DEBUG_COTASK(
                std::cout << "created a task: " << std::endl;
                __task_debug_info(_ptask);
            )

            return _ptask;
        }

        void __destory_task( task * ptask ) {
            if ( ptask->refcount > 0 ) return;
            assert(ptask->refcount >= 0);
            ON_DEBUG_COTASK(
                std::cout << "destroy a task: " << std::endl;
                __task_debug_info(ptask);
            )
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
            free(ptask->stack);

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
            // If I have a parent
            if ( ptask->p_task != NULL ) {
                // If im my parent's first child
                if ( ptask->p_task->c_task == ptask ) { 
                    ptask->p_task->c_task = ptask->next_task;
                }
            }

            #if !defined(PECO_USE_UCONTEXT) && !defined(PECO_USE_SIGUSR1)
            pthread_attr_destroy(&ptask->task_attr);
            #endif

            free(ptask);
            __all_task_count -= 1;
        }


#define __INSERT_TASK( __KEY__, __ROOT__, __ATTR__ )                \
        void __insert_##__KEY__##_task( task * ptask ) {            \
            if ( ptask == NULL ) return;                            \
            do {                                                    \
            ptask->next_##__KEY__##_task = NULL;                    \
            if ( __ROOT__ == NULL ) { __ROOT__ = ptask;             \
                __##__KEY__##_task_count += 1; break; }             \
            task *_prev_task = NULL, *_cur_task = __ROOT__;         \
            while ( _cur_task != NULL ) {                           \
                if ( _cur_task->__ATTR__ > ptask->__ATTR__) break;  \
                _prev_task = _cur_task;                             \
                _cur_task = _cur_task->next_##__KEY__##_task; }     \
            ptask->next_##__KEY__##_task = _cur_task;               \
            if ( _prev_task == NULL ) { __ROOT__ = ptask; }         \
            else { _prev_task->next_##__KEY__##_task = ptask; }     \
            __##__KEY__##_task_count += 1;                          \
            } while ( false );                                      \
            ++ptask->refcount;                                      \
            ON_DEBUG_COTASK(                                        \
                std::cout << "after insert "#__KEY__" task: " <<    \
                    ptask->id << ", list contains: " <<             \
                    __##__KEY__##_task_count << ", now order is: "; \
                task *_t = __ROOT__; while( _t != NULL ) {          \
                    std::cout << _t->id;                            \
                    if ( _t->next_##__KEY__##_task != NULL )        \
                        std::cout << ", ";                          \
                    _t = _t->next_##__KEY__##_task; }               \
                std::cout << std::endl;)                            \
        }
#define __REMOVE_TASK( __KEY__, __ROOT__ )                          \
        void __remove_##__KEY__##_task( task* ptask ) {             \
            if ( ptask == NULL ) return;                            \
            do {                                                    \
            task *_cur_task = __ROOT__, *_prev_task = NULL;         \
            if ( _cur_task == NULL ) break;                         \
            do {                                                    \
                if ( _cur_task == ptask ) {                         \
                    if ( _prev_task == NULL ) {                     \
                        __ROOT__ = _cur_task->next_##__KEY__##_task;\
                    } else {                                        \
                        _prev_task->next_##__KEY__##_task =         \
                            _cur_task->next_##__KEY__##_task; }     \
                    --ptask->refcount;                              \
                    __##__KEY__##_task_count -= 1;                  \
                    break; }                                        \
                _prev_task = _cur_task;                             \
                _cur_task = _cur_task->next_##__KEY__##_task; }     \
            while( _cur_task != NULL );                             \
            ptask->next_##__KEY__##_task = NULL;                    \
            } while ( false );                                      \
            ON_DEBUG_COTASK(                                        \
                std::cout << "after remove "#__KEY__" task: " <<    \
                    ptask->id << ", list contains: " <<             \
                    __##__KEY__##_task_count << ", now order is: "; \
                task *_t = __ROOT__; while( _t != NULL ) {          \
                    std::cout << _t->id;                            \
                    if ( _t->next_##__KEY__##_task != NULL )        \
                        std::cout << ", ";                          \
                    _t = _t->next_##__KEY__##_task; }               \
                std::cout << std::endl;)                            \
        }

        /*
            According to the next_key_task logic, one task may appeared
            in two or more different task list, simply destroy each
            task in the list will cause a double-free error
            Fix: Add Reference Count!
        */
#define __CLEAR_TASK_LIST( __KEY__, __ROOT__ )                      \
        void __clear_##__KEY__##_task_list() {                      \
            while ( __ROOT__ != NULL ) {                            \
                task *_ptask = __ROOT__;                            \
                __ROOT__ = _ptask->next_##__KEY__##_task;           \
                __destory_task(_ptask); }                           \
        }

        __INSERT_TASK(timed, __timed_task_root, wait_until)
        __REMOVE_TASK(timed, __timed_task_root)
        __CLEAR_TASK_LIST(timed, __timed_task_root)
        __INSERT_TASK(read, __read_task_root, read_until)
        __REMOVE_TASK(read, __read_task_root)
        __CLEAR_TASK_LIST(read, __read_task_root)
        __INSERT_TASK(write, __write_task_root, write_until)
        __REMOVE_TASK(write, __write_task_root)
        __CLEAR_TASK_LIST(write, __write_task_root)
        __INSERT_TASK(timedout, __timedout_task_root, wait_until)
        __REMOVE_TASK(timedout, __timedout_task_root)

        task * __search_read_task_by_fd( long fd ) {
            task * _task = __read_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return _task;
                _task = _task->next_read_task;
            }
            return NULL;
        }
        task * __search_write_task_by_fd( long fd ) {
            task * _task = __write_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return _task;
                _task = _task->next_write_task;
            }
            return NULL;
        }
        task * __search_task_by_fd( task * root, long fd ) {
            if ( root == __read_task_root ) return __search_read_task_by_fd(fd);
            if ( root == __write_task_root ) return __search_write_task_by_fd(fd);
            return NULL;
        }

        // Internal task switcher
        void __switch_task( task* ptask ) {
            #ifndef PECO_USE_UCONTEXT
            // Save the old status
            auto _tstatus = ptask->status;
            #endif

            if ( ptask->status != task_status_stopped ) {
                ptask->status = task_status_running;
            }
            __running_task = ptask;
            // If the task finishs and quit, then will continue
            // the main loop because we set uc_link as the main ctx
            // when we create the task
            // otherwise, the yield function will change the
            // wait_until of the task and insert it back to
            // the task cache.
            ON_DEBUG_COTASK(
                std::cout << CLI_BLUE << "X main => " << ptask->id << CLI_NOR << std::endl;
            )
        #ifdef PECO_USE_UCONTEXT

            swapcontext(P_MAIN_CTX, &ptask->ctx);

        #else
            if ( _tstatus == task_status_pending ) {
            #ifdef PECO_USE_SIGUSR1
                // Raise the signal to switch to the stack
                // And the handler will call setjmp and return immediately
                sigaltstack(&ptask->task_stack, NULL);

                raise(SIGUSR1);

                // Disable the altstack
                ptask->task_stack.ss_flags = SS_DISABLE;
                sigaltstack(&(ptask->task_stack), NULL);
            #else
                // Create a thread, and it will return immediately
                pthread_t _t;
                ignore_result( pthread_create(&_t, &(ptask->task_attr), &__job_run, (void *)ptask) );
                void *_st;
                pthread_join(_t, &_st);

            #endif
            }

            if ( !setjmp(R_MAIN_CTX) ) {
                longjmp(ptask->ctx, 1);
            }
        #endif
            // If the task's status is running, means the last task just
            // finished and quit, then check if the task is a oneshot task
            // if not, then decrease the repeat count and put back to
            // the task cache
            __running_task = NULL;
            // last task has been finished, then check if it is loop
            if ( ptask->status == task_status::task_status_running ) {
                if ( ptask->repeat_count == 1 ) {
                    // Clear the task and release the memory
                    __destory_task(ptask);
                    return;
                }
                if ( ptask->repeat_count != infinitive_task ) {
                    --ptask->repeat_count;
                }
                ptask->wait_until += ptask->duration;
                __reset_task(ptask);
                __insert_timed_task(ptask);
            } else if ( ptask->status == task_status::task_status_stopped ) {
                // The task has been stopped by someone, quit it
                __destory_task(ptask);
            }
            // else, the task is paused, do nothing
        }

		// Yield current task and switch to main loop
		void __swap_to_main_loop() {
            ON_DEBUG_COTASK(
                std::cout << "X " << __running_task->id << " => main" << std::endl;
            )
        #ifdef PECO_USE_UCONTEXT
            swapcontext(&__running_task->ctx, P_MAIN_CTX);
            // setcontext(&main_ctx_);
        #else
            if ( !setjmp(__running_task->ctx) ) {
                longjmp(R_MAIN_CTX, 1);
            }
        #endif
        }

        // Event Monitor
        bool __monitor_task( int fd, task * ptask, event_type eventid, duration_t timedout ) {
            // Add to the core_events
            int _c_eid = (eventid == event_type::event_read ? core_event_read : core_event_write);
            if ( eventid == event_type::event_read ) {
                if ( __search_task_by_fd( __read_task_root, fd ) != NULL ) return false;
#ifndef __APPLE__
                if ( __search_task_by_fd( __write_task_root, fd ) != NULL ) {
                    _c_eid |= core_event_write;
                }
#endif
            } else {
                if ( __search_task_by_fd( __write_task_root, fd ) != NULL ) return false;
#ifndef __APPLE__
                if ( __search_task_by_fd( __read_task_root, fd ) != NULL ) {
                    _c_eid |= core_event_read;
                }
#endif
            }
            core_event_t _e;
            core_event_init(_e);
            core_event_prepare(_e, fd);
            core_make_flag(_e, core_flag_add);
            core_mask_flag(_e, core_flag_oneshot);
            if ( -1 == core_event_ctl_with_flag(__core_fd, fd, _e, _c_eid) ) 
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
                __insert_read_task(ptask);
            } else {
                ptask->write_until = task_time_now() + timedout;
                ON_DEBUG_COTASK(
                    std::cout << "* MW <" << fd << ":" << ptask->id 
                        << "> for write event, timedout at "
                        << ptask->write_until.time_since_epoch().count() << std::endl;
                )
                __insert_write_task(ptask);
            }
            return true;
        }

        // Cancel all monitored item of a task
        void __cancel_monitor( task * ptask ) {
            if ( ptask == NULL ) return;

            ON_DEBUG_COTASK(
                std::cout << "a task will be cancelled: " << ptask->id << std::endl;
            )
            if ( ptask->related_fd != -1 ) {
                task *_rtask = __search_task_by_fd(__read_task_root, ptask->related_fd);
                if ( _rtask == ptask ) {
                    ON_DEBUG_COTASK(
                        std::cout << "the task is waiting on " << ptask->related_fd 
                            << " for incoming, now remove it." << std::endl;
                    )
                    __remove_read_task(ptask);
                    ptask->read_until = task_time_now();
                    __insert_read_task(ptask);
                }
                task *_wtask = __search_task_by_fd(__write_task_root, ptask->related_fd);
                if ( _wtask == ptask ) {
                    ON_DEBUG_COTASK(
                        std::cout << "the task is waiting on " << ptask->related_fd
                            << " for write buffer, now remove it." << std::endl;
                    )
                    __remove_write_task(ptask);
                    ptask->write_until = task_time_now();
                    __insert_write_task(ptask);
                }
            }
        }

        // Cancel all monitored task of specified fd
        void __cancel_fd( long fd ) {
            task *_rtask = __search_task_by_fd(__read_task_root, fd);
            if ( _rtask != NULL ) {
                __remove_read_task(_rtask);
                _rtask->read_until = task_time_now();
                __insert_read_task(_rtask);
            }
            task *_wtask = __search_task_by_fd(__write_task_root, fd);
            if ( _wtask != NULL ) {
                __remove_write_task(_wtask);
                _rtask->write_until = task_time_now();
                __insert_write_task(_wtask);
            }
        }

        // Timed task schedule
        void __timed_task_schedule() {
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __timed_task_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )
            while ( __timed_task_root != NULL && __timed_task_root->wait_until <= _now ) {
                task *_task = __timed_task_root;
                __remove_timed_task(_task);
                __switch_task(_task);
            }
        }
        // Event Task schedule
        void __event_task_schedule() {
            if ( 
                __timed_task_root == NULL && 
                __read_task_root == NULL && 
                __write_task_root == NULL 
            ) return;

            uint32_t _wait_time = 1000; // default 1000ms
            auto _now = task_time_now();
            auto _wait_to = _now + std::chrono::milliseconds(_wait_time);
            if ( __timed_task_root != NULL ) {
                if ( __timed_task_root->wait_until < _wait_to ) {
                    _wait_to = __timed_task_root->wait_until;
                }
            }
            if ( __read_task_root != NULL ) {
                if ( __read_task_root->read_until < _wait_to ) {
                    _wait_to = __read_task_root->read_until;
                }
            }
            if ( __write_task_root != NULL ) {
                if ( __write_task_root->write_until < _wait_to ) {
                    _wait_to = __write_task_root->write_until;
                }
            }
            if ( _wait_to > _now ) {
                auto _ts = _wait_to - _now;
                auto _ms = std::chrono::duration_cast< std::chrono::milliseconds >(_ts);
                _wait_time = (uint32_t)_ms.count();
            } else {
                _wait_time = 0;
            }

            int _count = core_event_wait(
                __core_fd, __core_events, CO_MAX_SO_EVENTS, _wait_time
            );
            ON_DEBUG_COTASK(
                std::cout << CLI_BLUE << "∑ E: " << _count << ", RC: " << __read_task_count
                    << ", WC: " << __write_task_count << CLI_NOR << std::endl;
            )
            if ( _count == -1 ) return;
            // If have any event, then do something
            // If not, then continue the loop
            for ( int i = 0; i < _count; ++i ) {
                core_event_t *_pe = __core_events + i;
                int _fd = (intptr_t)core_get_so(_pe);

                do {
                    // We only get outgoing event, do nothing in
                    // read event block
                    if ( core_is_outgoing(_pe) ) break;

                    // Find the task in the read cache
                    task *_rt = __search_task_by_fd(__read_task_root, _fd);
                    if ( _rt == NULL ) break;

                    if ( core_is_error_event(_pe) ) {
                        _rt->signal = waiting_signals::bad_signal;
                    } else {
                        _rt->signal = waiting_signals::receive_signal;
                    }
                    __remove_read_task(_rt);
                    __switch_task(_rt);
                } while ( false );

                do {
                    // We only get incoming event, do nothing in 
                    // write event block
                    if ( core_is_incoming(_pe) ) break;

                    // Find the task in the write cache
                    task *_wt = __search_task_by_fd(__write_task_root, _fd);
                    if ( _wt == NULL ) break;

                    if ( core_is_error_event(_pe) ) {
                        _wt->signal = waiting_signals::bad_signal;
                    } else {
                        _wt->signal = waiting_signals::receive_signal;
                    }
                    __remove_write_task(_wt);
                    __switch_task(_wt);
                } while ( false );

            #ifndef __APPLE__
                // For EPOLL, need to re-add the event.
                if ( __search_task_by_fd(__read_task_root, _fd) ) {
                    // Read not timedout, do it in the next runloop
                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, _fd);
                    core_make_flag(_e, core_flag_add);
                    core_mask_flag(_e, core_flag_oneshot);
                    core_event_ctl_with_flag(__core_fd, _fd, _e, core_event_read);
                }
                if ( __search_task_by_fd(__write_task_root, _fd) ) {
                    // Write not timedout
                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, _fd);
                    core_make_flag(_e, core_flag_add);
                    core_mask_flag(_e, core_flag_oneshot);
                    core_event_ctl_with_flag(__core_fd, _fd, _e, core_event_write);
                }
            #endif
            }
        }
        // Event Task Timedout check
        void __event_task_timedout_check() {
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __event_task_timedout_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )

            while ( __read_task_root != NULL ) {
                task *_task = __read_task_root;
                if ( _task->read_until > _now ) break;

                // Remove read event
                core_event_t _e;
                core_event_init(_e);
                core_event_prepare(_e, (int)_task->related_fd);
                core_event_del(__core_fd, _e, (int)_task->related_fd, core_event_read);

                _task->signal = waiting_signals::no_signal;
                __remove_read_task(_task);
                __insert_timedout_task(_task);
            }

            while ( __write_task_root != NULL ) {
                task *_task = __write_task_root;
                if ( _task->write_until > _now ) break;

                core_event_t _e;
                core_event_init(_e);
                core_event_prepare(_e, (int)_task->related_fd);
                core_event_del(__core_fd, _e, (int)_task->related_fd, core_event_write);

                _task->signal = waiting_signals::no_signal;
                __remove_write_task(_task);
                __insert_timedout_task(_task);
            }

            while ( __timedout_task_root != NULL ) {
                task *_t = __timedout_task_root;
                __remove_timedout_task(_t);
                __switch_task(_t);
            }
        }

        // Default C'str
        loop::loop() : running_(false), ret_code_(0) 
        { }
        // Default D'str
        loop::~loop() {
            if ( __core_fd == -1 ) return;
            if ( __running_task != NULL ) {
                __destory_task(__running_task);
                __running_task = NULL;
            }
            __clear_timed_task_list();
            __clear_read_task_list();
            __clear_write_task_list();
            ::close(__core_fd);
            free(__core_events);
        }

        // Get task count
        uint64_t loop::task_count() const { return __all_task_count; }
        // Get the return code
        int loop::return_code() const { return ret_code_; }

        task_t loop::do_job( task_job_t job ) {
            task *_ptask = __create_task(job);
            _ptask->repeat_count = oneshot_task;
            _ptask->wait_until = std::chrono::high_resolution_clock::now();
            __insert_timed_task(_ptask);

            // Switch to main and run the task
            this_task::yield();
            return (task_t)_ptask;
        }

        task_t loop::do_job( long fd, task_job_t job ) {
            if ( fd == -1 ) return NULL;
            task *_ptask = __create_task_with_fd( fd, job );
            _ptask->repeat_count = oneshot_task;
            _ptask->wait_until = std::chrono::high_resolution_clock::now();
            __insert_timed_task(_ptask);

            // Switch
            this_task::yield();
            return (task_t)_ptask;
        }

        task_t loop::do_loop( task_job_t job, duration_t interval ) {
            task *_ptask = __create_task(job);
            _ptask->repeat_count = infinitive_task;
            _ptask->wait_until = std::chrono::high_resolution_clock::now();
            _ptask->duration = interval;
            __insert_timed_task(_ptask);

            // Switch
            this_task::yield();
            return (task_t)_ptask;
        }

        task_t loop::do_delay( task_job_t job, duration_t delay ) {
            task *_ptask = __create_task(job);
            _ptask->repeat_count = oneshot_task;
            _ptask->wait_until = std::chrono::high_resolution_clock::now() + delay;
            __insert_timed_task(_ptask);

            // Switch
            this_task::yield();
            return (task_t)_ptask;
        }

        // The entry point of the main loop
        void loop::run() {
            // Init the core_fd and core_events
            if ( __core_fd != -1 ) return;

            // Initalize to ignore some sig
            // SIGPIPE
            signal(SIGPIPE, SIG_IGN);

            #ifdef PECO_USE_SIGUSR1

            // register the signal to run job
            struct sigaction    sa;            
            sa.sa_handler = &__job_run;
            sa.sa_flags = SA_ONSTACK;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);

            #endif

            core_fd_create(__core_fd);
            __core_events = (core_event_t *)calloc(CO_MAX_SO_EVENTS, sizeof(core_event_t));
 
            running_ = true;
            while (
                __timed_task_root != NULL ||
                __read_task_root != NULL ||
                __write_task_root != NULL
            ) {
                if ( running_ == false ) break;
                // Schedule all time-based task first
                __timed_task_schedule();

                // Event based task timedout checking
                __event_task_timedout_check();

                // Event based task schedule
                __event_task_schedule();
            }

            // bread from the loop, clear everything
            if ( __running_task != NULL ) {
                __destory_task(__running_task);
            }
            __clear_timed_task_list();
            __clear_read_task_list();
            __clear_write_task_list();
            ::close(__core_fd);
            free(__core_events);
            __core_fd = -1;
        }
        // Tell if current loop is running
        bool loop::is_running() const { return (__core_fd != -1); }

        // Exit Current Loop
        void loop::exit( int code ) {
            ret_code_ = code; running_ = false; this_task::yield();
        }

        void __task_debug_info( task * ptask, FILE *fp, int lv ) {
            if ( ptask == NULL ) {
                fprintf(fp, "%*s|= <null>\n", lv * 4, ""); return;
            } else {
                fprintf(fp, "%*s>>%s<<\n", lv * 4, "", utils::ptr_str(ptask).c_str() );
            }

            fprintf(fp, "%*s|= id: %ld\n", lv * 4, "", ptask->id);
            fprintf(fp, "%*s|= ref: %d\n", lv * 4, "", ptask->refcount);
            fprintf(fp, "%*s|= is_event: %s\n", lv * 4, "", (ptask->is_event_id ? "true" : "false"));
            fprintf(fp, "%*s|= stack: %s\n", lv * 4, "", pe::utils::ptr_str(ptask->stack).c_str());
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
            fprintf(fp, "%*s|= flags0: %04x\n", lv * 4, "", ptask->user_flag_mask0);
            fprintf(fp, "%*s|= flags1: %04x\n", lv * 4, "", ptask->user_flag_mask1);
            fprintf(fp, "%*s|= first_child: %s\n", lv * 4, "", 
                utils::ptr_str(ptask->c_task).c_str());
            fprintf(fp, "%*s|= next_timed_task: %s\n", lv * 4, "", 
                utils::ptr_str(ptask->next_timed_task).c_str());
            fprintf(fp, "%*s|= next_read_task: %s\n", lv * 4, "", 
                utils::ptr_str(ptask->next_read_task).c_str());
            fprintf(fp, "%*s|= next_write_task: %s\n", lv * 4, "", 
                utils::ptr_str(ptask->next_write_task).c_str());
            fprintf(fp, "%*s|= next_timedout_task: %s\n", lv * 4, "", 
                utils::ptr_str(ptask->next_timedout_task).c_str());
            fprintf(fp, "%*s|= exitjob: %s\n", lv * 4, "", utils::ptr_str(ptask->exitjob).c_str());

            if ( ptask->p_task != NULL ) {
                fprintf(fp, "%*s|= parent: \n", lv * 4, "");
                __task_debug_info( ptask->p_task, fp, lv + 1 );
            } else {
                void * _stack[64];
                size_t _ssize = backtrace(_stack, 64);
                backtrace_symbols_fd(_stack, _ssize, fileno(fp));
            }
        }

        void task_debug_info( task_t ptask, FILE *fp, int lv ) {
            __task_debug_info( (task *)ptask, fp, lv );
        }

        // Cancel all bounded task of given fd
        void tasks_cancel_fd( long fd ) { __cancel_fd(fd); }        

        // Force to exit a task
        void task_exit( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL || _ptask == __running_task ) return;
            _ptask->status = task_status_stopped;
            __cancel_monitor(_ptask);
            __remove_timed_task(_ptask);
            _ptask->wait_until = task_time_now();
            _ptask->repeat_count = 1;
            // Only put back a task not holding anything
            if ( _ptask->related_fd == -1 ) {
                __insert_timed_task(_ptask);
            }
        }
        // Go on a task
        void task_go_on( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            while ( this_task::yield_if( _ptask->hold_sig[1] == -1 ) );
            ignore_result(write( _ptask->hold_sig[1], "1", 1 ));
        }

        // Stop a task's holding
        void task_stop( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            if ( _ptask->hold_sig[1] != -1 ) {
                ignore_result(write( _ptask->hold_sig[1], "0", 1 ));
            }
            __cancel_monitor(_ptask);
        }

        // Exit all child task
        void task_recurse_exit( task_t ptask ) {
            task *_ptask = (task *)ptask;
            task *_ctask = _ptask->c_task;
            while ( _ctask != NULL ) {
                task_recurse_exit((task_t)_ctask);
                _ctask = _ctask->next_task;
            }
            // Exit self
            task_exit(ptask);
        }

        // Get task's ID
        task_id task_get_id( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return 0;
            return _ptask->id;
        }

        // Set/Get Task Argument
        void task_set_arg( task_t ptask, void * arg ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            _ptask->arg = arg;
        }
        void * task_get_arg( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return NULL;
            return _ptask->arg;
        }

        // Set/Get Task User Flags
        void task_set_user_flag0( task_t ptask, uint32_t flag ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            _ptask->user_flag_mask0 = flag;
        }
        uint32_t task_get_user_flag0( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return 0u;
            return _ptask->user_flag_mask0;
        }
        void task_set_user_flag1( task_t ptask, uint32_t flag ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            _ptask->user_flag_mask1 = flag;
        }
        uint32_t task_get_user_flag1( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return 0u;
            return _ptask->user_flag_mask1;
        }

        // Get task's status
        task_status task_get_status( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return task_status_stopped;
            return _ptask->status;
        }

        // Get task's waiting_signal
        waiting_signals task_get_waiting_signal( task_t ptask ) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return bad_signal;
            return _ptask->signal;
        }
        
        // Register task's exit job
        void task_register_exit_job(task_t ptask, task_job_t job) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            if ( _ptask->exitjob ) {
                delete _ptask->exitjob;
            }
            _ptask->exitjob = new task_job_t(job);
        }

        // Remove task exit job
        void task_remove_exit_job(task_t ptask) {
            task *_ptask = (task *)ptask;
            if ( _ptask == NULL ) return;
            if ( _ptask->exitjob == NULL ) return;
            delete _ptask->exitjob;
            _ptask->exitjob = NULL;
        }
        
        namespace this_task {
            // Get the task point
            task_t get_task() { return (task_t)__running_task; }
            // Get the task id
            task_id get_id() {
                if ( __running_task == NULL ) return 0;
                return __running_task->id;
            }
            // Recurse Exit
            void recurse_exit() {
                task_recurse_exit( (task_t)__running_task );
            }

            // Set/Get Task Argument
            void set_arg( void * arg ) {
                if ( __running_task == NULL ) return;
                __running_task->arg = arg;
            }
            void * get_arg( ) {
                if ( __running_task == NULL ) return NULL;
                return __running_task->arg;
            }

            // Set/Get Task User Flags
            void set_user_flag0( uint32_t flag ) {
                if ( __running_task == NULL ) return;
                __running_task->user_flag_mask0 = flag;
            }
            uint32_t get_user_flag0( ) {
                if ( __running_task == NULL ) return 0u;
                return __running_task->user_flag_mask0;
            }
            void set_user_flag1( uint32_t flag ) {
                if ( __running_task == NULL ) return;
                __running_task->user_flag_mask1 = flag;
            }
            uint32_t get_user_flag1( ) {
                if ( __running_task == NULL ) return 0u;
                return __running_task->user_flag_mask1;
            }

            // Get task's status
            task_status get_status( ) {
                if ( __running_task == NULL ) return task_status_stopped;
                return __running_task->status;
            }

            // Get task's waiting_signal
            waiting_signals get_waiting_signal( ) {
                if ( __running_task == NULL ) return bad_signal;
                return __running_task->signal;
            }
            // Cancel loop inside
            void cancel_loop() {
                if ( __running_task == NULL ) return;
                __running_task->repeat_count = 1;
            }
            // Force to switch to main context
            void yield() {
                if ( __running_task == NULL ) return;

                // Will not switch task if it already been masked as stopped.
                if ( __running_task->status == task_status_stopped ) return;
                // Put current task to the end of the queue
                __running_task->status = task_status_paused;
                // Update the next cpu time
                __running_task->wait_until = task_time_now();
                __insert_timed_task(__running_task);

                // Force to switch to main context
                __swap_to_main_loop();
            }

            // Yield if the condition is true
            bool yield_if( bool cond ) { 
                if ( cond ) yield(); 
                if ( __running_task == NULL ) return cond;
                return cond & (__running_task->status != task_status_stopped);
            }

            // Yield if the condition is false
            bool yield_if_not( bool cond ) {
                if ( !cond ) yield();
                if ( __running_task == NULL ) return cond;
                return cond & (__running_task->status != task_status_stopped);
            }

            // Hold current task, yield, but not put back
            // to the timed based cache
            bool holding() {
                if ( __running_task == NULL ) return false;
                if ( __running_task->hold_sig[0] == -1 ) {
                    while ( yield_if( pipe(__running_task->hold_sig) < 0 ) );
                }
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->related_fd != -1 ) return false;   // Already monitoring on something
                __running_task->related_fd = __running_task->hold_sig[0];
                do {
                    __running_task->status = task_status_paused;
                    if ( 
                        !__monitor_task(
                            __running_task->hold_sig[0], __running_task, 
                            event_read, std::chrono::seconds(1)
                        )
                    ) {
                        return false;
                    }
                    __swap_to_main_loop();
                } while ( __running_task->signal == no_signal );
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->signal == bad_signal ) return false;
                char _f;
                ignore_result(read( __running_task->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }
            // Hold the task until timedout
            bool holding_until( duration_t timedout ) {
                if ( __running_task == NULL ) return false;
                if ( __running_task->hold_sig[0] == -1 ) {
                    while ( yield_if( pipe(__running_task->hold_sig) < 0 ) );
                }
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->related_fd != -1 ) return false;   // Already monitoring on something
                __running_task->status = task_status_paused;
                if ( 
                    !__monitor_task(
                        __running_task->hold_sig[0], __running_task, 
                        event_read, timedout
                    )
                ) {
                    return false;
                }
                __running_task->related_fd = __running_task->hold_sig[0];
                __swap_to_main_loop();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->signal == bad_signal ) return false;
                if ( __running_task->signal == no_signal ) return false;
                char _f;
                ignore_result(read( __running_task->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }

            // Wait on event
            waiting_signals wait_for_event(
                event_type eventid, 
                duration_t timedout
            ) {
                if ( __running_task == NULL ) return bad_signal;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                if ( __running_task->related_fd != -1 ) return bad_signal;
                __running_task->status = task_status_paused;
                if ( 
                    !__monitor_task( 
                        (int)__running_task->id, __running_task, eventid, timedout 
                    ) 
                ) {
                    return bad_signal;
                }
                __running_task->related_fd = (long)__running_task->id;
                __swap_to_main_loop();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                return __running_task->signal;
            }

            // Wait on event if match the condition
            bool wait_for_event_if(
                bool cond,
                event_type eventid,
                duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_for_event( eventid, timedout );
                return ( _signal != bad_signal );
            }

            // Wait for other fd's event
            waiting_signals wait_fd_for_event(
                long fd,
                event_type eventid,
                duration_t timedout
            ) {
                if ( fd == -1 ) return bad_signal;
                if ( __running_task == NULL ) return bad_signal;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                __running_task->status = task_status_paused;
                __running_task->related_fd = fd;
                if ( 
                    !__monitor_task( 
                        fd, __running_task, eventid, timedout 
                    ) 
                ) {
                    __running_task->related_fd = -1;
                    return bad_signal;
                }
                __swap_to_main_loop();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                return __running_task->signal;
            }
            bool wait_fd_for_event_if(
                bool cond,
                long fd,
                event_type eventid,
                duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_fd_for_event(fd, eventid, timedout);
                return ( _signal != bad_signal );
            }
            // Wait For other task's event
            waiting_signals wait_other_task_for_event(
                task_t ptask,
                event_type eventid,
                duration_t timedout                
            ) {
                task *_ptask = (task *)ptask;
                if ( _ptask == NULL || _ptask->is_event_id == false ) return bad_signal;
                return wait_fd_for_event((long)_ptask->id, eventid, timedout);
            }
            bool wait_other_task_for_event_if(
                bool cond,
                task_t ptask,
                event_type eventid,
                duration_t timedout
            ) {
                if ( ! cond ) return cond;
                auto _signal = wait_other_task_for_event(ptask, eventid, timedout);
                return ( _signal != bad_signal );
            }

            // Sleep
            void sleep( duration_t duration ) {
                if ( __running_task == NULL ) return;
                if ( __running_task->status == task_status_stopped ) return;

                // Put current task to the end of the queue
                __running_task->status = task_status_paused;
                // Update the next cpu time
                __running_task->wait_until = task_time_now() + duration;
                __insert_timed_task(__running_task);

                // Force to switch to main context
                __swap_to_main_loop();
            }
            // Tick time
            void begin_tick() {
                if ( __running_task == NULL ) return;
                __running_task->lt_time = task_time_now();
            }
            double tick() {
                if ( __running_task == NULL ) return -1.f;
                auto _n = task_time_now();
                std::chrono::duration< double, std::milli > _t = _n - __running_task->lt_time;
                __running_task->lt_time = _n;
                return _t.count();
            }
            // Dump debug info to given FILE
            void debug_info( FILE * fp ) {
                task_debug_info(__running_task, fp, 0);
            }
        }

        // task guard
        task_guard::task_guard( task_t t ) : task_(t) { }
        task_guard::~task_guard() {
            if ( task_ != NULL ) task_exit(task_);
        }

        // task keeper
        task_keeper::task_keeper( task_t t ) : task_(t) { }
        task_keeper::~task_keeper() {
            if ( task_ != NULL ) task_stop(task_);
        }

        // task pauser
        task_pauser::task_pauser( task_t t ) : task_(t) { }
        task_pauser::~task_pauser() {
            if ( task_ != NULL ) task_go_on(task_);
        }

        task_helper::task_helper( task_t t ) : task_(t) { }
        task_helper::~task_helper() {
            if ( task_ != NULL ) task_stop(task_);
        }
        void task_helper::job_done() {
            if ( task_ != NULL ) task_go_on(task_);
            task_ = NULL;
        }

        namespace parent_task {
            
            // Get the parent task
            task_t get_task() {
                task * _ttask = __running_task;
                if ( _ttask == NULL ) return NULL;
                return (task_t)_ttask->p_task;
            }

            // Go on
            void go_on() {
                task_t _ptask = parent_task::get_task();
                task_go_on(_ptask);
            }

            // Tell the holding task to stop
            void stop() {
                task_t _ptask = parent_task::get_task();
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
                task_t _ptask = parent_task::get_task();
                if ( _ptask == NULL ) return;
                task_exit(_ptask);
            }
        }
    }
}

#ifdef PECO_USE_UCONTEXT
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
#endif

// Push Chen

