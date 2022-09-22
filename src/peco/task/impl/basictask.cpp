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

#include <unordered_map>

namespace peco {

/**
 * @brief Global Main Context
*/
stack_context_t* get_main_context() {
  thread_local static task_context_t main_task;
  return &main_task.ctx;
}

typedef void (*__context_fn_t)(void);

#if PECO_TARGET_APPLE
void* __context_main__(void * ptask) {
  task_context_t* raw_task = reinterpret_cast<task_context_t *>(ptask);
  if (!setjmp(raw_task->ctx)) {
    // Use pthread exit to keep the stack not been broken.
    pthread_exit(NULL);
    // This will never be executed.
    assert(0);
  }
  raw_task->worker();
  longjmp(*get_main_context(), 1);
  return (void *)0;
}
#else
void __context_main__(void * ptask) {
  task_context_t* raw_task = reinterpret_cast<task_context_t *>(ptask);
  raw_task->worker();
}
#endif

typedef std::shared_ptr<basic_task> basic_task_ptr_t;

/**
 * @brief Task Cache
*/
std::unordered_map<task_id_t, basic_task_ptr_t>& get_task_cache() {
  thread_local static std::unordered_map<task_id_t, basic_task_ptr_t> g_cache;
  return g_cache;
}

/**
 * @brief Create a task with worker
*/
basic_task::basic_task(worker_t worker, repeat_count_t repeat_count, duration_t interval)
  : buffer_(stack_cache::fetch()) {
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
  auto r = basic_task::running_task();
  if (r) {
    extra_->parent_tid = r->task_id();
  } else {
    extra_->parent_tid = kInvalidateTaskId;
  }

  this->reset_task();
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
  stack_cache::release(buffer_);
}

/**
 * @brief force to create a shared ptr task
*/
std::shared_ptr<basic_task> basic_task::create_task(
  worker_t worker, repeat_count_t repeat_count, duration_t interval) {

  auto ptr = std::shared_ptr<basic_task>(new basic_task(worker, repeat_count, interval));
  get_task_cache()[ptr->task_id()] = ptr;
  return ptr;
}

/**
 * @brief Destroy this task, will remove from global cache
*/
void basic_task::destroy_task() {
  get_task_cache().erase(task_->tid);
}

/**
 * @brief Reset the task's status and ready for another loop
 * Only use this method in repeatable task
*/
void basic_task::reset_task() {
#if PECO_TARGET_APPLE
  pthread_t _t;
  // according to the office document, a pthread_attr_t can be
  // used to create multiple thread, and will only be used when
  // create the thread. So after join the thread, the attr can
  // be destroied immediately.
  pthread_attr_t _attr;
  pthread_attr_init(&_attr);
  ignore_result(pthread_attr_setstack(&_attr, task_->stack, (TASK_STACK_SIZE - STACK_RESERVED_SIZE)));
  ignore_result(pthread_create(&_t, &_attr, __context_main__, (void *)buffer_->buf));
  pthread_join(_t, nullptr);
  pthread_attr_destroy(&_attr);
#else
  getcontext(&(task_->ctx));
  task_->ctx.uc_stack.ss_sp = task_->stack;
  task_->ctx.uc_stack.ss_size = TASK_STACK_SIZE - STACK_RESERVED_SIZE;
  task_->ctx.uc_stack.ss_flags = 0;
  task_->ctx.uc_link = get_main_context();

  // save the job on heap
  makecontext(&(task_->ctx), (__context_fn_t)__context_main__, 1, task_);
#endif
  task_->status = kTaskStatusPending;
}

/**
 * @brief Swap to current task
*/
void basic_task::swap_to_task() {
  if (task_->status == kTaskStatusStopped) {
    return;
  }
  // Update running task
  basic_task::running_task() = this->shared_from_this();
  task_->status = kTaskStatusRunning;

#if PECO_TARGET_APPLE
  if (!setjmp(*get_main_context())) {
    longjmp(task_->ctx, 1);
  }
#else
  swapcontext(get_main_context(), &(this->task_->ctx));
#endif

  basic_task::running_task() = nullptr;

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
    this->reset_task();
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
 * @brief Get Current running task
*/
std::shared_ptr<basic_task>& basic_task::running_task() {
  thread_local static std::shared_ptr<basic_task> s_running_task = nullptr;
  return s_running_task;
}

/**
 * @brief Swap to main context
*/
void basic_task::swap_to_main() {
#if PECO_TARGET_APPLE
  if (!setjmp(basic_task::running_task()->task_->ctx)) {
    longjmp(*get_main_context(), 1);
  }
#else
  swapcontext(&(basic_task::running_task()->task_->ctx), get_main_context());
#endif
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

/**
 * @brief Fetch the created task by its id
*/
std::shared_ptr<basic_task> basic_task::fetch(task_id_t tid) {
  if (tid == kInvalidateTaskId) return nullptr;
  auto t_it = get_task_cache().find(tid);
  if (t_it == get_task_cache().end()) {
    return nullptr;
  }
  return t_it->second;
}
/**
 * @brief Get the cache task count
*/
size_t basic_task::cache_size() {
  return get_task_cache().size();
}

/**
 * @brief Scan all cached task
*/
void basic_task::foreach(std::function<void(std::shared_ptr<basic_task>)> handler) {
  for (const auto& item : get_task_cache()) {
    handler(item.second);
  }
}

} // namespace peco

// Push Chen
