/*
    taskqueue.h
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

#ifndef PECO_TASKQUEUE_H__
#define PECO_TASKQUEUE_H__

#include "peco/task/loop.h"
#include "peco/task/semaphore.h"

#include <list>

namespace peco {

class taskqueue : public std::enable_shared_from_this<taskqueue> {
protected:
  taskqueue();
public:

  /**
   * @brief Create a task queue
  */
  static std::shared_ptr<taskqueue> create();

  /**
   * @brief Remove all runing worker, and cancel current running jon
  */
  ~taskqueue();

  /**
   * @brief Add new task to the end of the queue
  */
  void append(worker_t worker);

  /**
   * @brief Insert new task to the header of the queue
  */
  void insert(worker_t worker);

  /**
   * @brief Sync the invocation, wait until the worker has been done
  */
  void sync(worker_t worker);

protected:
  task inner_t_;
  semaphore sem_;
  std::list<worker_t> pending_task_list_;
};

} // namespace peco

#endif

// Push Chen
