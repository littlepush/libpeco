/*
    loopcore.hxx
    libpeco
    2022-02-12
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

#ifndef PECO_TASK_LOOPCORE_HXX
#define PECO_TASK_LOOPCORE_HXX

#include "pecostd.h"
#include "task/taskdef.h"
#include "basic/any.h"

namespace peco {

class loopcore {
public:
  typedef std::function<void(long)> core_error_handler_t;
  typedef std::function<void(long, EventType)> core_event_handler_t; 

public :
  /**
   * @brief Init the core fd(if any) and bind the error and event handler
   */
  void init(core_error_handler_t herr, core_event_handler_t hevent);

  /**
   * @brief Block and wait for all fd
  */
  void wait(duration_t duration);

  /**
   * @brief Break the waiting
  */
  void stop();

  /**
   * @brief Process the reading event
  */
  bool add_read_event(long fd);
  void del_read_event(long fd);

  /**
   * @brief Process the writing event
  */
  bool add_write_event(long fd);
  void del_write_event(long fd);

  /**
   * @brief Get all time cost on waiting
  */
  uint64_t get_wait_time() const;

protected:
  int core_fd_ = -1;
  void *core_vars_ = nullptr;
  // Any platform related data  
  any core_data_;
  uint64_t time_waited_ = 0;

  /**
   * @brief Event Handlers
  */
  core_error_handler_t on_error_;
  core_event_handler_t on_event_;
};
} // namespace peco

#endif

// Push Chen
