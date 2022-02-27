/*
    net_tcp.cpp
    libpeco
    2022-02-23
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

int main() {
  auto l = peco::shared::loop::create();
  l->run([]() {
    auto tl = peco::tcp_listener::create();
    tl->bind("0.0.0.0:12345");
    tl->listen([](std::shared_ptr<peco::tcp_connector> incoming) {
      auto d = incoming->read();
      peco::log::debug << "Incoming data: " << d.data << std::endl;
    });
  });

  l->run_delay([]() {
    auto c = peco::tcp_connector::create();
    c->connect("127.0.0.1:12345");
    c->write("hello peco", 10);
  }, PECO_TIME_S(1));

  l->sync_inject([]() {
    peco::task::this_task().sleep(PECO_TIME_S(2));
  });
  return 0;
}