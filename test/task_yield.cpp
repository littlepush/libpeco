/*
    task_yield.cpp
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

std::vector<int> running_order;

void task_1() {
  for ( int i = 0; i < 10; ++i ) {
    peco::log::debug << "[task_1]: " << i << std::endl;
    running_order.push_back(1);
    peco::task::this_task().yield();
  }
}

void task_2() {
  for ( int i = 0; i < 10; ++i ) {
    peco::log::debug << "[task_2]: " << i << std::endl;
    running_order.push_back(2);
    peco::task::this_task().yield();
  }
}

void task_3() {
  for ( int i = 0; i < 10; ++i ) {
    peco::log::debug << "[task_3]: " << i << std::endl;
    running_order.push_back(3);
    peco::task::this_task().yield();
  }
}

int main() {
  peco::current_loop::run(task_1);
  peco::current_loop::run(task_2);
  peco::current_loop::run(task_3);
  peco::ignore_result(peco::current_loop::main());
  assert(running_order.size() == 30);
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 3; ++j) {
      assert(running_order[i * 3 + j] == (j + 1));
    }
  }
  for (int i : running_order) {{
    std::cout << i << " ";
  }}
  std::cout << std::endl;
  return 0;
}

