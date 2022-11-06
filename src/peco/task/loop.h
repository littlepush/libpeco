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

#include "peco/task/task.h"
#include <thread>

namespace peco {

/**
 * @brief pre-define of the impl
*/
class loopimpl;

class loop {
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
  static loop& current();

  /**
   * @brief Check if current thread is running in a loop
  */
  static bool running_in_loop();

  /**
   * @brief Start in main thread, throw exception when current thread is not main thread
  */
  static int start_main_loop(worker_t entrance_pointer);

  /**
   * @brief Create a new loop(with new thread)
  */
  static loop* creeate_loop();

  /**
   * @brief Delete a loop and cancel all pending task, then destroy the inner thread
  */
  static void destroy_loop(loop* lp);

public:
  /**
   * @brief D'stor
  */
  ~loop();

protected:
  /**
   * @brief Entrypoint of current loop, will block current thread
  */
  int main();

  /**
   * @brief Not allow to create loop object
  */
  loop();

  /**
   * @brief Start to listen cross-thread event
  */
  void start_event_listen();

  /**
   * @brief Stop the cross-thread event listener
  */
  void stop_event_listen();

  /**
   * @brief internal impl
  */
  loopimpl* impl = nullptr;

  /**
   * @brief binding thread
  */
  std::thread::id tid_;
};

/**
 * @brief Current Loop API wrapper
*/
namespace current_loop {
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
/**
 * @brief Get current loop's load average
*/
double load_average();
/**
 * @brief Invoke in any task, which will case current loop to break and return from main
*/
void exit(int code = 0);

} // namespace current_loop

/**
 * @brief Get the main thread loop
*/
namespace main_loop {
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
/**
 * @brief Get current loop's load average
*/
double load_average();
/**
 * @brief Invoke in any task, which will case current loop to break and return from main
*/
void exit(int code = 0);

} // namespace main_loop

}

#endif

// Push Chen
