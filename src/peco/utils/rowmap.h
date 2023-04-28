/*
    rowmap.h
    libpeco
    2023-04-28
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

#ifndef PECO_ROWMAP_H__
#define PECO_ROWMAP_H__

#include <list>
#include <map>
#include <tuple>
#include <array>
#include <memory>

namespace peco {

template <class... column_t>
class rowmap {
protected:
  /**
   * @brief Function to create a tuple with top I types
  */
  template <typename... T, std::size_t... I>
  static auto __deduce_make_tuple__(std::index_sequence<I...>) {
    using full_type = std::tuple<T...>;
    return std::make_tuple((typename std::tuple_element<I, full_type>::type){}...);
  }

  /**
   * @brief A Tuple type without the last type in list
  */
  template<class... T>
  using popback_tuple = decltype(__deduce_make_tuple__<T...>(std::make_integer_sequence<size_t, sizeof...(T) - 1>{}));

public:
  enum { 
    /**
     * @brief Index Type Count
    */
    index_type_count = std::tuple_size<std::tuple<column_t...>>::value - 1,
    /**
     * @brief Value item index in the tuple
    */
    value_index = index_type_count
  };

  /**
   * @brief Storage and index type
  */
  typedef std::tuple<column_t...> row_t;
  typedef popback_tuple<column_t...> index_t;
  typedef std::list<row_t> db_t;
  /**
   * @brief Row iterator
  */
  typedef typename db_t::iterator dbi_t;
  typedef typename db_t::iterator iterator;
  typedef typename db_t::const_iterator const_iterator;
  typedef typename db_t::reverse_iterator reverse_iterator;
  typedef typename db_t::const_reverse_iterator const_reverse_iterator;

  /**
   * @brief Value Type
  */
  typedef typename std::tuple_element<value_index, row_t>::type value_t;

  /**
   * @brief Index storage in index array
  */
  typedef std::shared_ptr<void> index_storage_t;

public:
  /**
   * @brief Key wrapper, use the key pointer in the db row, save memory
  */
  template <typename key_t>
  struct key_wrapper_t {
    /**
     * @brief Excplit c'str
    */
    key_wrapper_t(const key_t& k) : key_ptr(&k) {}
    /**
     * @brief Big 5
    */
    key_wrapper_t() : key_ptr(nullptr) {}
    key_wrapper_t(const key_wrapper_t& rhs) : key_ptr(rhs.key_ptr) {}
    key_wrapper_t(key_wrapper_t&& rhs) : key_ptr(std::move(rhs.key_ptr)) {}
    key_wrapper_t& operator = (const key_wrapper_t& rhs) {
      if (this == &rhs) return *this;
      key_ptr = rhs.key_ptr; return *this;
    }
    key_wrapper_t& operator = (key_wrapper_t&& rhs) {
      if (this == &rhs) return *this;
      key_ptr = std::move(rhs.key_ptr); return *this;
    }

    /**
     * @brief Pointer to origin key object
    */
    const key_t* key_ptr;

    /**
     * @brief Operator <, for key compare
    */
    bool operator <(const key_wrapper_t& rhs) const {
      return *key_ptr < *(rhs.key_ptr);
    }
  };

public:

  rowmap() {
    init_(std::make_integer_sequence<size_t, index_type_count>{});
  }

  rowmap(const rowmap& rhs) {
    init_(std::make_integer_sequence<size_t, index_type_count>{});
    *this = rhs;
  }

  rowmap(rowmap&& rhs) {
    init_(std::make_integer_sequence<size_t, index_type_count>{});
    *this = std::move(rhs);
  }

  rowmap& operator = (const rowmap& rhs) {
    if (this == &rhs) return *this;
    db_storage_ = rhs.db_storage_;
    for (auto it = db_storage_.begin(); it != db_storage_.end(); ++it) {
      build_index_(it, std::make_integer_sequence<std::size_t, index_type_count>{});
    }
    return *this;
  }

  rowmap& operator = (rowmap&& rhs) {
    if (this == &rhs) return *this;
    std::swap(db_storage_, rhs.db_storage_);
    std::swap(db_index_, rhs.db_index_);
    return *this;
  }

  /**
   * @brief Iterators of the db
  */
  iterator begin() noexcept { return db_storage_.begin(); }
  const_iterator begin() const noexcept { return db_storage_.begin(); }
  iterator end() noexcept { return db_storage_.end(); }
  const_iterator end() const noexcept { return db_storage_.end(); }
  const_iterator cbegin() { return db_storage_.cbegin(); }
  const_iterator cend() { return db_storage_.cend(); }

  reverse_iterator rbegin() noexcept { return db_storage_.rbegin(); }
  const_reverse_iterator rbegin() const noexcept { return db_storage_.rbegin(); }
  reverse_iterator rend() noexcept { return db_storage_.rend(); }
  const_reverse_iterator rend() const noexcept { return db_storage_.rend(); }
  const_reverse_iterator crbegin() { return db_storage_.crbegin(); }
  const_reverse_iterator crend() { return db_storage_.crend(); }

  /**
   * @brief add new row to the db
  */
  rowmap& add(const column_t&... args) {
    db_storage_.emplace_back(row_t(args...));
    build_index_(--db_storage_.end(), std::make_integer_sequence<std::size_t, index_type_count>{});
    return *this;
  }

  /**
   * @brief Emplace, move the args to the db
  */
  rowmap& emplace(column_t&&... args) {
    db_storage_.emplace_back(row_t(std::forward<column_t>(args)...));
    build_index_(--db_storage_.end(), std::make_integer_sequence<std::size_t, index_type_count>{});
    return *this;
  }

  /**
   * @brief Find the first row match the key, or end of db
  */
  template<std::size_t key_index>
  iterator find_first(const typename std::tuple_element<key_index, index_t>::type& key) {
    auto result = find_by_one_key_<key_index>(key);
    if (result.size() == 0) {
      return db_storage_.end();
    }
    return *result.begin();
  }

  /**
   * @brief Get all rows of the key in specified column
  */
  template<std::size_t... key_index>
  std::list<iterator> find(const typename std::tuple_element<key_index, index_t>::type&... key) {
    return merge_result_(find_by_one_key_<key_index>(key)...);
  }

  /**
   * @brief Erase the row and its keys
  */
  void erase(iterator it) {
    erase_index_(it, std::make_integer_sequence<std::size_t, index_type_count>{});
    db_storage_.erase(it);
  }

  /**
   * @brief Get the value parts of the iterator
   * if value_it is end(), undefined behaive.
  */
  value_t& value_of(iterator value_it) {
    return std::get<value_index>(*value_it);
  }

  /**
   * @brief Row count of the db
  */
  size_t size() const { return db_storage_.size(); }

  /**
   * @brief If is empty
  */
  bool empty() const { return db_storage_.empty(); }

  /**
   * @brief Remove all data in the storage
  */
  void clear() {
    db_storage_.clear();
    clear_index_(std::make_integer_sequence<std::size_t, index_type_count>{});
  }

