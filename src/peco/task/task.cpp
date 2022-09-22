/*
    task.cpp
    libpeco
    2022-02-10
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

#include "peco/task/task.h"
#include "peco/task/impl/basictask.hxx"
#include "peco/task/impl/loopimpl.hxx"

namespace peco {

/**
 * @brief Create a task by its id
*/
task::task(task_id_t tid) : tid_(tid) {}

/**
 * @brief Copy & Move C'stor
*/
task::task(const task& other) : tid_(other.tid_) {}
task::task(task&& other) : tid_(other.tid_) {}
/**
 * @brief Operator = & Operator move
*/
task& task::operator = (const task& other) {
  if (this != &other) {
    tid_ = other.tid_;
  }
  return *this;
}
task& task::operator = (task&& other) {
  if (this != &other) {
    tid_ = other.tid_;
  }
  return *this;
}

/**
 * @brief Get this task object
*/
task task::this_task() {
  auto rt = basic_task::running_task();
  if (rt == nullptr) {
    return task(kInvalidateTaskId);
  } else {
    return task(rt->task_id());
  }
}

/**
 * @brief Check if the task ref is alive
*/
bool task::is_alive() const {
  if (tid_ == kInvalidateTaskId) return false;
  auto rt = basic_task::fetch(tid_);
  return (rt != nullptr);
}

/**
 * @brief Get the task id
*/
task_id_t task::task_id() const {
  return tid_;
}

/**
 * @brief Get the task's status
*/
TaskStatus task::status() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return kTaskStatusStopped;
  return rt->status();
}

/**
 * @brief Get the last waiting signal
*/
WaitingSignal task::signal() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return kWaitingSignalBroken;
  return rt->signal();
}

/**
 * @brief Set the exit function
 * @return the old exit function
*/
worker_t task::set_atexit(worker_t fn_exit) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return nullptr;
  return rt->set_atexit(fn_exit);
}

/**
 * @brief If current task is been cancelled
*/
bool task::is_cancelled() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return false;
  return rt->cancelled();
}

/**
 * @brief set arg pointer
*/
void task::set_arg(void *arg) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  rt->set_arg(arg);
}

/**
 * @brief Get arg pointer
*/
void* task::get_arg() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return NULL;
  return rt->get_arg();
}

/**
 * @brief Set flag at given index, max index is 16
*/
void task::set_flag(uint64_t flag, size_t index) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  rt->set_flag(flag, index);
}

/**
 * @brief Get flag at given index, if index is greater than 16, return -1
*/
uint64_t task::get_flag(size_t index) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return 0;
  return rt->get_flag(index);
}

/**
 * @brief Set the task's name
*/
void task::set_name(const char* name) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  rt->set_name(name);
}

/**
 * @brief Get the task's name
*/
const char* task::get_name() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return "";
  return rt->get_name();
}

/**
 * @brief Get parent task
*/
task task::parent_task() const {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return task(kInvalidateTaskId);
  return task(rt->parent_task());
}
  
/**
 * @brief Hold current task, yield, but not put back to the timed based cache
*/
bool task::holding() {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return false;
  loopimpl::shared().hold_task(rt);
  return (rt->signal() == kWaitingSignalReceived);
}

/**
 * @brief Hold the task until timedout
*/
bool task::holding_until(duration_t timedout) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return false;
  loopimpl::shared().hold_task_for(rt, timedout);
  return (rt->signal() == kWaitingSignalReceived);
}

/**
 * @brief Wakeup a holding task, cannot wakeup this_task
*/
void task::wakeup(WaitingSignal signal) {
  auto rt = basic_task::running_task();
  // Not this_task
  if (rt != nullptr && rt->task_id() == tid_) {
    return;
  }
  loopimpl::shared().wakeup_task(basic_task::fetch(tid_), signal);
  return;
}

/**
 * @brief Mask the task to be cancelled
 * all method will return immediately.
*/
void task::cancel() {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  loopimpl::shared().cancel(rt);
}

/**
 * @brief yield, force to switch task
*/
void task::yield() {
  auto rt = basic_task::running_task();
  // Must be this_task
  if (rt != nullptr && rt->task_id() != tid_) {
    return;
  }
  loopimpl::shared().yield_task(rt);
  return;
}

/**
 * @brief wait event on specified fd's event
*/
void task::wait_fd_for_event(fd_t fd, EventType e, duration_t timedout) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  if (e == kEventTypeRead) {
    loopimpl::shared().wait_for_reading(fd, rt, timedout);
  } else if (e == kEventTypeWrite) {
    loopimpl::shared().wait_for_writing(fd, rt, timedout);
  }
}

/**
 * @brief sleep current task then auto wakeup
*/
void task::sleep(duration_t duration) {
  // force to sleep, not holding
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  auto expect_end = TASK_TIME_NOW() + duration;
  do {
    loopimpl::shared().hold_task_for(rt, duration);
    // If the task has been marked cancelled, break the sleep loop
    if (rt->cancelled()) break;
    auto now = TASK_TIME_NOW();
    if (now >= expect_end) break;
    duration = (expect_end - now);
  } while (true);
}
/**
 * @brief Update a loop task's interval
*/
void task::update_interval(duration_t interval) {
  auto rt = basic_task::fetch(tid_);
  if (rt == nullptr) return;
  rt->update_interval(interval);
}

} // namespace peco

// Push Chen
