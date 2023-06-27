/*
    loopcore.cpp
    libpeco
    2022-02-17
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

#include "task/impl/loopcore.hxx"
#include <map>

#include <sys/syscall.h>
#include <sys/signal.h>
#include <unistd.h>
#define gettid()    syscall(__NR_gettid)

#ifndef FAR
  #define FAR
#endif

// include epoll
#include <sys/epoll.h>
typedef struct epoll_event  core_event_t;

#ifndef CO_MAX_SO_EVENTS
#define CO_MAX_SO_EVENTS 25600
#endif

namespace peco {

typedef std::map<long, uint8_t> cached_fd_event_t;

#define EPOLL_FD_NO_EVENT       (uint8_t)0
#define EPOLL_FD_READ_EVENT     (uint8_t)0x01
#define EPOLL_FD_NO_READ_EVENT  (uint8_t)0xFE
#define EPOLL_FD_WRITE_EVENT    (uint8_t)2
#define EPOLL_FD_NO_WRITE_EVENT (uint8_t)0xFD

#define __MARK_READ__(x)      (x) & EPOLL_FD_READ_EVENT
#define __MARK_WRITE__(x)     (x) & EPOLL_FD_WRITE_EVENT
#define __UNMARK_READ__(x)    (x) & EPOLL_FD_NO_READ_EVENT
#define __UNMARK_WRITE__(x)   (x) & EPOLL_FD_NO_WRITE_EVENT

inline int __core_event_ctl__(int core_fd, int so, uint32_t flag, int eid) {
  core_event_t e;
  memset(&e, 0, sizeof(e));
  e.data.fd = so;
  e.events = flag;
  if (-1 == epoll_ctl(core_fd, eid, so, &e)) {
    if (errno == EEXIST) {
      return epoll_ctl(core_fd, EPOLL_CTL_MOD, so, &e);
    } else {
      return -1;
    }
  } else {
    return 0;
  }
}

/**
 * @brief Init the core fd(if any) and bind the error and event handler
 */
void loopcore::init(loopcore::core_error_handler_t herr,
                    loopcore::core_event_handler_t hevent
                    ) 
{
  on_error_ = herr;
  on_event_ = hevent;

  signal(SIGPIPE, SIG_IGN);

  // Create kqueue core fd
  core_fd_ = epoll_create1(0);
  core_vars_ = calloc(CO_MAX_SO_EVENTS, sizeof(core_event_t));
  core_data_ = cached_fd_event_t();
}

/**
 * @brief Block and wait for all fd
 */
