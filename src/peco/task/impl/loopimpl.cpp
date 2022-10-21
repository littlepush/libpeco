/*
    loopimpl.cpp
    libpeco
    2022-02-12
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

#include "peco/task/impl/loopimpl.hxx"

namespace peco {

/**
 * @brief Default C'str
*/
loopimpl::loopimpl() {

}
/**
 * @brief Tell if current loop is running
*/
bool loopimpl::is_running() const {
  return running_;
}

/**
 * @brief Entrypoint
*/
void loopimpl::main() {
  // Invoke base init(which is platform-related)
  this->init(
    [=](fd_t fd) {
      auto id_list = this->timed_list_.search_all(fd);
      // erase task related with this fd
      this->timed_list_.erase(fd);
      for (const auto& tid : id_list) {
        auto ptrt = this->find_task(tid);
        if (ptrt) {
          ptrt->get_task()->signal = kWaitingSignalBroken;
          ptrt->get_task()->next_fire_time = TASK_TIME_NOW();
          // Run this task in next loop
          this->timed_list_.insert(ptrt, nullptr);
        }
      }
    },
    [=](fd_t fd, EventType event_type) {
      auto id_list = this->timed_list_.search(fd, event_type);
      for (const auto& tid: id_list) {
        auto ptrt = this->find_task(tid);
        this->timed_list_.erase(ptrt, fd, event_type);
        if (ptrt) {
          ptrt->get_task()->signal = kWaitingSignalReceived;
          ptrt->get_task()->next_fire_time = TASK_TIME_NOW();
          // Run this task in next loop
          this->timed_list_.insert(ptrt, nullptr);
        }
      }
    });

  running_ = true;
  begin_time_ = TASK_TIME_NOW();
  while (this->alive_task_size() > 0) {
    while (this->timed_list_.size() > 0 && TASK_TIME_NOW() >= this->timed_list_.nearest_time()) {
      auto result = this->timed_list_.fetch();
      if (result.on_time) {
        result.on_time();
      }
      auto ptrt = this->find_task(result.tid);
      // Switch to the task
      this->swap_to_task(ptrt);
      if (ptrt->status() == kTaskStatusStopped) {
        this->del_task(ptrt->task_id());
      } else if (ptrt->status() == kTaskStatusPending) {
        this->timed_list_.insert(ptrt, nullptr);
      }
    }
    // after all timed task's executing, if there is no
    // cached task, stop the loop
    if (this->alive_task_size() == 0) break;

    // Someone stop the loop, should cancel all task
    if (!running_) {
      // Mark all task to be cancelled
      this->foreach([=](std::shared_ptr<basic_task> ptrt) {
        this->cancel(ptrt);
      });
      continue;
    }

    auto idle_gap = (this->timed_list_.size() > 0 ? 
      (this->timed_list_.nearest_time() - TASK_TIME_NOW()) : 
      PECO_TIME_MS(1000));
    if (idle_gap.count() < 0) continue;
    // wait fd event until idle_gap
    this->wait(idle_gap);
  }
  // force to stop if we break from last while loop
  this->stop();
}
/**
 * @brief Exit with given code
*/
void loopimpl::exit(int code) {
  exit_code_ = code;
  running_ = false;
  this->stop();
}
/**
 * @brief Get the load average of current loop
*/
double loopimpl::load_average() const {
  return 1.0 - ((double)this->get_wait_time() / (double)(TASK_TIME_NOW() - begin_time_).count());
}

/**
 * @brief Get the exit code
*/
int loopimpl::exit_code() const {
  return exit_code_;
}

/**
 * @brief Just add a task to the timed list
*/
void loopimpl::add_task(std::shared_ptr<basic_task> ptrt) {
  if (ptrt == nullptr) return;
  assert(ptrt->status() != kTaskStatusStopped);

  ptrt->get_task()->status = kTaskStatusPaused;
  this->timed_list_.insert(ptrt, nullptr);
}

/**
 * @brief Yield a task
*/
void loopimpl::yield_task(std::shared_ptr<basic_task> ptrt) {
  if (ptrt == nullptr) return;
  // only running task can be holded
  assert(ptrt->task_id() == this->running_task_->task_id());
  ptrt->get_task()->next_fire_time = TASK_TIME_NOW();
  ptrt->get_task()->status = kTaskStatusPaused;
  this->timed_list_.insert(ptrt, nullptr);
  this->swap_to_main();
}

/**
 * @brief Hold the task, put the task into never-check list
*/
void loopimpl::hold_task(std::shared_ptr<basic_task> ptrt) {
  if (ptrt == nullptr) return;
  // only running task can be holded
  assert(ptrt->task_id() == this->running_task_->task_id());
  // This task will only remain in basic task's cache,
  // unless someone keep the task_id and wakeup it.
  ptrt->get_task()->signal = kWaitingSignalNothing;
  // if the task has already been marked as canceled,
  // just return, not allowed to be holded.
  if (ptrt->get_task()->cancelled) {
    ptrt->get_task()->signal = kWaitingSignalBroken;
    return;
  }
  ptrt->get_task()->status = kTaskStatusPaused;
  this->swap_to_main();
}

