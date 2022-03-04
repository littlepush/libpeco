/*
    task.h
    libpeco
    2022-02-09
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

#ifndef PECO_TASK_H__
#define PECO_TASK_H__

#include "peco/pecostd.h"
#include "peco/task/taskdef.h"

namespace peco {

/**
 * @brief A local task, only can be used in the thread which create it.
*/
class task {
public:
  /**
   * @brief Create a task by its id
  */
  task(task_id_t tid = kInvalidateTaskId);

  /**
   * @brief Copy & Move C'stor
  */
  task(const task& other);
  task(task&& other);
  /**
   * @brief Operator = & Operator move
  */
  task& operator = (const task& other);
  task& operator = (task&& other);

  /**
   * @brief Default D'stor
  */
  ~task() = default;

public:
  /**
   * @brief Check if the task ref is alive
  */
  bool is_alive() const;

public:
  /**
   * @brief Get this task object
  */
  static task this_task();

public:

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
   * @brief Get arg pointer
  */
  void * get_arg() const;

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
  task parent_task() const;

public:
  /**
   * @brief Hold current task, yield, but not put back to the timed based cache
  */
  bool holding();

  /**
   * @brief Hold the task until timedout
  */
  bool holding_until(duration_t timedout);

  /**
   * @brief Wakeup a holding task, cannot wakeup this_task
  */
  void wakeup(WaitingSignal signal = kWaitingSignalReceived);

  /**
   * @brief Mask the task to be cancelled
   * all method will return immediately.
  */
  void cancel();

  /**
   * @brief yield, force to switch task
  */
  void yield();

  /**
   * @brief wait event on specified fd's event
  */
  void wait_fd_for_event(long fd, EventType e, duration_t timedout);

  /**
   * @brief sleep current task then auto wakeup
  */
  void sleep(duration_t duration);

  /**
   * @brief Update a loop task's interval
  */
  void update_interval(duration_t interval);

protected:
  task_id_t tid_;
};

} // namespace peco

#endif

// Push Chen
