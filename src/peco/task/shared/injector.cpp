/*
    injector.cpp
    libpeco
    2022-02-16
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

#include "peco/task/shared/injector.h"
#include "peco/task/impl/basictask.hxx"
#include "peco/task/impl/loopimpl.hxx"
#include "peco/utils/logs.h"

namespace peco {
namespace shared {

/**
 * @brief Wrapper a pipe info
*/
typedef struct {
  int                       p[2];
} pipe_wrapper_t;

/**
 * @brief Injector Info to transfer
*/
typedef struct {
  /**
   * @brief The user worker need to run
  */
  worker_t *  p_worker;
  /**
   * @brief When the injection will be timedout
  */
  int64_t     timedout;
  /**
   * @brief The fd to write the finish signal
  */
  int         io_out;
} InjectorInfo;

/**
 * @brief Create a injector
*/
injector::injector() {
  int io_pipe[2] = {-1, -1};
  ignore_result(pipe(io_pipe));
  int io_read = io_pipe[0];
  io_write_ = io_pipe[1];

  auto t = basic_task::create_task([io_read]() {
    auto this_task = basic_task::running_task();
    InjectorInfo ij;
    while(this_task->signal() != kWaitingSignalBroken) {
      loopimpl::shared().wait_for_reading(io_read, this_task, PECO_TIME_S(1));
      if (this_task->signal() == kWaitingSignalNothing) {
        // timedout, no incoming command, continue
        continue;
      }
      if (this_task->signal() == kWaitingSignalReceived) {
        // have incoming command, process it
        int ret = read(io_read, &ij, sizeof(ij));
        auto work_task = basic_task::create_task([=](){
          if (ij.p_worker) {
            (*ij.p_worker)();
            delete ij.p_worker;
          }
          if (ij.io_out != -1 && (ij.timedout == -1 || TASK_TIME_NOW().time_since_epoch().count() < ij.timedout)) {
            // finished running
            ignore_result(write(ij.io_out, &ret, sizeof(int)));
          }
        });
        loopimpl::shared().add_task(work_task);
      }
    }
    // we are cancelled, not broken pipe
    if (this_task->cancelled()) {
      int ret = 0;
      fd_set fs;
      while (ret == 0) {
        struct timeval tv = {0, 0};
        do {
          FD_ZERO(&fs);
          FD_SET(io_read, &fs);
          ret = select(io_read + 1, &fs, NULL, NULL, &tv);
        } while (ret < 0 && errno == EINTR);
        assert(ret >= 0);
        if (ret == 0) {
          // no more pending task, safe exit
          break;
        }
        int l = read(io_read, &ij, sizeof(ij));
        if (ij.io_out != -1 && (ij.timedout == -1 || TASK_TIME_NOW().time_since_epoch().count() < ij.timedout)) {
          // finished running, -1 means cancelled
          if (ij.p_worker) {
            delete ij.p_worker;
          }
          l = -1;
          ignore_result(write(ij.io_out, &l, sizeof(int)));
        }
      }
    }
    // close the pipe
    close(io_read);
  });
  ij_task_ = t->task_id();
  loopimpl::shared().add_task(t);
}
/**
 * @brief Destroy all injector related pipe
*/
injector::~injector() {
  this->disable();
}
/**
  * @brief Disable current injector
*/
void injector::disable() {
  if (io_write_ == -1) return;
  // cancel ij task
  auto ij_tid = ij_task_;
  ij_task_ = kInvalidateTaskId;
  this->async_inject([ij_tid]() {
    auto t = basic_task::fetch(ij_tid);
    loopimpl::shared().cancel(t);
  });
  // close write pipe
  int w = io_write_;
  io_write_ = -1;
  close(w);
}

/**
 * @brief block current task/thread to inject the worker
*/
bool injector::sync_inject(worker_t worker) const {
  std::shared_ptr<pipe_wrapper_t> ij_io(new pipe_wrapper_t, [](pipe_wrapper_t * pw) {
    if (pw->p[0] != -1) {
      close(pw->p[0]);
    }
    if (pw->p[1] != -1) {
      close(pw->p[1]);
    }
    delete pw;
  });
  ignore_result(pipe(ij_io->p));

  InjectorInfo ij;
  ij.p_worker = new worker_t(worker);
  ij.timedout = -1;
  ij.io_out = ij_io->p[1];

  // write to the pipe
  ignore_result(write(io_write_, (void *)&ij, sizeof(ij)));

  // Wait for the injection done
  auto _this_task = basic_task::running_task();
  if (_this_task) {
    while (_this_task->signal() != kWaitingSignalBroken) {
      loopimpl::shared().wait_for_reading(ij_io->p[0], _this_task, PECO_TIME_S(1));
      if (_this_task->signal() == kWaitingSignalReceived) {
        return true;
      }
    }
    return false;
  } else {
    // block to wait
    fd_set fs;
    int ret = 0;
    while (ret == 0) {
      struct timeval tv = {1, 0};
      do {
        FD_ZERO(&fs);
        FD_SET(ij_io->p[0], &fs);
        ret = select(ij_io->p[0] + 1, &fs, NULL, NULL, &tv);
      } while (ret < 0 && errno == EINTR);
      if (ret > 0) return true;
    }
    // io error?
    return false;
  }
}

/**
 * @brief block current task/thread until the 'timedout'
*/
bool injector::inject_wait(worker_t worker, duration_t timedout) const {
  std::shared_ptr<pipe_wrapper_t> ij_io(new pipe_wrapper_t, [](pipe_wrapper_t * pw) {
    if (pw->p[0] != -1) {
      close(pw->p[0]);
    }
    if (pw->p[1] != -1) {
      close(pw->p[1]);
    }
    delete pw;
  });
  ignore_result(pipe(ij_io->p));

  auto real_time_out = TASK_TIME_NOW() + timedout;

  InjectorInfo ij;
  ij.p_worker = new worker_t(worker);
  ij.timedout = real_time_out.time_since_epoch().count();
  ij.io_out = ij_io->p[1];

  // write to the pipe
  ignore_result(write(io_write_, (void *)&ij, sizeof(ij)));

  // Wait for the injection done
  auto _this_task = basic_task::running_task();
  if (_this_task) {
    loopimpl::shared().wait_for_reading(ij_io->p[0], _this_task, timedout);
    if (_this_task->signal() == kWaitingSignalReceived) {
      return true;
    }
    return false;
  } else {
    // block to wait
    fd_set fs;
    int ret = 0;
    auto nano = timedout.count();
    struct timeval tv = {nano / (1000 * 1000 * 1000), (int)(nano % (1000 * 1000 * 1000)) / 1000};
    do {
      FD_ZERO(&fs);
      FD_SET(ij_io->p[0], &fs);
      ret = select(ij_io->p[0] + 1, &fs, NULL, NULL, &tv);
    } while (ret < 0 && errno == EINTR);
    return (ret > 0);
  }
}

/**
 * @brief just inject the worker and ignore the response
*/
void injector::async_inject(worker_t worker) const {
  InjectorInfo ij;
  ij.p_worker = new worker_t(worker);
  ij.timedout = -1;
  ij.io_out = -1;

  // write to the pipe
  ignore_result(write(io_write_, (void *)&ij, sizeof(ij)));
}

} // namespace shared
} // namespace peco

// Push Chen
