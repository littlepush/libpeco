/*
    net_udp_redirect
    libpeco
    2022-02-25
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

void begin_listen() {
  auto l = peco::udp_listener::create();
  l->bind("0.0.0.0:10053");
  l->listen([](std::shared_ptr<peco::udp_connector> incoming) {
    auto i = incoming->read();
    if (i.status == peco::kNetOpStatusFailed || i.status == peco::kNetOpStatusTimedout) {
      return;
    }
    auto outgoing = peco::udp_connector::create();
    outgoing->connect("192.168.1.1:53");
    outgoing->write(i.data);

    auto o = outgoing->read();
    if (o.status == peco::kNetOpStatusFailed || o.status == peco::kNetOpStatusTimedout) {
      return;
    }
    incoming->write(o.data);
  });
}

void begin_listen_packet() {
  auto l = peco::udp_listener::create();
  l->bind("0.0.0.0:10054");
  l->listen([](std::shared_ptr<peco::udp_packet> pkt) {
    peco::loop::shared()->run([=]() {
      auto outgoing = peco::udp_connector::create();
      outgoing->connect("192.168.1.1:53");
      outgoing->write(pkt->data());

      auto o = outgoing->read();
      if (o.status == peco::kNetOpStatusFailed || o.status == peco::kNetOpStatusTimedout) {
        return;
      }
      pkt->write(o.data);
    });
  });
}

int main() {
  peco::loop::shared()->run(begin_listen);
  peco::loop::shared()->run(begin_listen_packet);
  return peco::loop::shared()->main();
}