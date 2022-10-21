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
  auto inner_task = impl->create_task(worker);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  impl->add_task(inner_task);
  return task(inner_task->task_id());
}
/**
 * @brief Start a loop task
*/
task loop::run_loop(worker_t worker, duration_t interval, const char* name) {
  auto inner_task = impl->create_task(worker, REPEAT_COUNT_INFINITIVE, interval);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  impl->add_task(inner_task);
  return task(inner_task->task_id());
}
/**
 * @brief Start a task after given <delay>
*/
task loop::run_delay(worker_t worker, duration_t delay, const char* name) {
  auto inner_task = impl->create_task(worker, REPEAT_COUNT_ONESHOT, delay);
  inner_task->set_name(name);
  // put the new task into the loop wrapper's timed list
  impl->add_task(inner_task);
  return task(inner_task->task_id());
}

/**
 * @brief Entrypoint of current loop, will block current thread
*/
int loop::main() {
  // invoke loop wrapper's main
  impl->main();
  // return the loop wrapper's exit code
  return impl->exit_code();
}
/**
 * @brief Get current loop's load average
*/
double loop::load_average() const {
  return impl->load_average();
}

/**
 * @brief Invoke in any task, which will case current loop to break and return from main
*/
void loop::exit(int code) {
  // tell the loop wrapper to break main loop with given code
  impl->exit(code);
}

/**
 * @brief Get current thread's shared loop object
*/
loop& loop::current() {
  static thread_local loop g_cur;
  return g_cur;
}

/**
 * @brief D'stor
*/
loop::~loop() {
  // default d'stor
  delete impl;
}

/**
 * @brief Not allow to create loop object
*/
loop::loop() {
  // initialize the thread localed loop wrapper
  impl = new loopimpl();
}


/**
 * @brief Current Loop API wrapper
*/
namespace current_loop {
/**
 * @brief Start a new normal task
*/
task run(worker_t worker, const char* name) {
  return loop::current().run(worker, name);
}
/**
 * @brief Start a loop task
*/
task run_loop(worker_t worker, duration_t interval, const char* name) {
  return loop::current().run_loop(worker, interval, name);
}
/**
 * @brief Start a task after given <delay>
*/
task run_delay(worker_t worker, duration_t delay, const char* name) {
  return loop::current().run_delay(worker, delay, name);
}
/**
 * @brief Entrypoint of current loop, will block current thread
*/
int main() {
  return loop::current().main();
}
/**
 * @brief Get current loop's load average
*/
double load_average() {
  return loop::current().load_average();
}
/**
 * @brief Invoke in any task, which will case current loop to break and return from main
*/
void exit(int code) {
  loop::current().exit(code);
}

} // namespace current_loop

} // namespace peco 

// Push Chen

