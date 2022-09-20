/*
    task_loop.cpp
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

int repeat_count = 0;
const int max_repeat_count = 10;

void peco_loop_task() {
  if (peco::task::this_task().is_cancelled()) {
    // Will never been invoked
    assert(0);
  }
  repeat_count += 1;
  peco::log::debug << "in loop: " << repeat_count << std::endl;
  if (repeat_count == max_repeat_count) {
    peco::task::this_task().cancel();
  }
}

int main() {
  peco::current_loop::run_loop(peco_loop_task, PECO_TIME_MS(10));
  peco::ignore_result(peco::current_loop::main());
  peco::log::debug << "repeat_count: " << repeat_count << ", max_repeat_count: " << max_repeat_count << std::endl;
  return 0;
}

