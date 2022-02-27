/*
    depend.h
    libpeco
    2019-01-30
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

#ifndef PECO_DEPENDE_H__
#define PECO_DEPENDE_H__

#include "peco/pecostd.h"

#include <set>
#include <queue>
#include <map>

namespace peco {

template <typename T> struct dep_node_t {
  typedef typename std::shared_ptr<dep_node_t<T>> shared_dn_t;
  T item;
  std::set<shared_dn_t> r_dep;
  size_t reference_count;

  void __reference_depend(const T &root_item, std::set<shared_dn_t> &main_set) {
    for (auto _ri = r_dep.begin(); _ri != r_dep.end(); ++_ri) {
      assert((*_ri)->item != root_item && "Error: Cycle Dependency!");
      main_set.insert(*_ri);
      (*_ri)->__reference_depend(root_item, main_set);
    }
  }

  size_t ref_count() {
    std::set<shared_dn_t> _rset;
    this->__reference_depend(this->item, _rset);
    this->reference_count = _rset.size();
    return _rset.size();
  }
};

template <typename T>
std::shared_ptr<dep_node_t<T>> &operator<<(std::shared_ptr<dep_node_t<T>> &a,
                                           std::shared_ptr<dep_node_t<T>> &b) {
  // Set b as the target which a depends on.
  b->r_dep.insert(a);
  return a;
}

template <typename T, typename D>
std::shared_ptr<dep_node_t<T>>
operator*(std::map<std::shared_ptr<dep_node_t<T>>, D> &container,
          const T &item) {
  for (const auto &_pdkv : container) {
    if (_pdkv.first->item != item)
      continue;
    return _pdkv.first;
  }
  auto _p = std::make_shared<dep_node_t<T>>();
  _p->item = item;
  container[_p] = (D)0;
  return _p;
}

template <typename T>
std::vector<T> dep_order(const T &target_name,
                         std::function<std::vector<T>(const T &)> f_dep) {
  std::set<T> _targets_set;
  std::queue<T> _pending_targets;
  std::map<std::shared_ptr<dep_node_t<T>>, size_t> _result_map;

  _pending_targets.push(target_name);

  // Get all targets we need to process.
  while (_pending_targets.size() > 0) {
    // Fetch the first item in the queue.
    auto _t = _pending_targets.front();
    _pending_targets.pop();

    // Check if processed the item
    auto _ir = _targets_set.insert(_t);
    if (_ir.second == false) {
      // Already in set
      continue;
    }

    // Find if we already make the shared ptr
    auto _target_p = _result_map * _t;

    // Find the target's dependency, then add to the pending targets.
    auto _deps = f_dep(_t);
    for (const auto &_d : _deps) {
      _pending_targets.push(_d);

      // Build relationship
      auto _dt_p = _result_map * _d;
      _target_p << _dt_p;
    }
  }

  // Calculate the reference count
  for (auto &_pskv : _result_map) {
    _pskv.second = _pskv.first->ref_count();
  }

  std::vector<T> _result;
  for (auto &_t : _targets_set)
    _result.push_back(_t);
  std::sort(_result.begin(), _result.end(),
            [&](const T &a, const T &b) -> bool {
              auto _pa = _result_map * a;
              auto _pb = _result_map * b;
              return _pa->reference_count > _pb->reference_count;
            });
  return _result;
}
} // namespace peco

#endif

// Push Chen