/**
 * @brief Hold the task, wait until timedout
 * if waked up before timedout, the signal will be received
*/
void loopimpl::hold_task_for(std::shared_ptr<basic_task> ptrt, duration_t timedout) {
  if (ptrt == nullptr) return;
  // only running task can be holded
  assert(ptrt->task_id() == this->running_task_->task_id());
  ptrt->get_task()->signal = kWaitingSignalNothing;
  // if the task has already been marked as canceled,
  // just return, not allowed to be holded.
  if (ptrt->get_task()->cancelled) {
    ptrt->get_task()->signal = kWaitingSignalBroken;
    return;
  }
  ptrt->get_task()->next_fire_time = (TASK_TIME_NOW() + timedout);
  ptrt->get_task()->status = kTaskStatusPaused;
  this->timed_list_.insert(ptrt, nullptr);
  this->swap_to_main();
}

/**
 * @brief brief
*/
void loopimpl::wakeup_task(std::shared_ptr<basic_task> ptrt, WaitingSignal signal) {
  if (ptrt == nullptr) return;
  // Make the given task validate again
  assert(
    (!this->running_task_) ||
    (ptrt->task_id() != this->running_task_->task_id())
  );
  ptrt->get_task()->signal = signal;
  if (this->timed_list_.has(ptrt)) {
    this->timed_list_.replace_time(ptrt, TASK_TIME_NOW());
  } else {
    this->timed_list_.insert(ptrt, nullptr);
  }
}

/**
 * @brief Monitor the fd for reading event and put the task into
 * timed list with a timedout handler
*/
void loopimpl::wait_for_reading(fd_t fd, std::shared_ptr<basic_task> ptrt, duration_t timedout) {
  if (ptrt == nullptr) return;

  // if the task has already been marked as canceled,
  // just return, not allowed to be holded.
  if (ptrt->get_task()->cancelled) {
    ptrt->get_task()->signal = kWaitingSignalBroken;
    return;
  }
  ptrt->get_task()->signal = kWaitingSignalNothing;
  ptrt->get_task()->status = kTaskStatusPaused;
  ptrt->get_task()->next_fire_time = (TASK_TIME_NOW() + timedout);
  this->timed_list_.insert(ptrt, [=]() {
    this->timed_list_.erase(ptrt, fd);
    this->del_read_event(fd);
    if (ptrt->get_task()->cancelled) {
      ptrt->get_task()->signal = kWaitingSignalBroken;
    }
  }, fd, kEventTypeRead);
  this->add_read_event(fd);
  this->swap_to_main();
}

/**
 * @brief Monitor the fd for writing buffer and put the task into
 * timed list with a timedout handler
*/
void loopimpl::wait_for_writing(fd_t fd, std::shared_ptr<basic_task> ptrt, duration_t timedout) {
  if (ptrt == nullptr) return;
  // if the task has already been marked as canceled,
  // just return, not allowed to be holded.
  if (ptrt->get_task()->cancelled) {
    ptrt->get_task()->signal = kWaitingSignalBroken;
    return;
  }
  ptrt->get_task()->signal = kWaitingSignalNothing;
  ptrt->get_task()->status = kTaskStatusPaused;
  ptrt->get_task()->next_fire_time = (TASK_TIME_NOW() + timedout);
  this->timed_list_.insert(ptrt, [=]() {
    this->timed_list_.erase(ptrt, fd);
    this->del_write_event(fd);
    if (ptrt->get_task()->cancelled) {
      ptrt->get_task()->signal = kWaitingSignalBroken;
    }
  }, fd, kEventTypeWrite);
  this->add_write_event(fd);
  this->swap_to_main();
}

/**
 * @brief Cancel a repeatable or delay task
 * If a task is running, will wakeup and set the status to cancel
*/
void loopimpl::cancel(std::shared_ptr<basic_task> ptrt) {
  if (ptrt == nullptr) return;
  // The task is already been marked as cancelled, do nothing
  if (ptrt->get_task()->cancelled) return;
  ptrt->get_task()->cancelled = true;
  // Cancel self
  if (this->running_task_ == ptrt) {
    // Means the task is not in the timed_list and not holding
    // we dont need to do anything
    return;
  }

  // The task is just been holded, force to wakeup
  if (!this->timed_list_.has(ptrt)) {
    this->wakeup_task(ptrt, kWaitingSignalBroken);
  } else {
    // Mark the task's next fire time to now, force all pending
    // event to be timedout
    this->timed_list_.replace_time(ptrt, TASK_TIME_NOW());
  }
}

} // namespace peco
