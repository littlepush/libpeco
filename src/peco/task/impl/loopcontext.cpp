/*
    loopcontext.cpp
    libpeco
    2022-10-21
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

#include "peco/task/impl/loopcontext.hxx"

namespace peco {

loopcontext::~loopcontext() {
  // nothing to do
}

/**
 * @brief Get the main context
*/
stack_context_t* loopcontext::get_main_context() {
  return &main_task_.ctx;
}

/**
 * @brief create a task object
*/
std::shared_ptr<basic_task> loopcontext::create_task(
  worker_t worker, repeat_count_t repeat_count = REPEAT_COUNT_ONESHOT, 
  duration_t interval = PECO_TIME_MS(0)
) {
  auto t = std::shared_ptr<basic_task>(new basic_task(
    stack_cache_.fetch(), 
    (running_task_ ? running_task_->task_id() : kInvalidateTaskId), 
    worker, repeat_count, interval
  ));
  t->reset_task(this->get_main_context());
  task_cache_[t->task_id()] = t;
}

/**
 * @brief remove a task from the cache
*/
void loopcontext::del_task(task_id_t tid) {
  task_cache_.erase(tid);
}

/**
 * @brief search and return the task
*/
std::shared_ptr<basic_task> loopcontext::find_task(task_id_t tid) {
  if (tid == kInvalidateTaskId) return nullptr;
  auto t_it = task_cache_.find(tid);
  if (t_it == task_cache_.end()) {
    return nullptr;
  }
  return t_it->second;
}

/**
 * @brief get alive task size
*/
size_t loopcontext::alive_task_size() const {
  return task_cache_.size();
}

/**
 * @brief swap current context to the given task
*/
void loopcontext::swap_to_task(std::shared_ptr<basic_task> task) {
  if (!task || (task->status() == kTaskStatusStopped)) {
    return;
  }
  running_task_ = task;
  task->swap_to_task(this->get_main_context());
  running_task_ = nullptr;
}

/**
 * @brief swap back to main context
*/
void loopcontext::swap_to_main() {
  if (running_task_ == nullptr) {
    return;
  }
#if PECO_TARGET_APPLE
  if (!setjmp(running_task_->get_task()->ctx)) {
    longjmp(*this->get_main_context(), 1);
  }
#else
  swapcontext(&(running_task_->get_task()->ctx), this->get_main_context());
#endif
}

/**
 * @brief Scan all cached task
*/
void loopcontext::foreach(std::function<void(std::shared_ptr<basic_task>)> handler) {
  for (auto& r : task_cache_) {
    handler(r.second);
  }
}

} // namespace peco

// Push Chen
