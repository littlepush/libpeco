/*
    task_hold.cpp
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

#include "peco.h"

bool hold_task_is_wakedup = false;

void task_sleep() {
  // This task will sleep 3 seconds
  peco::task::this_task().sleep(PECO_TIME_S(3));
  // after wakeup, exit the loop
  peco::current_loop::exit(0);
}

void task_hold() {
  // infinitive hold, until loop quit
  peco::task::this_task().holding();
  assert(peco::task::this_task().is_cancelled());
  hold_task_is_wakedup = true;
  peco::log::debug << "waked up and the task status must be cancelled" << std::endl;
}

void task_hold_until_and_wakeup() {
  bool been_waked_up = peco::task::this_task().holding_until(PECO_TIME_S(1));
  assert(been_waked_up == true);
  if (been_waked_up) {
    peco::log::debug << "task_hold_until_and_wakeup has been waked up" << std::endl;
  } else {
    peco::log::error << "task_hold_until_and_wakeup has not been waked up" << std::endl;
  }
}

void task_hold_until_and_timeout() {
  bool been_waked_up = peco::task::this_task().holding_until(PECO_TIME_S(1));
  assert(been_waked_up == false);
  if (!been_waked_up) {
    peco::log::debug << "task_hold_until_and_timeout has been timedout" << std::endl;
  } else {
    peco::log::error << "task_hold_until_and_timeout has been wakedup" << std::endl;
  }
}

void task_wakeup(peco::task t) {
  t.wakeup();
}

int main() {
  peco::task s_task = peco::current_loop::run(task_sleep);
  peco::current_loop::run(task_hold);
  peco::task hw_task = peco::current_loop::run(task_hold_until_and_wakeup);
  peco::current_loop::run(task_hold_until_and_timeout);
  peco::current_loop::run_delay([hw_task]() {
    task_wakeup(hw_task);
  }, PECO_TIME_MS(200));
  int wake_up_sleep_times = 0;
  peco::current_loop::run_loop([s_task, &wake_up_sleep_times]() {
    task_wakeup(s_task);
    wake_up_sleep_times += 1;
  }, PECO_TIME_MS(100));
  peco::ignore_result(peco::current_loop::main());
  assert(wake_up_sleep_times > 1);
  assert(hold_task_is_wakedup);
  peco::log::debug << "test done, please check the time with last log, should be almost the same" << std::endl;
  return 0;
}