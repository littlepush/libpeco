/*
    loop.h
    libpeco
    2022-02-05
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

#ifndef PECO_LOOP_H__
#define PECO_LOOP_H__

#include "task/task.h"

namespace peco {

class loop : public std::enable_shared_from_this<loop> {
public:
  /**
   * @brief Start a new normal task
  */
  task run(worker_t worker, const char* name = nullptr);
  /**
   * @brief Start a loop task
  */
  task run_loop(worker_t worker, duration_t interval, const char* name = nullptr);
  /**
   * @brief Start a task after given <delay>
  */
  task run_delay(worker_t worker, duration_t delay, const char* name = nullptr);

public:
  /**
   * @brief Entrypoint of current loop, will block current thread
  */
  int main();

  /**
   * @brief Get current loop's load average
  */
  double load_average() const;

public:
  /**
   * @brief Invoke in any task, which will case current loop to break and return from main
  */
  void exit(int code = 0);

public:
  /**
   * @brief Get current thread's shared loop object
  */
  static std::shared_ptr<loop>& shared();

public:
  /**
   * @brief D'stor
  */
  ~loop();

protected:
  /**
   * @brief Not allow to create loop object
  */
  loop();
};

}

#endif

// Push Chen
