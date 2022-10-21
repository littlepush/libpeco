/*
    loopcontext.h
    libpeco
    2022-10-21
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

#ifndef PECO_LOOPCONTEXT_HXX
#define PECO_LOOPCONTEXT_HXX

#include "peco/task/impl/taskcontext.hxx"
#include "peco/task/impl/stackcache.hxx"
#include "peco/task/impl/basictask.hxx"
#include "peco/task/impl/loopcore.hxx"

#include <unordered_map>

namespace peco {

class loopcontext : public loopcore {
public:
  virtual ~loopcontext();
  /**
   * @brief Get the main context
  */
  stack_context_t* get_main_context();

  /**
   * @brief create a task object
  */
  std::shared_ptr<basic_task> create_task(
    worker_t worker, repeat_count_t repeat_count = REPEAT_COUNT_ONESHOT, 
    duration_t interval = PECO_TIME_MS(0)
  );

  /**
   * @brief remove a task from the cache
  */
  void del_task(task_id_t tid);

  /**
   * @brief search and return the task
  */
  std::shared_ptr<basic_task> find_task(task_id_t tid);

  /**
   * @brief get alive task size
  */
  size_t alive_task_size() const;

  /**
   * @brief swap current context to the given task
  */
  void swap_to_task(std::shared_ptr<basic_task> task);

  /**
   * @brief swap back to main context
  */
  void swap_to_main();

  /**
   * @brief Scan all cached task
  */
  void foreach(std::function<void(std::shared_ptr<basic_task>)> handler);

protected: 
  task_context_t main_task_;
  std::unordered_map<task_id_t, std::shared_ptr<basic_task>> task_cache_;
  std::shared_ptr<basic_task> running_task_ = nullptr;
  stack_cache stack_cache_;
};

} // namespace peco

#endif

// Push Chen
