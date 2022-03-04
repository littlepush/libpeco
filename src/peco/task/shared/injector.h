/*
    injector.h
    libpeco
    2022-02-15
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

#ifndef PECO_SHARED_INJECTOR_H__
#define PECO_SHARED_INJECTOR_H__

#include "peco/pecostd.h"
#include "peco/task/taskdef.h"

#include <atomic>

namespace peco {
namespace shared {

class injector {
public:
  /**
   * @brief Create a injector
  */
  injector();
  /**
   * @brief Destroy all injector related pipe
  */
  ~injector();

  /**
   * @brief No copy & move
  */
  injector(const injector&) = delete;
  injector(injector&&) = delete;
  injector& operator = (const injector&) = delete;
  injector& operator = (injector&&) = delete;

public:
  /**
   * @brief Disable current injector
  */
  void disable();

public:
  /**
   * @brief block current task/thread to inject the worker
  */
  bool sync_inject(worker_t worker) const;

  /**
   * @brief block current task/thread until the 'timedout'
  */
  bool inject_wait(worker_t worker, duration_t timedout) const;

  /**
   * @brief just inject the worker and ignore the response
  */
  void async_inject(worker_t worker) const;
protected:
  std::atomic<int> io_write_;
  task_id_t ij_task_;
};

} // namespace shared
} // namespace peco

#endif

// Push Chen
