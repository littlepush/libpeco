/*
    loop.cpp
    PECoTask
    2019-05-07
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/loop.h>

// For old libpeco version, forct to use ucontext
#if defined(__LIBPECO_USE_GNU__) && (LIBPECO_STANDARD_VERSION < 0x00020001)
#define FORCE_USE_UCONTEXT 1
// When enabled ucontext and we are using gnu lib, can use ucontext
#elif defined(__LIBPECO_USE_GNU__) && defined(ENABLE_UCONTEXT) && (ENABLE_UCONTEXT == 1)
#define FORCE_USE_UCONTEXT 1
#endif

#ifdef FORCE_USE_UCONTEXT
#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <thread>

#else

// We donot use ucontext
#if defined(USING_SIGUSR1_STACK) && USING_SIGUSR1_STACK == 1
#define PECO_SIGNAL_STACK 1
#endif

#endif

#ifdef FORCE_USE_UCONTEXT
#include <ucontext.h>
#else
#include <setjmp.h>
#endif

#include <signal.h>

#ifdef __APPLE__
#include <peco/cotask/kevent_wrapper.h>
#include <cxxabi.h>
#else
#include <peco/cotask/epoll_wrapper.h>
#endif

#include <peco/peutils.h>

// Only in gun
#ifdef __LIBPECO_USE_GNU__
#include <execinfo.h>
#endif

#ifndef TASK_STACK_SIZE
#define TASK_STACK_SIZE     524288      // 512KB
#endif


extern "C" {
    int PECoTask_Autoconf() { return 0; }
}
namespace pe {
    namespace co {

        #ifndef ATTR_VOLATILE
        #define ATTR_VOLATILE   volatile
        #endif

        struct full_task_t : public task {
            // Is event id
            bool                is_event_id;
            // The stack for the job
            ATTR_VOLATILE stack_ptr     stack;
            // The real running job's point
            task_job_t* ATTR_VOLATILE   pjob;

            // Status of the task
            task_status         status;
            // Status of the last monitoring event
            waiting_signals     signal;

            // Flags for holding
            int                 hold_sig[2];
            long                related_fd;

            // Next firing time of the task
            // for a oneshot job, is the creatation time
            task_time_t         next_time;
            task_time_t         read_until;
            task_time_t         write_until;
            duration_t          duration;
            // Repeat count of the task
            repeat_t            repeat_count;

            // Parent task
            full_task_t* ATTR_VOLATILE p_task;
            // Child task list head
            full_task_t*               c_task;
            // Task list
            full_task_t*               prev_task;
            full_task_t*               next_task;
            full_task_t* ATTR_VOLATILE next_action_task;

            // Tick
            task_time_t         lt_time;

            // On Task Destory
            task_job_t* ATTR_VOLATILE  exitjob;

        #ifdef FORCE_USE_UCONTEXT
            // Context for the job
            ucontext_t          ctx;
        #else

        #ifdef PECO_SIGNAL_STACK
            // We only need to save the stack info when we use SIGUSR1 to create new stack
            stack_t             task_stack;
        #else
            pthread_attr_t      task_attr;
        #endif
            jmp_buf             buf;
        #endif
        };

        // The main loop
        __thread_attr loop                  loop::main;

        // The thread local loop point
        __thread_attr loop *                            __this_loop = &loop::main;
        __thread_attr full_task_t * ATTR_VOLATILE       __running_task = NULL;
        // All event-based tasks
        __thread_attr full_task_t * ATTR_VOLATILE       __timed_task_root = NULL;
        __thread_attr full_task_t * ATTR_VOLATILE       __timed_task_tail = NULL;
        __thread_attr full_task_t * ATTR_VOLATILE       __read_task_root = NULL;
        __thread_attr full_task_t * ATTR_VOLATILE       __write_task_root = NULL;

        #ifdef FORCE_USE_UCONTEXT
        typedef ucontext_t                  context_type_t;
        #else
        typedef jmp_buf                     context_type_t;
        #endif
        __thread_attr context_type_t        __main_context;
        #define R_MAIN_CTX                  (__main_context)
        #define P_MAIN_CTX                  ((context_type_t *)&__main_context)

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
        void __dump_call_stack( FILE* fd, int lv = 0 ) {
        #if defined(PECO_SIGNAL_STACK) || defined(FORCE_USE_UCONTEXT)
            intptr_t _stack[64];
            size_t _ssize = backtrace((void **)_stack, 64);
            #ifdef __APPLE__
            char **_stackstrs = backtrace_symbols((void **)_stack, _ssize);
            char* _symbol = (char *)malloc(sizeof(char) * 1024);
            char* _module = (char *)malloc(sizeof(char) * 1024);
            int _offset = 0;
            char* _addr = (char *)malloc(sizeof(char) * 48);
            for ( size_t i = 2; i < _ssize; ++i ) {
                sscanf(_stackstrs[i], "%*s %s %s %s %*s %d", 
                    _module, _addr, _symbol, &_offset);
                int _valid_cxx_name = 0;
                char * _func = abi::__cxa_demangle(_symbol, NULL, 0, &_valid_cxx_name);
                fprintf(fd, "%*s(%s) 0x%s - %s + %d\n", lv * 4, "",
                    _module, _addr, _func, _offset);
                if ( _func ) free(_func);
            }
            free(_addr);
            free(_module);
            free(_symbol);
            #else
            backtrace_symbols_fd((void **)_stack, _ssize, fileno(fd));
            #endif
        #endif
        }

        typedef void (*context_job_t)(void);
		// Run job wrapper
        #ifdef FORCE_USE_UCONTEXT
        inline void __job_run( void )
        #else
        #ifdef PECO_SIGNAL_STACK
        inline void __job_run( int sig )
        #else
        inline void * __job_run( void * ptask )
        #endif
        #endif
        {
            ON_DEBUG_COTASK(
                std::cout << "in task job: " << std::endl;
        #if defined(FORCE_USE_UCONTEXT) || defined(PECO_SIGNAL_STACK)
                task_debug_info(__running_task, stderr);
        #else
                task_debug_info((task *)ptask, stderr);            
        #endif
            )
        #ifndef FORCE_USE_UCONTEXT
            #ifdef PECO_SIGNAL_STACK
            auto job = __running_task->pjob;
            #else
            full_task_t *_task = (full_task_t *)ptask;
            auto job = _task->pjob;
            #endif
            #ifdef PECO_SIGNAL_STACK
            if ( !setjmp(__running_task->buf) ) return;
            #else
            if ( !setjmp(_task->buf) ) {
                pthread_exit(NULL);
                return NULL;
            }
            #endif
        #else
            auto job = __running_task->pjob;
        #endif
            try {
                (*job)(); 
            } catch( const char * msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
            } catch( std::string& msg ) {
                std::cerr << "exception: " << msg << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
            } catch( std::stringstream& msg ) {
                std::cerr << "exception: " << msg.str() << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
            } catch ( std::logic_error& ex ) {
                std::cerr << "logic error: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
            } catch ( std::runtime_error& ex ) {
                std::cerr << "runtime error: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
            } catch ( std::bad_exception& ex ) {
                std::cerr << "bad exception: " << ex.what() << ", task info: " << std::endl;
                task_debug_info( (task *)__running_task, stderr );
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
                task_debug_info( (task *)__running_task, stderr );
            }
            #ifndef FORCE_USE_UCONTEXT
            __running_task->status = task_status_stopped;
            // Long Jump back to the main context
            longjmp( R_MAIN_CTX, 1 );

            #ifndef PECO_SIGNAL_STACK
            return NULL;
            #endif

            #endif
        }

        full_task_t* __create_task( task_job_t job ) {

            // The task should be put on the heap
            full_task_t * _ptask = (full_task_t *)malloc(sizeof(full_task_t));
            _ptask->id = (intptr_t)(_ptask);

            // Init the stack for the task
            // Get a 512KB Memory piece
            _ptask->stack = (stack_ptr)malloc(TASK_STACK_SIZE);

            // Save the job on heap
            _ptask->pjob = new task_job_t(job);

            #ifdef FORCE_USE_UCONTEXT

            // Init the context
            getcontext(&(_ptask->ctx));
            _ptask->ctx.uc_stack.ss_sp = _ptask->stack;
            _ptask->ctx.uc_stack.ss_size = TASK_STACK_SIZE;
            _ptask->ctx.uc_stack.ss_flags = 0;
            _ptask->ctx.uc_link = P_MAIN_CTX;
            errno = 0;
            makecontext(&(_ptask->ctx), (context_job_t)&__job_run, 0);
            if ( errno != 0 ) {
                std::cerr << "makecontext failed: " << errno << ", "
                    << strerror(errno) << std::endl;
            }

            #else

            #ifdef PECO_SIGNAL_STACK

            _ptask->task_stack.ss_flags = 0;
            _ptask->task_stack.ss_size = TASK_STACK_SIZE;
            _ptask->task_stack.ss_sp = _ptask->stack;

            #else

            pthread_attr_init(&_ptask->task_attr);
            pthread_attr_setstack(&_ptask->task_attr, _ptask->stack, TASK_STACK_SIZE);

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
            _ptask->p_task = (full_task_t *)__running_task;
            // I have no child task
            _ptask->c_task = NULL;
            // I have no newer sibling node
            _ptask->prev_task = NULL;
            // Init my next sibline node to NULL
            _ptask->next_task = NULL;

            // Next Timed task init to be NULL
            _ptask->next_action_task = NULL;

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
            _ptask->exitjob = NULL;

            return _ptask;
        }
        // Yield current task and switch to main loop
        void __swap_to_main() {
            ON_DEBUG_COTASK(
                std::cout << "X " << __running_task->id << " => main" << std::endl;
            )
            #ifdef FORCE_USE_UCONTEXT
            swapcontext(&(__running_task->ctx), P_MAIN_CTX);
            // setcontext(&main_ctx_);
            #else
            if ( !setjmp(((full_task_t *)__running_task)->buf) ) {
                longjmp(R_MAIN_CTX, 1);
            }
            #endif
        }
        void __reset_task( full_task_t* ptask ) {
        #ifdef FORCE_USE_UCONTEXT
            getcontext(&(ptask->ctx));
            ptask->ctx.uc_stack.ss_sp = ptask->stack;
            ptask->ctx.uc_stack.ss_size = TASK_STACK_SIZE;
            ptask->ctx.uc_stack.ss_flags = 0;
            ptask->ctx.uc_link = P_MAIN_CTX;

            // Save the job on heap
            makecontext(&(ptask->ctx), (context_job_t)&__job_run,0);
        #else

        #ifdef PECO_SIGNAL_STACK
            // Reset the task stack to the beginning
            ptask->task_stack.ss_sp = ptask->stack;
            ptask->task_stack.ss_flags = 0;
        #else
            // Create another stack
        #endif
            // make this task looks like a new one
            ptask->status = task_status_pending;
        #endif
            ON_DEBUG_COTASK(
                std::cout << "after reset task: " << std::endl;
                task_debug_info(ptask, stderr);
            )
        }

        full_task_t* __create_task_with_fd( int fd, task_job_t job ) {
            full_task_t * _ptask = __create_task( job );
            _ptask->id = (intptr_t)fd;
            _ptask->is_event_id = true;
            return _ptask;
        }

        void __destory_task( full_task_t * ptask ) {
            ON_DEBUG_COTASK(
                std::cerr << "destroying task: " << std::endl;
                task_debug_info(ptask, stderr);
            )
            if ( ptask->exitjob ) {
                (*ptask->exitjob)();
                delete ptask->exitjob;
                ptask->exitjob = NULL;
            }

            delete ptask->pjob;
            if ( ptask->hold_sig[0] != -1 ) {
                ::close(ptask->hold_sig[0]);
                ptask->hold_sig[0] = -1;
            }
            if ( ptask->hold_sig[1] != -1 ) {
                ::close(ptask->hold_sig[1]);
                ptask->hold_sig[1] = -1;
            }
            if ( ptask->is_event_id ) {
                ::close(ptask->id);
                ptask->id = 0;
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

            #ifndef FORCE_USE_UCONTEXT
            #ifndef PECO_SIGNAL_STACK
            pthread_attr_destroy(&ptask->task_attr);
            #endif
            #endif
            free(ptask);
        }

        // Insert Task to the timed list
        void __insert_task( full_task_t * ptask ) {
            // Set the next action to be null
            ptask->next_action_task = NULL;
            if ( __timed_task_root == NULL ) {
                __timed_task_root = ptask;
            }
            if ( __timed_task_tail != NULL ) {
                __timed_task_tail->next_action_task = ptask;
            }
            __timed_task_tail = ptask;
            ON_DEBUG_COTASK(
                std::cout << "after insert timed task: " << ptask->id << 
                    ", now order is: ";
                full_task_t *_t = __timed_task_root;
                while ( _t != NULL ) {
                    std::cout << _t->id;
                    if ( _t->next_action_task != NULL ) {
                        std::cout << ", ";
                    }
                    _t = _t->next_action_task;
                }
                std::cout << std::endl;
            )
        }

        void __sort_insert_read_task( full_task_t * ptask ) {
            ptask->next_action_task = NULL;
            if ( __read_task_root == NULL ) {
                __read_task_root = ptask;
                return;
            }
            full_task_t *_prev_task = NULL;
            full_task_t *_cur_task = __read_task_root;
            while ( _cur_task->next_action_task != NULL ) {
                if ( _cur_task->read_until > ptask->read_until ) break;
                _prev_task = _cur_task;
                _cur_task = _cur_task->next_action_task;
            }
            ptask->next_action_task = _cur_task;
            if ( _prev_task == NULL ) {
                __read_task_root = ptask;
            } else {
                _prev_task->next_action_task = ptask;
            }
        }

        void __sort_insert_write_task( full_task_t * ptask ) {
            ptask->next_action_task = NULL;
            if ( __write_task_root == NULL ) {
                __write_task_root = ptask;
                return;
            }
            full_task_t *_prev_task = NULL;
            full_task_t *_cur_task = __write_task_root;
            while ( _cur_task->next_action_task != NULL ) {
                if ( _cur_task->write_until > ptask->write_until ) break;
                _prev_task = _cur_task;
                _cur_task = _cur_task->next_action_task;
            }
            ptask->next_action_task = _cur_task;
            if ( _prev_task == NULL ) {
                __write_task_root = ptask;
            } else {
                _prev_task->next_action_task = ptask;
            }
        }

        void __remove_task( full_task_t * ptask ) {
            full_task_t *_cur_task = __timed_task_root;
            if ( _cur_task == NULL ) return;
            full_task_t *_prev_task = NULL;
            do {
                if ( _cur_task == ptask ) {
                    if ( _prev_task == NULL ) {
                        // This is root
                        __timed_task_root = _cur_task->next_action_task;
                    } else {
                        _prev_task->next_action_task = _cur_task->next_action_task;
                    }
                    // Check if match task is tail
                    if ( __timed_task_tail == _cur_task ) {
                        __timed_task_tail = _prev_task;
                    }
                    break;
                }
                // Go to check next
                _prev_task = _cur_task;
                _cur_task = _cur_task->next_action_task;
            } while ( _cur_task != NULL );
            ptask->next_action_task = NULL;
            ON_DEBUG_COTASK(
                std::cout << "after remove task: " << ptask->id << 
                    ", now order is: ";
                full_task_t *_t = __timed_task_root;
                while ( _t != NULL ) {
                    std::cout << _t->id;
                    if ( _t->next_action_task != NULL ) {
                        std::cout << ", ";
                    }
                    _t = _t->next_action_task;
                }
                std::cout << std::endl;
            )
        }

        void __remove_read_task( full_task_t * ptask ) {
            full_task_t *_cur_task = __read_task_root;
            if ( _cur_task == NULL ) return;
            full_task_t *_prev_task = NULL;
            do {
                if ( _cur_task == ptask ) {
                    if ( _prev_task == NULL ) {
                        __read_task_root = _cur_task->next_action_task;
                    } else {
                        _prev_task->next_action_task = _cur_task->next_action_task;
                    }
                    break;
                }
                _prev_task = _cur_task;
                _cur_task = _cur_task->next_action_task;
            } while ( _cur_task != NULL );
            ptask->next_action_task = NULL;
        }

        void __remove_write_task( full_task_t * ptask ) {
            full_task_t *_cur_task = __write_task_root;
            if ( _cur_task == NULL ) return;
            full_task_t *_prev_task = NULL;
            do {
                if ( _cur_task == ptask ) {
                    if ( _prev_task == NULL ) {
                        __write_task_root = _cur_task->next_action_task;
                    } else {
                        _prev_task->next_action_task = _cur_task->next_action_task;
                    }
                    break;
                }
                _prev_task = _cur_task;
                _cur_task = _cur_task->next_action_task;
            } while ( _cur_task != NULL );
            ptask->next_action_task = NULL;
        }

        // Internal task switcher
        void __switch_task( full_task_t* ptask ) {
            #ifndef FORCE_USE_UCONTEXT
            auto _tstatus = ptask->status;
            #endif
            if ( ptask->status != task_status_stopped ) {
                ptask->status = task_status_running;
            }

            ON_DEBUG_COTASK(
                std::cout << CLI_BLUE << "X main => " << ptask->id << CLI_NOR << std::endl;
            )
            __running_task = ptask;

            #ifdef FORCE_USE_UCONTEXT
            // If the task finishs and quit, then will continue
            // the main loop because we set uc_link as the main ctx
            // when we create the task
            // otherwise, the yield function will change the
            // next_time of the task and insert it back to
            // the task cache.
            swapcontext(P_MAIN_CTX, &(ptask->ctx));
            #else

            #ifdef PECO_SIGNAL_STACK
            // Set to use the target task's stack to handler the signal, 
            // which means context swap
            // New task
            if ( _tstatus == task_status_pending ) {
                // Raise the signal to switch to the stack
                // And the handler will call setjmp and return immediately
                sigaltstack(&(ptask->task_stack), NULL);

                raise(SIGUSR1);

                // Disable the altstack
                ptask->task_stack.ss_flags = SS_DISABLE;
                sigaltstack(&(ptask->task_stack), NULL);
            }
            #else

            // Create a thread, and it will return immediately
            if ( _tstatus == task_status_pending ) {
                pthread_t _t;
                ignore_result( pthread_create(&_t, &(ptask->task_attr), &__job_run, (void *)ptask) );
                void *_st;
                pthread_join(_t, &_st);
            }

            #endif
            // Save current main jmp's position and jump to the 
            // task stack
            if ( !setjmp(R_MAIN_CTX) ) {
                longjmp(ptask->buf, 1);
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
                    // --task_count_;
                    return;
                }
                if ( ptask->repeat_count != infinitive_task ) {
                    --ptask->repeat_count;
                }
                ptask->next_time += ptask->duration;
                __reset_task(ptask);
                __insert_task(ptask);
            } else if ( ptask->status == task_status::task_status_stopped ) {
                // The task has been stopped by someone, quit it
                __destory_task(ptask);
                // --task_count_;
                return;
            }
            // else, the task is paused, do nothing
        }

        bool __read_task_contains_fd( long fd ) {
            full_task_t *_task = __read_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return true;
                _task = _task->next_action_task;
            }
            return false;
        }

        full_task_t * __find_read_task_by_fd( long fd ) {
            full_task_t *_task = __read_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return _task;
                _task = _task->next_action_task;
            }
            return NULL;
        }

        bool __write_task_contains_fd( long fd ) {
            full_task_t *_task = __write_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return true;
                _task = _task->next_action_task;
            }
            return false;
        }

        full_task_t * __find_write_task_by_fd( long fd ) {
            full_task_t *_task = __write_task_root;
            while ( _task != NULL ) {
                if ( _task->related_fd == fd ) return _task;
                _task = _task->next_action_task;
            }
            return NULL;
        }

        // Default C'str
        loop::loop() : 
            core_fd_(-1), core_events_(NULL),
            task_count_(0), 
            running_(false), ret_code_(0) 
        { 
            if ( __this_loop != this ) {
                throw std::runtime_error("cannot create more than one loop in one thread");
            }

            #ifndef FORCE_USE_UCONTEXT

            #ifdef PECO_SIGNAL_STACK
            // register the signal to run job
            struct sigaction    sa;            
            sa.sa_handler = &__job_run;
            sa.sa_flags = SA_ONSTACK;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGUSR1, &sa, NULL);
            #endif

            #endif
        }
        // Default D'str
        loop::~loop() {
            if ( __running_task != NULL ) {
                __destory_task((full_task_t *)__running_task);
                --task_count_;
            }
            while ( __timed_task_root != NULL ) {
                full_task_t *_task = __timed_task_root;
                __remove_task(_task);
                __destory_task(_task);
                --task_count_;
            }

            while ( __read_task_root != NULL ) {
                full_task_t *_task = __read_task_root;
                __remove_read_task(_task);
                __destory_task(_task);
                --task_count_;
            }

            while ( __write_task_root != NULL ) {
                full_task_t *_task = __write_task_root;
                __remove_write_task(_task);
                __destory_task(_task);
                --task_count_;
            }

            if ( core_fd_ == -1 ) return;
            ::close(core_fd_);
            free(core_events_);
        }

		// Get current running task
        task* loop::get_running_task() { return (task *)__running_task; }

        // Get task count
        uint64_t loop::task_count() const { return task_count_; }
        // Get the return code
        int loop::return_code() const { return ret_code_; }

        task * loop::do_job( task_job_t job ) {
            full_task_t *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();

            ON_DEBUG_COTASK(
                std::cout << "create a job: " << std::endl;
                task_debug_info(_ptask, stderr);
            )
            __insert_task(_ptask);

            // Switch to main and run the task
            this_task::yield();
            return _ptask;
        }

        task * loop::do_job( long fd, task_job_t job ) {
            if ( fd == -1 ) return NULL;
            full_task_t *_ptask = __create_task_with_fd( fd, job );
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();

            ON_DEBUG_COTASK(
                std::cout << "create a job: " << std::endl;
                task_debug_info(_ptask, stderr);
            )
            __insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        task * loop::do_loop( task_job_t job, duration_t interval ) {
            full_task_t *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = infinitive_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now();
            _ptask->duration = interval;
            ON_DEBUG_COTASK(
                std::cout << "create a job: " << std::endl;
                task_debug_info(_ptask, stderr);
            )
            __insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        task * loop::do_delay( task_job_t job, duration_t delay ) {
            full_task_t *_ptask = __create_task(job);
            ++task_count_;
            _ptask->repeat_count = oneshot_task;
            _ptask->next_time = std::chrono::high_resolution_clock::now() + delay;
            ON_DEBUG_COTASK(
                std::cout << "create a job: " << std::endl;
                task_debug_info(_ptask, stderr);
            )
            __insert_task(_ptask);

            // Switch
            this_task::yield();
            return _ptask;
        }

        // Event Monitor
        bool loop::monitor_task( int fd, task * ptask, event_type eventid, duration_t timedout ) {
            // Add to the core_events
            full_task_t *_ptask = (full_task_t *)ptask;
            int _c_eid = (eventid == event_type::event_read ? core_event_read : core_event_write);
            if ( eventid == event_type::event_read ) {
                if ( __read_task_contains_fd( fd ) ) return false;
#ifndef __APPLE__
                if ( __write_task_contains_fd( fd ) ) {
                    _c_eid |= core_event_write;
                }
#endif
            } else {
                if ( __write_task_contains_fd(fd) ) return false;
#ifndef __APPLE__
                if ( __read_task_contains_fd( fd ) ) {
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
                _ptask->read_until = task_time_now() + timedout;
                ON_DEBUG_COTASK(
                    std::cout << "* MR <" << fd << ":" << _ptask->id 
                        << "> for read event, timedout at "
                        << _ptask->read_until.time_since_epoch().count() << std::endl;
                )
                __sort_insert_read_task(_ptask);
            } else {
                _ptask->write_until = task_time_now() + timedout;
                ON_DEBUG_COTASK(
                    std::cout << "* MW <" << fd << ":" << _ptask->id 
                        << "> for write event, timedout at "
                        << _ptask->write_until.time_since_epoch().count() << std::endl;
                )
                __sort_insert_write_task(_ptask);
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
            full_task_t *_ptask = (full_task_t *)ptask;
            if ( _ptask->related_fd != -1 ) {
                this->cancel_fd(_ptask->related_fd);
            }
        }

        // Cancel all monitored task of specified fd
        void loop::cancel_fd( long fd ) {
            full_task_t *_rtask = __find_read_task_by_fd(fd);
            if ( _rtask != NULL ) {
                __remove_read_task(_rtask);
                _rtask->read_until = task_time_now();
                __sort_insert_read_task(_rtask);
            }
            full_task_t *_wtask = __find_write_task_by_fd(fd);
            if ( _wtask != NULL ) {
                __remove_write_task(_wtask);
                _wtask->write_until = task_time_now();
                __sort_insert_write_task(_wtask);
            }
        }

        // Timed task schedule
        void loop::__timed_task_schedule() {
            auto _now = task_time_now();
            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __timed_task_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )
            while ( __timed_task_root != NULL && __timed_task_root->next_time <= _now ) {
                full_task_t *_task = __timed_task_root;
                __remove_task(_task);
                __switch_task(_task);
            }
        }
        // Event Task schedule
        void loop::__event_task_schedule() {
            if ( 
                __timed_task_root == NULL &&
                __read_task_root == NULL &&
                __write_task_root == NULL
            ) return;

            uint32_t _wait_time = 0;
            auto _now = task_time_now();
            auto _nearest = _now;

            ON_DEBUG_COTASK(
                std::cout << CLI_COFFEE << "@ __event_task_schedule@" << 
                    _now.time_since_epoch().count() << CLI_NOR << std::endl;
            )
            if ( __timed_task_root != NULL ) {
                _nearest = __timed_task_root->next_time;
            }
            if ( __read_task_root != NULL ) {
                if ( __read_task_root->read_until < _nearest ) {
                    _nearest = __read_task_root->read_until;
                }
            }
            if ( __write_task_root != NULL ) {
                if ( __write_task_root->write_until < _nearest ) {
                    _nearest = __write_task_root->write_until;
                }
            }
            // Check the top timedout, if we already passed the time, wait 0
            if ( _nearest > _now ) {
                auto _ts = _nearest - _now;
                auto _ms = std::chrono::duration_cast< std::chrono::milliseconds >(_ts);
                _wait_time = (uint32_t)_ms.count();
            } else {
                _wait_time = 0;
            }

            int _count = core_event_wait(
                core_fd_, core_events_, CO_MAX_SO_EVENTS, _wait_time
            );

            if ( _count == -1 ) return;
            // If have any event, then do something
            // If not, then continue the loop
            for ( int i = 0; i < _count; ++i ) {
                core_event_t *_pe = core_events_ + i;
                int _fd = (intptr_t)core_get_so(_pe);

                do {
                    // We only get outgoing event, do nothing in
                    // read event block
                    if ( core_is_outgoing(_pe) ) break;

                    // Find the task in the read cache
                    full_task_t *_rt = __find_read_task_by_fd(_fd);
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
                    full_task_t *_wt = __find_write_task_by_fd(_fd);
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
                if ( __find_read_task_by_fd(_fd) ) {
                    // Read not timedout, do it in the next runloop
                    core_event_t _e;
                    core_event_init(_e);
                    core_event_prepare(_e, _fd);
                    core_make_flag(_e, core_flag_add);
                    core_mask_flag(_e, core_flag_oneshot);
                    core_event_ctl_with_flag(core_fd_, _fd, _e, core_event_read);
                }
                if ( __find_write_task_by_fd(_fd) ) {
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

            std::list< full_task_t * > _timedout_tasks;
            while ( __read_task_root != NULL ) {
                full_task_t *_task = __read_task_root;
                if ( _task->read_until > _now ) break;

                // Remove read event
                core_event_t _e;
                core_event_init(_e);
                core_event_prepare(_e, (int)_task->related_fd);
                core_event_del(core_fd_, _e, (int)_task->related_fd, core_event_read);

                _task->signal = waiting_signals::no_signal;
                _timedout_tasks.push_back(_task);
                __remove_read_task(_task);
            }

            while ( __write_task_root != NULL ) {
                full_task_t *_task = __write_task_root;
                if ( _task->write_until > _now ) break;

                core_event_t _e;
                core_event_init(_e);
                core_event_prepare(_e, (int)_task->related_fd);
                core_event_del(core_fd_, _e, (int)_task->related_fd, core_event_write);

                _task->signal = waiting_signals::no_signal;
                _timedout_tasks.push_back(_task);
                __remove_write_task(_task);
            }

            for ( full_task_t * t : _timedout_tasks ) {
                __switch_task(t);
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
                __timed_task_root != NULL ||
                __read_task_root != NULL ||
                __write_task_root != NULL
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
            if ( __running_task != NULL ) {
                __destory_task((full_task_t *)__running_task);
                --task_count_;
                __running_task = NULL;
            }
            while ( __timed_task_root != NULL ) {
                full_task_t *_task = __timed_task_root;
                __remove_task(_task);
                __destory_task(_task);
                --task_count_;
            }
            while ( __read_task_root != NULL ) {
                full_task_t *_task = __read_task_root;
                __remove_read_task(_task);
                __destory_task(_task);
                --task_count_;
            }
            while ( __write_task_root != NULL ) {
                full_task_t *_task = __write_task_root;
                __remove_write_task(_task);
                __destory_task(_task);
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

            __dump_call_stack(fp, lv);

            if ( ptask == NULL ) {
                fprintf(fp, "%*s|= <null>\n", lv * 4, ""); return;
            } else {
                fprintf(fp, "%*s>>%s<<\n", lv * 4, "", utils::ptr_str(ptask).c_str() );
            }

            full_task_t *_ptask = (full_task_t *)ptask;

            fprintf(fp, "%*s|= id: %ld\n", lv * 4, "", _ptask->id);
            fprintf(fp, "%*s|= is_event: %s\n", lv * 4, "", (_ptask->is_event_id ? "true" : "false"));
            fprintf(fp, "%*s|= stack: %s\n", lv * 4, "", pe::utils::ptr_str(_ptask->stack).c_str());
            utils::hex_dump _hd((const char *)_ptask->stack, 32);
            fprintf(fp, "%*s|= ->%s\n", lv * 4, "", _hd.line(0));
            fprintf(fp, "%*s|= ->%s\n", lv * 4, "", _hd.line(1));
            fprintf(fp, "%*s|= status: %s\n", lv * 4, "", 
                (_ptask->status == task_status_pending ? "pending" : 
                    (_ptask->status == task_status_paused ? "paused" : 
                        (_ptask->status == task_status_stopped ? "stopped" : 
                            "running"))));
            fprintf(fp, "%*s|= signal: %s\n", lv * 4, "", (_ptask->signal == no_signal ? "no_signal" : 
                (_ptask->signal == bad_signal ? "bad_signal" : "receive_signal")));
            fprintf(fp, "%*s|= hold_sig: %d, %d\n", lv * 4, "", _ptask->hold_sig[0], _ptask->hold_sig[1]);
            fprintf(fp, "%*s|= related_fd: %ld\n", lv * 4, "", _ptask->related_fd);
            if ( _ptask->repeat_count == infinitive_task ) {
                fprintf(fp, "%*s|= repeat_count: infinitive\n", lv * 4, "");
            } else {
                fprintf(fp, "%*s|= repeat_count: %lu\n", lv * 4, "", _ptask->repeat_count);
            }
            fprintf(fp, "%*s|= flags: %x\n", lv * 4, "", *(uint8_t *)&_ptask->reserved_flags);
            fprintf(fp, "%*s|= exitjob: %s\n", lv * 4, "", utils::ptr_str(_ptask->exitjob).c_str());

            if ( _ptask->p_task != NULL ) {
                fprintf(fp, "%*s|= parent: \n", lv * 4, "");
                task_debug_info( _ptask->p_task, fp, lv + 1 );
            }
        }
        // Force to exit a task
        void task_exit( task * ptask ) {
            if ( ptask == NULL || ptask == __running_task ) return;
            full_task_t *_ptask = (full_task_t *)ptask;
            _ptask->status = task_status_stopped;
            __this_loop->cancel_monitor(ptask);
            __remove_task((full_task_t *)ptask);
            _ptask->next_time = task_time_now();
            _ptask->repeat_count = 1;
            // Only put back a task not holding anything
            if ( _ptask->related_fd == -1 ) {
                __insert_task((full_task_t *)ptask);
            }
        }
        // Go on a task
        void task_go_on( task * ptask ) {
            if ( ptask == NULL ) return;
            full_task_t *_ptask = (full_task_t *)ptask;
            if ( _ptask->hold_sig[1] == -1 ) return;
            ignore_result(write( _ptask->hold_sig[1], "1", 1 ));
        }

        // Stop a task's holding
        void task_stop( task * ptask ) {
            if ( ptask == NULL ) return;
            full_task_t *_ptask = (full_task_t *)ptask;
            if ( _ptask->hold_sig[1] != -1 ) {
                ignore_result(write( _ptask->hold_sig[1], "0", 1 ));
            }
            __this_loop->cancel_monitor(ptask);
        }
        // Register task's exit job
        void task_register_exit_job(task * ptask, task_job_t job) {
            if ( ptask == NULL ) return;
            full_task_t *_ptask = (full_task_t *)ptask;
            if ( _ptask->exitjob ) {
                delete _ptask->exitjob;
            }
            _ptask->exitjob = new task_job_t(job);
        }

        // Remove task exit job
        void task_remove_exit_job(task * ptask) {
            if ( ptask == NULL ) return;
            full_task_t *_ptask = (full_task_t *)ptask;
            if ( _ptask->exitjob == NULL ) return;
            delete _ptask->exitjob;
            _ptask->exitjob = NULL;
        }

        namespace this_task {
            // Get the task point
            task * get_task() { return (task *)__running_task; }
            // Wrapper current task
            other_task wrapper() { return other_task((task *)__running_task); }
            // Get the task id
            task_id get_id() {
                if ( __running_task == NULL ) return 0;
                return __running_task->id;
            }
            // Get task status
            task_status status() {
                if ( __running_task == NULL ) return task_status_stopped;
                return __running_task->status;
            }
            // Get task last waiting signal
            waiting_signals last_signal() {
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
                __running_task->status = pe::co::task_status_paused;
                // Update the next cpu time
                __running_task->next_time = std::chrono::high_resolution_clock::now();
                __insert_task(__running_task);

                // Force to switch to main context
                __swap_to_main();
            }

            // Yield if the condition is true
            bool yield_if( bool cond ) { 
                if ( cond ) yield(); 
                if ( __running_task == NULL ) return cond;
                return cond && (__running_task->status != task_status_stopped);
            }

            // Yield if the condition is false
            bool yield_if_not( bool cond ) {
                if ( !cond ) yield();
                if ( __running_task == NULL ) return cond;
                return cond && (__running_task->status != task_status_stopped);
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
                    __running_task->status = pe::co::task_status::task_status_paused;
                    if ( 
                        !__this_loop->monitor_task(
                            __running_task->hold_sig[0], __running_task, 
                            event_type::event_read, std::chrono::seconds(1)
                        )
                    ) {
                        return false;
                    }
                    __swap_to_main();
                } while ( __running_task->signal == pe::co::no_signal );
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->signal == pe::co::bad_signal ) return false;
                char _f;
                ignore_result(read( __running_task->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }
            // Hold the task until timedout
            bool holding_until( pe::co::duration_t timedout ) {
                if ( __running_task == NULL ) return false;
                if ( __running_task->hold_sig[0] == -1 ) {
                    while ( yield_if( pipe(__running_task->hold_sig) < 0 ) );
                }
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->related_fd != -1 ) return false;   // Already monitoring on something
                __running_task->status = pe::co::task_status::task_status_paused;
                if ( 
                    !__this_loop->monitor_task(
                        __running_task->hold_sig[0], __running_task, 
                        event_type::event_read, timedout
                    )
                ) {
                    return false;
                }
                __running_task->related_fd = __running_task->hold_sig[0];
                __swap_to_main();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return false;
                if ( __running_task->signal == pe::co::bad_signal ) return false;
                if ( __running_task->signal == pe::co::no_signal ) return false;
                char _f;
                ignore_result(read( __running_task->hold_sig[0], &_f, 1 ));
                return (_f == '1');
            }

            // Wait on event
            pe::co::waiting_signals wait_for_event(
                pe::co::event_type eventid, 
                pe::co::duration_t timedout
            ) {
                if ( __running_task == NULL ) return pe::co::bad_signal;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                if ( __running_task->related_fd != -1 ) return bad_signal;
                __running_task->status = pe::co::task_status::task_status_paused;
                if ( 
                    !__this_loop->monitor_task( 
                        (int)__running_task->id, __running_task, eventid, timedout 
                    ) 
                ) {
                    return pe::co::bad_signal;
                }
                __running_task->related_fd = (long)__running_task->id;
                __swap_to_main();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                return __running_task->signal;
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
                if ( __running_task == NULL ) return pe::co::bad_signal;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                __running_task->status = pe::co::task_status::task_status_paused;
                __running_task->related_fd = fd;
                if ( 
                    !__this_loop->monitor_task( 
                        fd, __running_task, eventid, timedout 
                    ) 
                ) {
                    __running_task->related_fd = -1;
                    return pe::co::bad_signal;
                }
                __swap_to_main();
                __running_task->related_fd = -1;
                if ( __running_task->status == task_status_stopped ) return bad_signal;
                return __running_task->signal;
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
                if ( ptask == NULL || 
                    ((full_task_t *)ptask)->is_event_id == false ) 
                    return pe::co::bad_signal;
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
                if ( __running_task == NULL ) return;
                if ( __running_task->status == task_status_stopped ) return;

                // Put current task to the end of the queue
                __running_task->status = pe::co::task_status_paused;
                // Update the next cpu time
                __running_task->next_time = std::chrono::high_resolution_clock::now() + duration;
                __insert_task(__running_task);

                // Force to switch to main context
                __swap_to_main();
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
                if ( __running_task == NULL ) return NULL;
                return __running_task->p_task;
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
            }
        }
    }
}

#ifdef FORCE_USE_UCONTEXT
#ifdef __APPLE__
#pragma clang diagnostic pop
#endif
#endif

// Push Chen

