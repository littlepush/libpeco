/*
    task.h
    libpeco
    2022-02-13
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

#pragma once

#ifndef PECO_SHARED_TASK_H__
#define PECO_SHARED_TASK_H__

#include "task/task.h"

namespace peco {
namespace shared {
class loop;

class task {

protected:
  /**
   * @brief Make loop as the friend class of task
  */
  friend class loop;
  /**
   * @brief A task cannot be created out of class loop
  */
  task(std::weak_ptr<loop> wloop, task_id_t tid);

public:
  ~task();

  /**
   * @brief Copy & Move
  */
  task(const peco::shared::task& other);
  task(peco::shared::task&& other);
  peco::shared::task& operator= (const peco::shared::task& other);
  peco::shared::task& operator= (peco::shared::task&& other);

public:
  /**
   * @brief Check if the task ref is alive
  */
  bool is_alive() const;

  /**
   * @brief Get the task id
  */
  task_id_t task_id() const;

  /**
   * @brief Get the task's status
  */
  TaskStatus status() const;

  /**
   * @brief Get the last waiting signal
  */
  WaitingSignal signal() const;

  /**
   * @brief Set the exit function
   * @return the old exit function
  */
  worker_t set_atexit(worker_t fn_exit);

  /**
   * @brief If current task is been cancelled
  */
  bool is_cancelled() const;

  /**
   * @brief set arg pointer
  */
  void set_arg(void *arg);

  /**
   * @brief Set flag at given index, max index is 16
  */
  void set_flag(uint64_t flag, size_t index);

  /**
   * @brief Get flag at given index, if index is greater than 16, return -1
  */
  uint64_t get_flag(size_t index);

  /**
   * @brief Set the task's name
  */
  void set_name(const char* name);

  /**
   * @brief Get the task's name
  */
  const char* get_name() const;

  /**
   * @brief Get parent task
  */
  peco::shared::task parent_task() const;

  /**
   * @brief Mask the task to be cancelled
   * all method will return immediately.
  */
  void cancel();
  
  /**
   * @brief Update a loop task's interval
  */
  void update_interval(duration_t interval);

protected:
  std::weak_ptr<loop> loop_;
  task_id_t tid_;
};

} // namespace shared
} // namespace peco

#endif

// Push Chen
