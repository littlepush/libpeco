/*
    tasklist.cpp
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

#include "peco/task/impl/tasklist.hxx"

namespace peco {

/**
 * @brief Inner Cache Key Item
*/
/**
 * @brief Create from the basic task ptr
*/
tasklist::cache_item_t::cache_item_t(basic_task_ptr_t t)
  :tid(t->task_id()), next_fire_time(t->get_task()->next_fire_time) { }
/**
 * @brief Operator to make this type can be map's key type
*/
bool tasklist::cache_item_t::operator <(const tasklist::cache_item_t& r) const {
  return (this->tid != r.tid) && (this->next_fire_time < r.next_fire_time);
}
/**
 * @brief Simple operator == to fast map index
*/
bool tasklist::cache_item_t::operator == (const tasklist::cache_item_t& r) const {
  return this->tid == r.tid;
}
bool tasklist::cache_item_t::operator != (const tasklist::cache_item_t& r) const {
  return this->tid != r.tid;
}
/**
 * @brief Easy Cstor
 */
tasklist::cache_fd_t::cache_fd_t(fd_t fd, EventType etype) 
  : fd(fd), event_type(etype) { }

/**
 * @brief Operator to make this type can be map's key type
 */
bool tasklist::cache_fd_t::operator<(const tasklist::cache_fd_t &r) const {
  return this->fd < r.fd;
}

/**
 * @brief Simple operator compare to fast map index
 */
bool tasklist::cache_fd_t::operator==(const tasklist::cache_fd_t &r) const {
  return (this->fd == r.fd && this->event_type == r.event_type);
}
bool tasklist::cache_fd_t::operator!=(const tasklist::cache_fd_t &r) const {
  return (this->fd != r.fd || this->event_type != r.event_type);
}

/**
 * @brief Get the nearest fire time among all task in this list
*/
task_time_t tasklist::nearest_time() const {
  return ordered_time_map_.begin()->first.next_fire_time;
}

/**
 * @brief Fetch the task with the nearest fire time
*/
tasklist::result_type tasklist::fetch() {
  auto begin_it = ordered_time_map_.begin();
  tasklist::result_type result{
    begin_it->first.tid, begin_it->second
  };
  ordered_time_map_.erase(begin_it);
  return result;
}

/**
 * @brief Sort & Insert a task into the list
*/
void tasklist::insert(tasklist::basic_task_ptr_t t, worker_t timedout_handler,
                      fd_t fd, EventType event_type) {
  if (t == nullptr) return;
  ordered_time_map_[tasklist::cache_item_t(t)] = timedout_handler;
  if (fd != -1l) {
    // Rewrite the fd map
    fd_cache_map_.emplace(cache_fd_t(fd, event_type), t->task_id());
  }
}

/**
 * @brief Replace a given task's next_fire_time to the fire_time
*/
void tasklist::replace_time(tasklist::basic_task_ptr_t t, task_time_t fire_time) {
  auto task_it = ordered_time_map_.find(cache_item_t(t));
  // No such task
  if (task_it == ordered_time_map_.end()) return;
  // Copy the on_time handler
  auto on_time = std::move(task_it->second);
  // Remove the old task
  ordered_time_map_.erase(task_it);
  t->get_task()->next_fire_time = fire_time;
  ordered_time_map_[tasklist::cache_item_t(t)] = on_time;
}

/**
 * @brief Remove a task from the list
*/
void tasklist::erase(basic_task_ptr_t t, fd_t fd, EventType event_type) {
  if (t == nullptr) return;
  tasklist::cache_item_t cache_key(t);
  ordered_time_map_.erase(cache_key);
  if (fd != -1l) {
    auto fd_range = fd_cache_map_.equal_range(cache_fd_t(fd, event_type));
    for (auto fd_it = fd_range.first; fd_it != fd_range.second;) {
      if (fd_it->first.event_type == event_type && fd_it->second == t->task_id()) {
        fd_it = fd_cache_map_.erase(fd_it);
      } else {
        ++fd_it;
      }
    }
  }
}

/**
 * @brief Erase all fd related item
*/
void tasklist::erase(fd_t fd, std::function<basic_task_ptr_t(task_id_t)> p_find) {
  const static EventType kEventTypePool[] = {kEventTypeRead, kEventTypeWrite};
  if (fd == -1l) return;
  for (size_t i = 0; i < 2u; ++i) {
    auto fd_range = fd_cache_map_.equal_range(cache_fd_t(fd, kEventTypePool[i]));
    for (auto fd_it = fd_range.first; fd_it != fd_range.second;) {
      // remove related timed list
      if (p_find) {
        auto ptrt = p_find(fd_it->second);
        if ( ptrt != nullptr ) {
          ordered_time_map_.erase(cache_item_t(ptrt));
        }
        fd_it = fd_cache_map_.erase(fd_it);
      }
    }
  }
}

/**
 * @brief Get the task count
*/
size_t tasklist::size() const {
  return ordered_time_map_.size();
}

/**
 * @brief Check if the given task is in the list
*/
bool tasklist::has(basic_task_ptr_t t) const {
  return (ordered_time_map_.find(cache_item_t(t)) != ordered_time_map_.end());
}

/**
 * @brief Search task with the related fd
*/
std::list<task_id_t> tasklist::search(fd_t fd, EventType event_type) {
  std::list<task_id_t> result;
  auto fd_range = fd_cache_map_.equal_range(cache_fd_t(fd, event_type));
  for (auto fd_it = fd_range.first; fd_it != fd_range.second; ++fd_it) {
    if (fd_it->first.event_type == event_type) {
      result.push_back(fd_it->second);
    }
  }
  return result;
}

/**
 * @brief Search all event's task
*/
std::list<task_id_t> tasklist::search_all(fd_t fd) {
  auto read_result = this->search(fd, kEventTypeRead);
  auto write_result = this->search(fd, kEventTypeWrite);
  read_result.splice(read_result.end(), write_result);
  return read_result;
}

} // namespace peco

// Push Chen
