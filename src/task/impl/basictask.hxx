/*
    basictask.h
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

#pragma once

#ifndef PECO_BASICTASK_HXX
#define PECO_BASICTASK_HXX

#include "task/impl/taskcontext.hxx"
#include "task/impl/stackcache.hxx"

namespace peco {

class basic_task : public std::enable_shared_from_this<basic_task> {
public:
  enum {
    kMaxFlagCount = 16
  };
private:
  /**
   * @brief Create a task with worker
  */
  basic_task(
    worker_t worker, repeat_count_t repeat_count = REPEAT_COUNT_ONESHOT, 
    duration_t interval = PECO_TIME_MS(0));

public:
  /**
   * @brief force to create a shared ptr task
  */
  static std::shared_ptr<basic_task> create_task(
    worker_t worker, repeat_count_t repeat_count = REPEAT_COUNT_ONESHOT, 
    duration_t interval = PECO_TIME_MS(0));

public:
  /**
   * @brief Release the task, and invoke `atexit`
  */
  ~basic_task();

  /**
   * @brief Destroy this task, will remove from global cache
  */
  void destroy_task();

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
   * @brief If the task is marked to be cancelled
  */
  bool cancelled() const;

  /**
   * @brief Set the exit function
   * @return the old exit function
  */
  worker_t set_atexit(worker_t fn_exit);

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
  task_id_t parent_task() const;

  /**
   * @brief Reset the task's status and ready for another loop
   * Only use this method in repeatable task
  */
  void reset_task();

  /**
   * @brief Get the internal task pointer
  */
  task_context_t* get_task();

  /**
   * @brief Update a loop task's interval
  */
  void update_interval(duration_t interval);

  /**
   * @brief Swap to current task
  */
  void swap_to_task();

  /**
   * @brief Swap to main context
  */
  static void swap_to_main();

  /**
   * @brief Get Current running task
  */
  static std::shared_ptr<basic_task>& running_task();
  
  /**
   * @brief Fetch the created task by its id
  */
  static std::shared_ptr<basic_task> fetch(task_id_t tid);

  /**
   * @brief Scan all cached task
  */
  static void foreach(std::function<void(std::shared_ptr<basic_task>)> handler);

  /**
   * @brief Get the cache task count
  */
  static size_t cache_size();
protected:
  stack_cache::task_buffer_ptr    buffer_;
  task_context_t*                 task_;
  task_extra_t*                   extra_;
};

} // namespace peco

#endif

// Push Chen
