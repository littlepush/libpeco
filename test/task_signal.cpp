/*
    task_signal.cpp
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

#include "peco.h"

static peco::signal gSignal;

void pending_task() {
  if (gSignal.wait()) {
    peco::log::debug << "get signal!" << std::endl;
  } else {
    peco::log::error << "task has been cancelled" << std::endl;
  }
  peco::current_loop::exit(0);
}

void pending_task_until() {
  if (gSignal.wait_until(PECO_TIME_MS(1000))) {
    peco::log::error << "invalidate signal got" << std::endl;
  } else {
    peco::log::debug << "yes, no signal in 1000ms" << std::endl;
  }
  gSignal.trigger_one();
}

int main() {
  peco::current_loop::run(pending_task);
  peco::current_loop::run(pending_task_until);
  return peco::current_loop::main();
}
