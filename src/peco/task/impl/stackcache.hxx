/*
    stackcache.h
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

#pragma once

#ifndef PECO_STACKCACHE_HXX
#define PECO_STACKCACHE_HXX

#include "peco/pecostd.h"
#include "peco/utils/bufguard.h"
#include "peco/task/impl/taskcontext.hxx"

#include <list>

namespace peco {

class stack_cache {
public:

  typedef bufguard<TASK_STACK_SIZE>           task_buffer_t;
  typedef std::shared_ptr<task_buffer_t>      task_buffer_ptr;

public:
  /**
   * @brief Fetch a freed stack buffer
  */
  task_buffer_ptr fetch();

  /**
   * @brief Release the stack buffer
  */
  void release(task_buffer_ptr buffer);

  /**
   * @brief Set the max free buffer count;
  */
  void set_cache_count(size_t cache_count);

  /**
   * @brief Get current cached buffer count
  */
  size_t free_count();
protected:
  size_t max_count_ = 32;
  std::list<task_buffer_ptr> cache_list_;
};

} // namespace peco

#endif

// Push Chen
