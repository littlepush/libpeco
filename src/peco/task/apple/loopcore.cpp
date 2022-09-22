/*
    loopcore.cpp
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

#include "peco/task/impl/loopcore.hxx"

// Include kqueue
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct kevent core_event_t;

#ifndef CO_MAX_SO_EVENTS
#define CO_MAX_SO_EVENTS 25600
#endif

namespace peco {
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
  core_fd_ = kqueue();
  core_vars_ = calloc(CO_MAX_SO_EVENTS, sizeof(core_event_t));
}

/**
 * @brief Block and wait for all fd
 */
void loopcore::wait(duration_t duration) {
  static const auto kNanoToSeconds = (1000 * 1000 * 1000);
  auto nano = duration.count();
  struct timespec ts = {nano / kNanoToSeconds, nano % kNanoToSeconds};
  auto begin = TASK_TIME_NOW();
  int count = kevent(core_fd_, NULL, 0, (core_event_t *)core_vars_,
                     CO_MAX_SO_EVENTS, &ts);
  time_waited_ += (TASK_TIME_NOW() - begin).count();
  
  // Already stopped
  if (count == -1) return;

  for (int i = 0; i < count; ++i) {
    core_event_t* process_event = reinterpret_cast<core_event_t*>(core_vars_) + i;
    fd_t fd = process_event->ident;

    auto flags = process_event->flags;
    if ((flags & EV_EOF) || (flags & EV_ERROR)) {
      if (on_error_) on_error_(fd);
    } else {
      EventType etype = (
        process_event->filter == EVFILT_READ ? 
        kEventTypeRead : kEventTypeWrite
      );
      if (on_event_)
        on_event_(fd, etype);
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
bool loopcore::add_read_event(fd_t fd) {
  core_event_t e;
  EV_SET(&e, fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
  return (-1 != kevent(core_fd_, &e, 1, NULL, 0, NULL));
}
void loopcore::del_read_event(fd_t fd) {
  core_event_t e;
  EV_SET(&e, fd, EVFILT_READ, EV_DELETE | EV_ONESHOT, 0, 0, NULL);
  ignore_result(kevent(core_fd_, &e, 1, NULL, 0, NULL));
}

/**
 * @brief Process the writing event
 */
bool loopcore::add_write_event(fd_t fd) {
  core_event_t e;
  EV_SET(&e, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
  return (-1 != kevent(core_fd_, &e, 1, NULL, 0, NULL));
}
void loopcore::del_write_event(fd_t fd) {
  core_event_t e;
  EV_SET(&e, fd, EVFILT_WRITE, EV_DELETE | EV_ONESHOT, 0, 0, NULL);
  ignore_result(kevent(core_fd_, &e, 1, NULL, 0, NULL));
}

/**
 * @brief Get all time cost on waiting
*/
uint64_t loopcore::get_wait_time() const {
  return time_waited_;
}


} // namespace peco

// Push Chen