protected:
  /**
   * @brief Init the index, for each type in index type
  */
  template <std::size_t... I>
  void init_(std::index_sequence<I...>) {
    using unused = int[];
    (void)unused{init_single_<typename std::tuple_element<I, index_t>::type>(I)...};
  }

  /**
   * @brief Build index for single key
  */
  template <class key_t>
  int init_single_(std::size_t col_index) {
    auto index_map = std::make_shared<std::multimap<key_wrapper_t<key_t>, dbi_t>>();
    db_index_[col_index] = std::static_pointer_cast<void>(index_map);
    return 0;
  }

  /**
   * @brief Build index for each row
  */
  template <std::size_t... I>
  void build_index_(dbi_t row, std::index_sequence<I...>) {
    using unused = int[];
    (void)unused{(build_index_<typename std::tuple_element<I, index_t>::type>(std::get<I>(*row), I, row), 0)...};
  }

  /**
   * @brief Build index for single key
  */
  template <class key_t>
  void build_index_(const key_t& key, std::size_t col_index, dbi_t row) {
    using index_map_t = std::multimap<key_wrapper_t<key_t>, dbi_t>;
    auto key_map = std::static_pointer_cast<index_map_t>(db_index_[col_index]);
    key_map->emplace(key_wrapper_t<key_t>(key), row);
  }

  /**
   * @brief Find rows by one key
  */
  template<std::size_t key_index>
  std::list<iterator> find_by_one_key_(const typename std::tuple_element<key_index, index_t>::type& key) {
    using key_t = typename std::tuple_element<key_index, index_t>::type;
    using index_map_t = std::multimap<key_wrapper_t<key_t>, dbi_t>;

    // get the index
    auto key_map = std::static_pointer_cast<index_map_t>(db_index_[key_index]);
    auto range = key_map->equal_range(key_wrapper_t<key_t>(key));

    std::list<iterator> r;
    for (auto i = range.first; i != range.second; ++i) {
      r.push_back(i->second);
    }
    return r;
  }

  /**
   * @brief Merge several key's find result
  */
  template<typename list1, typename... list_t>
  std::list<iterator> merge_result_(const list1& l1, const list_t&...ls) {
    return merge_result_(l1, merge_result_(ls...));
  }
  std::list<iterator> merge_result_(const std::list<iterator>& l1, const std::list<iterator>& l2) {
    std::list<iterator> r;
    for (auto i : l1) {
      for (auto j : l2) {
        if (i == j) {
          r.push_back(i);
        }
      }
    }
    return r;
  }
  std::list<iterator> merge_result_(const std::list<iterator>& l) {
    return l;
  }

  /**
   * @brief Erase all index
  */
  template <std::size_t... I>
  void erase_index_(dbi_t row, std::index_sequence<I...>) {
    using unused = int[];
    (void)unused{(erase_index_<typename std::tuple_element<I, index_t>::type>(std::get<I>(*row), I, row), 0)...};
  }

  /**
   * @brief Erase a single key with the row
  */
  template <class key_t>
  void erase_index_(const key_t& key, std::size_t col_index, dbi_t row) {
    using index_map_t = std::multimap<key_wrapper_t<key_t>, dbi_t>;
    auto key_map = std::static_pointer_cast<index_map_t>(db_index_[col_index]);
    auto fr = key_map->equal_range(key_wrapper_t<key_t>(key));
    auto e_it = key_map->end();
    for (auto i = fr.first; i != fr.second; ++i) {
      if (i->second == row) {
        e_it = i;
        break;
      }
    }
    if (e_it != key_map->end()) {
      key_map->erase(e_it);
    }
  }

  /**
   * @brief Clear all key-index
  */
  template <std::size_t... I>
  void clear_index_(std::index_sequence<I...>) {
    using unused = int[];
    (void)unused{(clear_index_<typename std::tuple_element<I, index_t>::type>(I), 0)...};
  }

  /**
   * @brief Clear single key-index
  */
  template <class key_t>
  void clear_index_(std::size_t col_index) {
    using index_map_t = std::multimap<key_wrapper_t<key_t>, dbi_t>;
    auto key_map = std::static_pointer_cast<index_map_t>(db_index_[col_index]);
    key_map->clear();
  }
  

protected:
  db_t db_storage_;
  std::array<index_storage_t, index_type_count> db_index_;
};

} // namespace peco

#endif 

// Push Chen
