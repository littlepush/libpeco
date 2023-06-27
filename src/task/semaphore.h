/*
    semaphore.h
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

#ifndef PECO_SEMPAPHORE_H__
#define PECO_SEMPAPHORE_H__

#include "task/loop.h"

#include <list>

namespace peco {

class semaphore {
public:
  /**
   * @brief Init the semaphore with count, default is an empty one
  */
  semaphore( uint32_t init_count = 0 );
  /**
   * @brief Wakeup all pending task, with signal no recieived(timedout)
  */
  ~semaphore();

  /**
   * @brief Hold current task until other one give a signal
  */
  bool fetch();

  /**
   * @brief Hold current task until other one give a signal or timedout
  */
  bool fetch_until(duration_t timedout);

  /**
   * @brief Give a signal and increase the count
  */
  void give();

  /**
   * @brief Force to cancel all pending task
  */
  void cancel();

protected:
  uint32_t count_ = 0;
  std::list<task_id_t> waiting_task_;
};

} // namespace peco

#endif

// Push Chen