void loopcore::wait(duration_t duration) {
  cached_fd_event_t* p_cache = any_cast<cached_fd_event_t>(&core_data_);

  auto begin = TASK_TIME_NOW();
  int count = epoll_wait(
    core_fd_, (core_event_t *)core_vars_, CO_MAX_SO_EVENTS, 
    std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
  time_waited_ += (TASK_TIME_NOW() - begin).count();
  
  // Already stopped
  if (count == -1) return;

  for (int i = 0; i < count; ++i) {
    core_event_t* process_event = reinterpret_cast<core_event_t*>(core_vars_) + i;
    long fd = process_event->data.fd;

    // Check if is on error
    auto c_it = p_cache->find(fd);
    if ((process_event->events & EPOLLHUP) || (process_event->events & EPOLLERR)) {
      if (c_it != p_cache->end()) {
        uint32_t events = 0;
        if (c_it->second & EPOLL_FD_READ_EVENT) {
          events &= EPOLLIN;
        }
        if (c_it->second & EPOLL_FD_WRITE_EVENT) {
          events &= EPOLLOUT;
        }
        // remove the event
        __core_event_ctl__(core_fd_, fd, events, EPOLL_CTL_DEL);
        // remove from cache
        p_cache->erase(c_it);
      }
      if (on_error_) on_error_(fd);
    } else {
      if ((process_event->events & EPOLLIN) && on_event_) {
        if (c_it != p_cache->end()) {
          c_it->second = __UNMARK_READ__(c_it->second);
        }
        on_event_(fd, kEventTypeRead);
      }
      if ((process_event->events & EPOLLOUT) && on_event_) {
        if (c_it != p_cache->end()) {
          c_it->second = __UNMARK_WRITE__(c_it->second);
        }
        on_event_(fd, kEventTypeWrite);
      }
      // Put the un reported event back to the epoll list
      // or just remove the cache
      if (c_it != p_cache->end()) {
        if (c_it->second == EPOLL_FD_NO_EVENT) {
          p_cache->erase(c_it);
        } else if (c_it->second == EPOLL_FD_READ_EVENT) {
          __core_event_ctl__(core_fd_, fd, EPOLLIN | EPOLLET, EPOLL_CTL_ADD);
        } else if (c_it->second == EPOLL_FD_WRITE_EVENT) {
          __core_event_ctl__(core_fd_, fd, EPOLLOUT | EPOLLET, EPOLL_CTL_ADD);
        } else {
          assert(false);
        }
      }
    }
  }
}

/**s
 * @brief Break the waiting
 */
void loopcore::stop() {
  // Already stopped
  if (core_fd_ == -1) return;
  
  close(core_fd_);
  core_fd_ = -1;
  if (core_vars_ != nullptr) {
    free(core_vars_);
    core_vars_ = nullptr;
  }
}

/**
 * @brief Process the reading event
 */
bool loopcore::add_read_event(long fd) {
  cached_fd_event_t* p_cache = any_cast<cached_fd_event_t>(&core_data_);
  auto c_it = p_cache->find(fd);
  // Already monite on read event
  if (c_it != p_cache->end() && (c_it->second & EPOLL_FD_READ_EVENT)) {
    return false;
  }
  uint32_t event = EPOLLIN | EPOLLET;
  if (c_it != p_cache->end() && (c_it->second & EPOLL_FD_WRITE_EVENT)) {
    event |= EPOLLOUT;
  }
  c_it->second = __MARK_READ__(c_it->second);
  return (__core_event_ctl__(core_fd_, fd, event, EPOLL_CTL_ADD) > 0);
}
void loopcore::del_read_event(long fd) {
  cached_fd_event_t* p_cache = any_cast<cached_fd_event_t>(&core_data_);
  auto c_it = p_cache->find(fd);
  // not monited
  if (c_it == p_cache->end()) return;
  __core_event_ctl__(core_fd_, fd, EPOLLIN, EPOLL_CTL_DEL);
  c_it->second = __UNMARK_READ__(c_it->second);
  if (c_it->second == EPOLL_FD_NO_EVENT) {
    p_cache->erase(c_it);
  }
}

/**
 * @brief Process the writing event
 */
bool loopcore::add_write_event(long fd) {
  cached_fd_event_t* p_cache = any_cast<cached_fd_event_t>(&core_data_);
  auto c_it = p_cache->find(fd);
  // Already monite on write event
  if (c_it != p_cache->end() && (c_it->second & EPOLL_FD_WRITE_EVENT)) {
    return false;
  }
  uint32_t event = EPOLLOUT | EPOLLET;
  if (c_it != p_cache->end() && (c_it->second & EPOLL_FD_READ_EVENT)) {
    event |= EPOLLIN;
  }
  c_it->second = __MARK_WRITE__(c_it->second);
  return (__core_event_ctl__(core_fd_, fd, event, EPOLL_CTL_ADD) > 0);
}
void loopcore::del_write_event(long fd) {
  cached_fd_event_t* p_cache = any_cast<cached_fd_event_t>(&core_data_);
  auto c_it = p_cache->find(fd);
  // not monited
  if (c_it == p_cache->end()) return;
  __core_event_ctl__(core_fd_, fd, EPOLLOUT, EPOLL_CTL_DEL);
  c_it->second = __UNMARK_WRITE__(c_it->second);
  if (c_it->second == EPOLL_FD_NO_EVENT) {
    p_cache->erase(c_it);
  }
}

/**
 * @brief Get all time cost on waiting
*/
uint64_t loopcore::get_wait_time() const {
  return time_waited_;
}

} // namespace peco

// Push Chen
