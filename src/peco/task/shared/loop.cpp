/*
    loop.cpp
    libpeco
    2022-02-14
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

#include "peco/task/shared/loop.h"

#include <thread>

namespace peco {
namespace shared {

std::shared_ptr<loop> loop::create() {
  return std::shared_ptr<loop>(new loop);
}

/**
 * @brief Get the global context loop
*/
std::shared_ptr<loop> loop::gcontext() {
  static std::shared_ptr<loop> s_context_loop = create();
  return s_context_loop;
}

loop::loop() {
  int ij_pipe[2];
  ignore_result(pipe(ij_pipe));
  lt_ = new std::thread([&]() {
    auto ij = std::make_shared<injector>();
    this->ij_ = ij;
    int flag = 0;
    write(ij_pipe[1], &flag, sizeof(int));

    // Start the original loop
    peco::loop::shared()->main();
  });

  // Wait 
  fd_set fs;
  FD_ZERO(&fs);
  FD_SET(ij_pipe[0], &fs);
  int ret = 0;
  while (ret == 0) {
    struct timeval tv = {1, 0};
    do {
      ret = select(ij_pipe[0] + 1, &fs, NULL, NULL, &tv);
    } while (ret < 0 && errno == EINTR);
    if (ret > 0) break;
  }
  close(ij_pipe[0]);
  close(ij_pipe[1]);
}

loop::~loop() {
  if (auto ij = ij_.lock()) {
    ij->async_inject([]() {
      peco::loop::shared()->exit(0);
    });
  }
  if (lt_->joinable()) {
    lt_->join();
  }
  delete lt_;
}

/**
 * @brief Post a 'run' command to the shared loop
*/
peco::shared::task loop::run(worker_t worker) {
  task_id_t tid = kInvalidateTaskId;
  if (auto ij = ij_.lock()) {
    ij->inject_wait([&]() {
      auto t = peco::loop::shared()->run(worker);
      tid = t.task_id();
    }, PECO_TIME_S(1));
  }
  return peco::shared::task(this->shared_from_this(), tid);
}

/**
 * @brief Post a 'run_loop' command to the shared loop
*/
peco::shared::task loop::run_loop(worker_t worker, duration_t interval) {
  task_id_t tid = kInvalidateTaskId;
  if (auto ij = ij_.lock()) {
    ij->inject_wait([&]() {
      auto t = peco::loop::shared()->run_loop(worker, interval);
      tid = t.task_id();
    }, PECO_TIME_S(1));
  }
  return peco::shared::task(this->shared_from_this(), tid);
}

/**
 * @brief Post a 'run_delay' command to the shared loop
*/
peco::shared::task loop::run_delay(worker_t worker, duration_t delay) {
  task_id_t tid = kInvalidateTaskId;
  if (auto ij = ij_.lock()) {
    ij->inject_wait([&]() {
      auto t = peco::loop::shared()->run_delay(worker, delay);
      tid = t.task_id();
    }, PECO_TIME_S(1));
  }
  return peco::shared::task(this->shared_from_this(), tid);
}

/**
 * @brief block current task/thread to inject the worker
*/
bool loop::sync_inject(worker_t worker) {
  if (auto ij = ij_.lock()) {
    return ij->sync_inject(worker);
  }
  return false;
}

/**
 * @brief block current task/thread until the 'timedout'
*/
bool loop::inject_wait(worker_t worker, duration_t timedout) {
  if (auto ij = ij_.lock()) {
    return ij->inject_wait(worker, timedout);
  }
  return false;
}

/**
 * @brief just inject the worker and ignore the response
*/
void loop::async_inject(worker_t worker) {
  if (auto ij = ij_.lock()) {
    ij->async_inject(worker);
  }
}

/**
 * @brief post `exit` command to the shared loop
*/
void loop::exit(int code) {
  if (auto ij = ij_.lock()) {
    ij->async_inject([code]() {
      peco::loop::shared()->exit(code);
    });
  }
}

} // namespace shared
} // namespace peco

// Push Chen
