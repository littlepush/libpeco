/*
    signal.h
    libpeco
    2022-03-08
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

#ifndef PECO_SIGNAL_H__
#define PECO_SIGNAL_H__

#include "task/task.h"
#include "task/loop.h"

#include <list>

namespace peco {

class signal {
public:
  /**
   * @brief Init the signal
  */
  signal();
  /**
   * @brief Wakeup all pending task, with signal no recieived(timedout)
  */
  ~signal();

  /**
   * @brief Hold current task until other one give a signal
  */
  bool wait();

  /**
   * @brief Hold current task until other one give a signal or timedout
  */
  bool wait_until(duration_t timedout);

  /**
   * @brief Give a signal to the first waiting task
  */
  void trigger_one();

  /**
   * @brief Notify all pending task
  */
  void trigger_all();

  /**
   * @brief Force to cancel all pending task
  */
  void cancel();

protected:
  std::list<task_id_t> waiting_task_;
};

} // namespace peco

#endif

// Push Chen
