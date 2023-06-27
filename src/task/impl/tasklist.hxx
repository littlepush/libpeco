/*
    tasklist.hxx
    libpeco
    2022-02-10
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

#ifndef PECO_TASKLIST_HXX
#define PECO_TASKLIST_HXX

#include "task/impl/taskcontext.hxx"
#include "task/impl/basictask.hxx"

#include <list>
#include <map>

namespace peco {

/**
 * @brief Ordered task list by its until_time
*/
class tasklist {
public:
  typedef std::shared_ptr<basic_task>   basic_task_ptr_t;

  /**
   * @brief Inner Cache Key Item
  */
  struct cache_item_t {
    /**
     * @brief Create from the basic task ptr
    */
    cache_item_t(basic_task_ptr_t t);
    /**
     * @brief Task ID
    */
    task_id_t     tid;
    /**
     * @brief Next fire time
    */
    task_time_t   next_fire_time;

    /**
     * @brief Operator to make this type can be map's key type
    */
    bool operator <(const cache_item_t& r) const;

    /**
     * @brief Simple operator compare to fast map index
    */
    bool operator == (const cache_item_t& r) const;
    bool operator != (const cache_item_t& r) const;
  };

  struct cache_fd_t {
    /**
     * @brief The cached fd
    */
    long          fd;
    /**
     * @brief One of the event type to monitor
    */
    EventType     event_type;

    /**
     * @brief Easy Cstor
    */
    cache_fd_t(long fd, EventType etype);

    /**
     * @brief Operator to make this type can be map's key type
    */
    bool operator <(const cache_fd_t& r) const;

    /**
     * @brief Simple operator compare to fast map index
    */
    bool operator == (const cache_fd_t& r) const;
    bool operator != (const cache_fd_t& r) const;
  };

  /**
   * @brief Item to return when fetch the nearest time
  */
  struct result_type {
    /**
     * @brief Fetched task id
    */
    task_id_t     tid;
    /**
     * @brief Binded worker when timedout
    */
    worker_t      on_time;
  };

public:
  /**
   * @brief Get the nearest fire time among all task in this list
  */
  task_time_t nearest_time() const;

  /**
   * @brief Fetch the task with the nearest fire time
  */
  result_type fetch();

  /**
   * @brief Sort & Insert a task into the list
  */
  void insert(basic_task_ptr_t t, worker_t timedout_handler, 
    long fd = -1l, EventType event_type = kEventTypeRead);

  /**
   * @brief Replace a given task's next_fire_time to the fire_time
  */
  void replace_time(basic_task_ptr_t t, task_time_t fire_time);

  /**
   * @brief Remove a task from the list
  */
  void erase(basic_task_ptr_t t, long fd = -1l, EventType event_type = kEventTypeRead);

  /**
   * @brief Erase all fd related item
  */
  void erase(long fd);

  /**
   * @brief Get the task count
  */
  size_t size() const;

  /**
   * @brief Check if the given task is in the list
  */
  bool has(basic_task_ptr_t t) const;

  /**
   * @brief Search task with the related fd
  */
  std::list<task_id_t> search(long fd, EventType event_type);

  /**
   * @brief Search all event's task
  */
  std::list<task_id_t> search_all(long fd);

protected:
  std::map<cache_item_t, worker_t>   ordered_time_map_;
  std::multimap<cache_fd_t, task_id_t> fd_cache_map_;
};

} // namespace peco

#endif

// Push Chen
