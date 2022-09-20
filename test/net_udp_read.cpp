/*
    net_udp_read.cpp
    libpeco
    2022-03-06
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

void udp_read_test() {
  auto ul = peco::udp_listener::create();
  ul->bind("0.0.0.0:11111");
  ul->listen([](std::shared_ptr<peco::udp_connector> conn) {
    peco::task::this_task().sleep(PECO_TIME_S(3));
    for (int i = 0; i < 3; ++i) {
      auto r = conn->read();
      if (r.status == peco::kNetOpStatusTimedout) {
        break;
      }
      peco::log::debug << r.data << std::endl;
    }
    peco::current_loop::exit(0);
  });

  auto uc = peco::udp_connector::create();
  uc->connect("127.0.0.1:11111");
  uc->write("aaaaaaaaaa");
  uc->write("bbbbbbbbbb");
  uc->write("cccccccccc");
}

int main() {
  peco::current_loop::run(udp_read_test);
  peco::current_loop::main();
  return 0;
}
