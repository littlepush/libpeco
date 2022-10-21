/*
    stackcache.cpp
    libpeco
    2022-02-04
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

#include "peco/task/impl/stackcache.hxx"

#include <list>

namespace peco {

/**
 * @brief Fetch a freed stack buffer
*/
stack_cache::task_buffer_ptr stack_cache::fetch() {
  if (cache_list_.size() > 0) {
    auto buf = cache_list_.front();
    cache_list_.pop_front();
    return buf;
  } else {
    return std::make_shared<stack_cache::task_buffer_t>();
  }
}

/**
 * @brief Release the stack buffer
*/
void stack_cache::release(stack_cache::task_buffer_ptr buffer) {
  if (cache_list_.size() >= max_count_) {
    return;
  } else {
    cache_list_.push_back(buffer);
  }
}

/**
 * @brief Set the max free buffer count;
*/
void stack_cache::set_cache_count(size_t cache_count) {
  max_count_ = cache_count;
}

/**
 * @brief Get current cached buffer count
*/
size_t stack_cache::free_count() {
  return cache_list_.size();
}

} // namespace peco

// Push Chen
