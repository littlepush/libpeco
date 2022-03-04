/*
    task.cpp
    libpeco
    2022-02-14
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

#include "peco/task/shared/task.h"
#include "peco/task/shared/loop.h"

namespace peco {
namespace shared {

/**
 * @brief A Sharedtask cannot be created out of class loop
*/
task::task(std::weak_ptr<loop> wloop, task_id_t tid)
  :loop_(wloop), tid_(tid) { }

task::~task() {

}

/**
 * @brief Copy & Move
*/
task::task(const task& other) : loop_(other.loop_), tid_(other.tid_) {

}
task::task(task&& other) : loop_(std::move(other.loop_)), tid_(other.tid_) {
  
}
task& task::operator= (const task& other) {
  if (this == &other) return *this;
  loop_ = other.loop_;
  tid_ = other.tid_;
  return *this;
}
task& task::operator= (task&& other) {
  if (this == &other) return *this;
  loop_ = std::move(other.loop_);
  tid_ = other.tid_;
  return *this;
}

/**
 * @brief Check if the task ref is alive
*/
bool task::is_alive() const {
  bool ret = false;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      ret = peco::task(this->tid_).is_alive();
    });
  }
  return ret;
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
  TaskStatus status = kTaskStatusStopped;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      status = peco::task(tid_).status();
    });
  }
  return status;
}

/**
 * @brief Get the last waiting signal
*/
WaitingSignal task::signal() const {
  WaitingSignal signal = kWaitingSignalBroken;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      signal = peco::task(tid_).signal();
    });
  }
  return signal;
}

/**
 * @brief Set the exit function
 * @return the old exit function
*/
worker_t task::set_atexit(worker_t fn_exit) {
  worker_t old_worker;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      old_worker = peco::task(tid_).set_atexit(fn_exit);
    });
  }
  return old_worker;
}

/**
 * @brief If current task is been cancelled
*/
bool task::is_cancelled() const {
  bool ret = false;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      ret = peco::task(tid_).is_cancelled();
    });
  }
  return ret;
}

/**
 * @brief set arg pointer
*/
void task::set_arg(void *arg) {
  if (auto l = loop_.lock()) {
    l->sync_inject([=]() {
      peco::task(tid_).set_arg(arg);
    });
  }
}

/**
 * @brief Set flag at given index, max index is 16
*/
void task::set_flag(uint64_t flag, size_t index) {
  if (auto l = loop_.lock()) {
    l->sync_inject([=]() {
      peco::task(tid_).set_flag(flag, index);
    });
  }
}

/**
 * @brief Get flag at given index, if index is greater than 16, return -1
*/
uint64_t task::get_flag(size_t index) {
  uint64_t flag = 0llu;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      flag = peco::task(tid_).get_flag(index);
    });
  }
  return flag;
}

/**
 * @brief Set the task's name
*/
void task::set_name(const char* name) {
  if (auto l = loop_.lock()) {
    l->sync_inject([=]() {
      peco::task(tid_).set_name(name);
    });
  }
}

/**
 * @brief Get the task's name
*/
const char* task::get_name() const {
  const char * p_name = "";
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      p_name = peco::task(tid_).get_name();
    });
  }
  return "";
}

/**
 * @brief Get parent task
*/
peco::shared::task task::parent_task() const {
  task_id_t pid = kInvalidateTaskId;
  if (auto l = loop_.lock()) {
    l->sync_inject([&]() {
      pid = peco::task(tid_).parent_task().task_id();
    });
  }
  return task(loop_, kInvalidateTaskId);
}

/**
 * @brief Mask the task to be cancelled
 * all method will return immediately.
*/
void task::cancel() {
  if (auto l = loop_.lock()) {
    l->sync_inject([=]() {
      peco::task(tid_).cancel();
    });
  }
}
/**
 * @brief Update a loop task's interval
*/
void task::update_interval(duration_t interval) {
  if (auto l = loop_.lock()) {
    l->sync_inject([=]() {
      peco::task(tid_).update_interval(interval);
    });
  }
}

} // namespace shared
} // namespace peco

// Push Chen
