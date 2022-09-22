/*
    loopimpl.hxx
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

#pragma once

#ifndef PECO_LOOPIMPL_HXX
#define PECO_LOOPIMPL_HXX

#include "peco/task/impl/basictask.hxx"
#include "peco/task/impl/taskcontext.hxx"
#include "peco/task/impl/stackcache.hxx"
#include "peco/task/impl/tasklist.hxx"
#include "peco/task/impl/loopcore.hxx"

namespace peco {

class loopimpl : public loopcore {
public:
  /**
   * @brief Default C'str
  */
  loopimpl();

  /**
   * @brief Singleton loopimpl
  */
  static loopimpl& shared();

  /**
   * @brief Tell if current loop is running
  */
  bool is_running() const;

  /**
   * @brief Entrypoint
  */
  void main();

  /**
   * @brief Exit with given code
  */
  void exit(int code = 0);

  /**
   * @brief Get the exit code
  */
  int exit_code() const;

  /**
   * @brief Just add a task to the timed list
  */
  void add_task(std::shared_ptr<basic_task> ptrt);

  /**
   * @brief Yield a task
  */
  void yield_task(std::shared_ptr<basic_task> ptrt);

  /**
   * @brief Hold the task, put the task into never-check list
  */
  void hold_task(std::shared_ptr<basic_task> ptrt);

  /**
   * @brief Hold the task, wait until timedout
   * if waked up before timedout, the signal will be received
  */
  void hold_task_for(std::shared_ptr<basic_task> ptrt, duration_t timedout);

  /**
   * @brief brief
  */
  void wakeup_task(std::shared_ptr<basic_task> ptrt, WaitingSignal signal);

  /**
   * @brief Monitor the fd for reading event and put the task into
   * timed list with a timedout handler
  */
  void wait_for_reading(fd_t fd, std::shared_ptr<basic_task> ptrt, duration_t timedout);

  /**
   * @brief Monitor the fd for writing buffer and put the task into
   * timed list with a timedout handler
  */
  void wait_for_writing(fd_t fd, std::shared_ptr<basic_task> ptrt, duration_t timedout);

  /**
   * @brief Cancel a repeatable or delay task
   * If a task is running, will wakeup and set the status to cancel
  */
  void cancel(std::shared_ptr<basic_task> ptrt);

  /**
   * @brief Get the load average of current loop
  */
  double load_average() const;

protected:
  tasklist timed_list_;
  bool running_ = false;
  int exit_code_ = 0;
  task_time_t begin_time_;
};

} // namespace peco

#endif

// Push Chen
