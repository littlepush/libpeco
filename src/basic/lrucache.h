/*
    lrucache.h
    libpeco
    2020-02-13
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

#ifndef PECO_LRUCACHE_H__
#define PECO_LRUCACHE_H__

#include "pecostd.h"
#include <unordered_map>

namespace peco {

template <typename _KeyType, typename _ValueType> class lrucache {
  typedef std::pair<_KeyType, _ValueType> item_type;
  typedef std::list<item_type> pool_type;
  typedef typename pool_type::iterator pool_iterator_type;
  typedef std::function<_ValueType(const _KeyType &)> fetch_action_type;

protected:
  pool_type item_pool_;
  std::unordered_map<_KeyType, pool_iterator_type> index_map_;
  size_t cache_size_;

public:
  lrucache() : cache_size_(10000) {}
  lrucache(const lrucache &r) : cache_size_(r.cache_size_) {
    item_pool_ = r.item_pool_;
    for (auto i = item_pool_.begin(); i != item_pool_.end(); ++i) {
      index_map_[i->first] = i;
    }
  }
  lrucache(lrucache &&r) : cache_size_(r.cache_size_) {
    item_pool_ = std::move(r.item_pool_);
    for (auto i = item_pool_.begin(); i != item_pool_.end(); ++i) {
      index_map_[i->first] = i;
    }
  }
  lrucache &operator=(const lrucache &r) {
    if (this == &r)
      return *this;
    cache_size_ = r.cache_size_;
    item_pool_ = r.item_pool_;
    for (auto i = item_pool_.begin(); i != item_pool_.end(); ++i) {
      index_map_[i->first] = i;
    }
    return *this;
  }
  lrucache &operator=(lrucache &&r) {
    if (this == &r)
      return *this;
    cache_size_ = r.cache_size_;
    item_pool_ = std::move(r.item_pool_);
    for (auto i = item_pool_.begin(); i != item_pool_.end(); ++i) {
      index_map_[i->first] = i;
    }
    return *this;
  }

  // Get the item and re-order the index
  _ValueType &get(const _KeyType &key, fetch_action_type fa = nullptr) {
    auto _it = index_map_.find(key);
    if (_it == index_map_.end()) {
      if (fa) {
        _ValueType _newfetch(fa(key));
        _it = index_map_.find(key);
        // Other task may already set the value during we fetch the object
        if (_it != index_map_.end())
          return _it->second->second;
        item_pool_.emplace_front(std::make_pair(key, std::move(_newfetch)));
      } else {
        // Just set an empty value
        _ValueType _empty_value;
        item_pool_.emplace_front(std::make_pair(key, std::move(_empty_value)));
      }
      // Create a new indexed info
      index_map_[key] = item_pool_.begin();

      // Clear, remove the last one
      while (item_pool_.size() > cache_size_) {
        auto _last = item_pool_.rbegin();
        index_map_.erase(_last->first);
        item_pool_.pop_back();
      }

      _it = index_map_.find(key);
    } else {
      // Re-order
      auto _item(std::move(*_it->second));
      item_pool_.erase(_it->second);
      item_pool_.emplace_front(std::move(_item));
      _it->second = item_pool_.begin();
    }
    return _it->second->second;
  }

  void put(const _KeyType &key, const _ValueType &value) { get(key) = value; }

  void erase(const _KeyType &key) {
    auto _it = index_map_.find(key);
    if (_it == index_map_.end())
      return;
    item_pool_.erase(_it->second);
    index_map_.erase(_it);
  }

  // Check if has the key, will not change the order
  bool contains(const _KeyType &key) {
    return index_map_.find(key) != index_map_.end();
  }

  _ValueType &operator[](const _KeyType &key) { return this->get(key); }

  // Clear all cache items
  void clear() {
    item_pool_.clear();
    index_map_.clear();
  }

  // Set the cache size, default is 10000
  size_t cache_size() const { return cache_size_; }
  void set_cache_size(size_t s) { cache_size_ = s; }

  // Get current cached count
  size_t size() const { return item_pool_.size(); }
};

} // namespace peco

#endif

// Push Chen
