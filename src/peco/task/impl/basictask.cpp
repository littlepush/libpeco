/*
    basictask.cpp
    libpeco
    2022-02-04
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "peco/task/impl/basictask.hxx"

namespace peco {

#if PECO_TARGET_APPLE
typedef struct __ctx__ {
  void *ptask;
  stack_context_t* main_ctx;
}* ctx_t;
void* __context_main__(void * p_ctx) {
  ctx_t c = (ctx_t)p_ctx;
  task_context_t* raw_task = reinterpret_cast<task_context_t *>(c->ptask);
  if (!setjmp(raw_task->ctx)) {
    // Use pthread exit to keep the stack not been broken.
    pthread_exit(NULL);
    // This will never be executed.
    assert(0);
  }
  raw_task->worker();
  longjmp(*c->main_ctx, 1);
  return (void *)0;
}
#else
typedef void (*__context_fn_t)(void);
void __context_main__(void * ptask) {
  task_context_t* raw_task = reinterpret_cast<task_context_t *>(ptask);
  raw_task->worker();
}
#endif

/**
 * @brief Create a task with worker
*/
basic_task::basic_task(
  stack_cache::task_buffer_ptr pbuf, task_id_t running_task,
  worker_t worker, repeat_count_t repeat_count, duration_t interval)
  : buffer_(pbuf) {
  task_ = new (buffer_->buf) task_context_t;
  // Task id is the address of the buffer
  task_->tid = (task_id_t)(&buffer_->buf);
  // Set worker
  task_->worker = worker;
  task_->repeat_count = repeat_count;
  task_->interval = interval;
  // Set stack
  task_->stack = buffer_->buf + STACK_RESERVED_SIZE;

  task_->atexit = nullptr;
  task_->signal = kWaitingSignalNothing;
  task_->status = kTaskStatusPending;
  task_->cancelled = false;
  task_->next_fire_time = TASK_TIME_NOW() + interval;

  // Extra
  extra_ = reinterpret_cast<task_extra_t *>(buffer_->buf + (size_t)kTaskContextSize);
  extra_->zero1 = 0;
  extra_->name[0] = '\0';
  extra_->arg = nullptr;
  extra_->parent_tid = running_task;
}

/**
 * @brief Release the task, and invoke `atexit`
*/
basic_task::~basic_task() {
  if (task_->atexit) {
    task_->atexit();
    task_->atexit = nullptr;
  }
  task_->worker = nullptr;
}

/**
 * @brief Reset the task's status and ready for another loop
 * Only use this method in repeatable task
*/
void basic_task::reset_task(stack_context_t* main_ctx) {
#if PECO_TARGET_APPLE
  pthread_t _t;
  // according to the office document, a pthread_attr_t can be
  // used to create multiple thread, and will only be used when
  // create the thread. So after join the thread, the attr can
  // be destroied immediately.
  pthread_attr_t _attr;
  pthread_attr_init(&_attr);
  ignore_result(pthread_attr_setstack(
      &_attr, task_->stack, (TASK_STACK_SIZE - STACK_RESERVED_SIZE)));
  __ctx__ c{(void *)buffer_->buf, main_ctx};
  ignore_result(pthread_create(&_t, &_attr, __context_main__, (void *)&c));
  pthread_join(_t, nullptr);
  pthread_attr_destroy(&_attr);
#else
  getcontext(&(task_->ctx));
  task_->ctx.uc_stack.ss_sp = task_->stack;
  task_->ctx.uc_stack.ss_size = TASK_STACK_SIZE - STACK_RESERVED_SIZE;
  task_->ctx.uc_stack.ss_flags = 0;
  task_->ctx.uc_link = main_ctx;

  // save the job on heap
  makecontext(&(task_->ctx), (__context_fn_t)__context_main__, 1, task_);
#endif
  task_->status = kTaskStatusPending;
}

/**
 * @brief Swap to current task
*/
void basic_task::swap_to_task(stack_context_t* main_ctx) {
  task_->status = kTaskStatusRunning;

#if PECO_TARGET_APPLE
  if (!setjmp(*main_ctx)) {
    longjmp(task_->ctx, 1);
  }
#else
  swapcontext(main_ctx, &(this->task_->ctx));
#endif

  // Only when the task worker normal exit, the status will remined 
  // as running, then we need to check if this is a repeatable task
  // and reset it.
  // If the task is marsked as cancelled, force to stop it.
  if (task_->status == kTaskStatusRunning) {
    if (task_->repeat_count == 1 || task_->cancelled) {
      task_->status = kTaskStatusStopped;
      return;
    }
    if (task_->repeat_count != REPEAT_COUNT_INFINITIVE) {
      --task_->repeat_count;
    }
    task_->next_fire_time += task_->interval;
    this->reset_task(main_ctx);
    task_->status = kTaskStatusPending;
  }
}

/**
 * @brief Update a loop task's interval
*/
void basic_task::update_interval(duration_t interval) {
  task_->interval = interval;
}

/**
 * @brief Get the internal task pointer
*/
task_context_t* basic_task::get_task() {
  return task_;
}

/**
 * @brief Get the task id
*/
task_id_t basic_task::task_id() const {
  return task_->tid;
}

/**
 * @brief Get the task's status
*/
TaskStatus basic_task::status() const {
  return task_->status;
}

/**
 * @brief Get the last waiting signal
*/
WaitingSignal basic_task::signal() const {
  return task_->signal;
}

/**
 * @brief If the task is marked to be cancelled
*/
bool basic_task::cancelled() const {
  return task_->cancelled;
}

/**
 * @brief Set the exit function
 * @return the old exit function
*/
worker_t basic_task::set_atexit(worker_t fn_exit) {
  auto _ret = task_->atexit;
  task_->atexit = fn_exit;
  return _ret;
}

/**
 * @brief set arg pointer
*/
void basic_task::set_arg(void *arg) {
  extra_->arg = arg;
}

/**
 * @brief Get arg pointer
*/
void * basic_task::get_arg() const {
  return extra_->arg;
}

/**
 * @brief Set flag at given index, max index is 16
*/
void basic_task::set_flag(uint64_t flag, size_t index) {
  if (index >= basic_task::kMaxFlagCount) return;
  extra_->flags[index] = flag;
}

/**
 * @brief Get flag at given index, if index is greater than 16, return -1
*/
uint64_t basic_task::get_flag(size_t index) {
  if (index >= basic_task::kMaxFlagCount) return (uint64_t)(0llu - 1llu);
  return extra_->flags[index];
}

/**
 * @brief Set the task's name
*/
void basic_task::set_name(const char* name) {
  if (name == nullptr) return;
  size_t nl = strlen(name);
  size_t cl = (nl > kTaskNameLength ? kTaskNameLength : nl);
  memcpy(extra_->name, name, cl);
  if (cl < kTaskNameLength) {
    extra_->name[cl] = '\0';
  }
}

/**
 * @brief Get the task's name
*/
const char* basic_task::get_name() const {
  return extra_->name;
}

/**
 * @brief Get parent task
*/
task_id_t basic_task::parent_task() const {
  return extra_->parent_tid;
}

} // namespace peco

// Push Chen
