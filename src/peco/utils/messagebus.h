/*
    messagebus.h
    libpeco
    2023-05-04
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

#ifndef LIBPECO_UTILS_MESSAGEBUS_H__
#define LIBPECO_UTILS_MESSAGEBUS_H__

#include <functional>
#include <shared_mutex>
#include "peco/utils/rowmap.h"

namespace peco {

template <typename IdentityType, typename KeyType, typename ValueType>
class messagebus {
public:
  typedef std::function<void(const ValueType&)> event_handler_t;

  enum {
    IDENTITY_INDEX,
    KEY_INDEX
  };

public:

  void subscribe(const IdentityType& id, const KeyType& key, event_handler_t handler) {
    std::lock_guard<std::shared_mutex> _(handler_lock_);
    handler_map_.add(id, key, handler);
  }
  void unsubscribe(const IdentityType& id, const KeyType& key) {
    std::lock_guard<std::shared_mutex> _(handler_lock_);
    auto its = handler_map_.template find<IDENTITY_INDEX, KEY_INDEX>(id, key);
    for (auto& i : its) {
      handler_map_.erase(i);
    }
  }
  void unsubscribe(const IdentityType& id) {
    std::lock_guard<std::shared_mutex> _(handler_lock_);
    auto its = handler_map_.template find<IDENTITY_INDEX>(id);
    for (auto& i : its) {
      handler_map_.erase(i);
    }
  }
  void publish(const KeyType& key, const ValueType& value) {
    std::shared_lock<std::shared_mutex> _(handler_lock_);
    auto its = handler_map_.find<KEY_INDEX>(key);
    for (auto& i : its) {
      handler_map_.value_of(i)(value);
    }
  }

protected:
  std::shared_mutex handler_lock_;
  rowmap<IdentityType, KeyType, ValueType> handler_map_;
};

} // namespace peco

#endif

// Push Chen
