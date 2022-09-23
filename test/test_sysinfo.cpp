/*
    test_sysinfo.cpp
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

#include "peco/utils.h"
#include <thread>

int main(int argc, char *argv[]) {
  peco::log::info << "Current Process Name: " << peco::process_name() << std::endl;
  peco::log::info << "CPU Count: " << peco::cpu_count() << std::endl;
  peco::log::info << "Total Memory: " << peco::total_memory() << "B" << std::endl;
  peco::log::info << "Monitor For 10 Seconds: " << std::endl;
  // In Windows, it will need about 3-4 seconds to load data from system
  // so the data of the beginning should always be 0%
  for (size_t i = 0; i < 10; ++i) {
    peco::log::info << "-#" << (i + 1) << ": " << std::endl;
    peco::log::info << " Memory Usage: " << peco::memory_usage() << "B" << std::endl;
    peco::log::info << " Free Memory: " << peco::free_memory() << "B" << std::endl;
    peco::log::info << " App CPU Usage: " << peco::cpu_usage() * 100 << "%" << std::endl;
    auto _cpu_loads = peco::cpu_loads();
    for (size_t c = 0; c < _cpu_loads.size(); ++c) {
      peco::log::info << " Core #" << c << ": " << _cpu_loads[c] * 100 << "%" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}