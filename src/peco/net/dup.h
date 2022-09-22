/*
    dup.h
    libpeco
    2022-02-24
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

#pragma once

#ifndef PECO_NET_DUP_H__
#define PECO_NET_DUP_H__

#include "peco/task.h"
#include "peco/utils.h"

namespace peco {

template <typename _TyIncomingAdapter, typename _TyOutgoingAdapter>
void dup(std::shared_ptr<_TyIncomingAdapter> i, std::shared_ptr<_TyOutgoingAdapter> o) {
  auto i2o = current_loop::run([=]() {
    while(true) {
      auto d = i->read();
      if (d.status == kNetOpStatusFailed) {
        break;
      }
      if (d.status == kNetOpStatusTimedout) {
        continue;
      }
      o->write(d.data);
    }
  });
  auto o2i = current_loop::run([=](){
    while(true) {
      auto d = o->read();
      if (d.status == kNetOpStatusFailed) {
        break;
      }
      if (d.status == kNetOpStatusTimedout) {
        continue;
      }
      i->write(d.data);
    }
  });
  i2o.set_atexit([o2i]() {
    task(o2i).cancel();
  });
  o2i.set_atexit([i2o]() {
    task(i2o).cancel();
  });
}

} // namespace peco

#endif

// Push Chen
