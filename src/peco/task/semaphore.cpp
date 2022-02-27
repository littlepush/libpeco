/*
    semaphore.cpp
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

#include "peco/task/semaphore.h"

namespace peco {
/**
 * @brief Init the semaphore with count, default is an empty one
*/
semaphore::semaphore(uint32_t init_count) : count_(init_count) {}
/**
 * @brief Wakeup all pending task, with signal no recieived(timedout)
*/
semaphore::~semaphore() {
  this->cancel();
}

/**
 * @brief Hold current task until other one give a signal
*/
bool semaphore::fetch() {
  if (count_ > 0) {
    --count_;
    return true;
  }
  task _this_task = task::this_task();
  waiting_task_.push_back(_this_task.task_id());
  bool ret = _this_task.holding();
  auto it = std::find(waiting_task_.begin(), waiting_task_.end(), _this_task.task_id());
  if (it != waiting_task_.end()) {
    waiting_task_.erase(it);
  }
  return ret;
}

/**
 * @brief Hold current task until other one give a signal or timedout
*/
bool semaphore::fetch_until(duration_t timedout) {
  if (count_ > 0) {
    --count_;
    return true;
  }
  task _this_task = task::this_task();
  waiting_task_.push_back(_this_task.task_id());
  bool ret = _this_task.holding_until(timedout);
  auto it = std::find(waiting_task_.begin(), waiting_task_.end(), _this_task.task_id());
  if (it != waiting_task_.end()) {
    waiting_task_.erase(it);
  }
  return ret;
}

/**
 * @brief Give a signal and increase the count
*/
void semaphore::give() {
  while (waiting_task_.size() > 0) {
    auto tid = waiting_task_.front();
    waiting_task_.pop_front();
    task t(tid);
    if (!t.is_alive()) continue;
    t.wakeup();
    return;
  }
  count_ += 1;
}

/**
 * @brief Force to cancel all pending task
*/
void semaphore::cancel() {
  for (const auto tid : waiting_task_) {
    task t(tid);
    t.wakeup(kWaitingSignalNothing);
  }
  waiting_task_.clear();
}

} // namespace peco

// Push Chen
