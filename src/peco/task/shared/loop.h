/*
    loop.h
    libpeco
    2022-02-14
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

#ifndef PECO_SHARED_LOOP_H__
#define PECO_SHARED_LOOP_H__

#include "peco/pecostd.h"
#include "peco/task/loop.h"
#include "peco/task/shared/task.h"
#include "peco/task/shared/injector.h"

#include <thread>

namespace peco {
namespace shared {

class loop : public std::enable_shared_from_this<loop> {
protected:
  loop();
public:
  ~loop();

  /**
   * @brief Create a new shared loop
  */
  static std::shared_ptr<loop> create();

  /**
   * @brief Get the global context loop
  */
  static std::shared_ptr<loop> gcontext();

public:
  /**
   * @brief Post a 'run' command to the shared loop
  */
  peco::shared::task run(worker_t worker);

  /**
   * @brief Post a 'run_loop' command to the shared loop
  */
  peco::shared::task run_loop(worker_t worker, duration_t interval);

  /**
   * @brief Post a 'run_delay' command to the shared loop
  */
  peco::shared::task run_delay(worker_t worker, duration_t delay);

public:
  /**
   * @brief block current task/thread to inject the worker
  */
  bool sync_inject(worker_t worker);

  /**
   * @brief block current task/thread until the 'timedout'
  */
  bool inject_wait(worker_t worker, duration_t timedout);

  /**
   * @brief just inject the worker and ignore the response
  */
  void async_inject(worker_t worker);

public:
  /**
   * @brief post `exit` command to the shared loop
  */
  void exit(int code = 0);

protected:
  std::weak_ptr<injector> ij_;
  std::thread* lt_;
};

} // namespace shared
} // namespace peco

#endif

// Push Chen
