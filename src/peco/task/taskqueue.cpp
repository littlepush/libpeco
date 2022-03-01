/*
    taskqueue.cpp
    libpeco
    2022-02-17
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

#include "peco/task/taskqueue.h"

namespace peco {

/**
 * @brief Create a task queue
*/
std::shared_ptr<taskqueue> taskqueue::create() {
  return std::shared_ptr<taskqueue>(new taskqueue);
}

taskqueue::taskqueue() {
  inner_t_ = loop::shared()->run([&]() {
    while( !task::this_task().is_cancelled() && this->sem_.fetch() ) {
      auto w = pending_task_list_.front();
      pending_task_list_.pop_front();
      w();
    }
  });
}
/**
 * @brief Remove all runing worker, and cancel current running jon
*/
taskqueue::~taskqueue() {
  inner_t_.cancel();
}

/**
 * @brief Add new task to the end of the queue
*/
void taskqueue::append(worker_t worker) {
  pending_task_list_.emplace_back(std::move(worker));
  sem_.give();
}

/**
 * @brief Insert new task to the header of the queue
*/
void taskqueue::insert(worker_t worker) {
  pending_task_list_.emplace_front(std::move(worker));
  sem_.give();
}

/**
  * @brief Sync the invocation, wait until the worker has been done
*/
void taskqueue::sync(worker_t worker) {
  task this_task = task::this_task();
  this->append([worker, this_task]() {
    worker();
    task(this_task).wakeup();
  });
  this_task.holding();
}

} // namespace peco

// Push Chen
