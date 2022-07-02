/*
    loop.cpp
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

#include "peco/task/loop.h"
#include "peco/task/impl/basictask.hxx"
#include "peco/task/impl/loopimpl.hxx"

namespace peco {

/**
 * @brief Start a new normal task
*/
task loop::run(worker_t worker, const char* name) {
  auto inner_task = basic_task::create_task(worker);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  loopimpl::shared().add_task(inner_task);
  return task(inner_task->task_id());
}
/**
 * @brief Start a loop task
*/
task loop::run_loop(worker_t worker, duration_t interval, const char* name) {
  auto inner_task = basic_task::create_task(worker, REPEAT_COUNT_INFINITIVE, interval);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  loopimpl::shared().add_task(inner_task);
  return task(inner_task->task_id());
}
/**
 * @brief Start a task after given <delay>
*/
task loop::run_delay(worker_t worker, duration_t delay, const char* name) {
  auto inner_task = basic_task::create_task(worker, REPEAT_COUNT_ONESHOT, delay);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  loopimpl::shared().add_task(inner_task);
  return task(inner_task->task_id());
}

/**
 * @brief Entrypoint of current loop, will block current thread
*/
int loop::main() {
  // invoke loop wrapper's main
  loopimpl::shared().main();
  // return the loop wrapper's exit code
  return loopimpl::shared().exit_code();
}
/**
 * @brief Get current loop's load average
*/
double loop::load_average() const {
  return loopimpl::shared().load_average();
}

/**
 * @brief Invoke in any task, which will case current loop to break and return from main
*/
void loop::exit(int code) {
  // tell the loop wrapper to break main loop with given code
  loopimpl::shared().exit(code);
}

/**
 * @brief Get current thread's shared loop object
*/
std::shared_ptr<loop>& loop::shared() {
  thread_local static std::shared_ptr<loop> s_loop(new loop);
  return s_loop;
}

/**
 * @brief D'stor
*/
loop::~loop() {
  // default d'stor
}

/**
 * @brief Not allow to create loop object
*/
loop::loop() {
  // initialize the thread localed loop wrapper
}

} // namespace peco 

// Push Chen

