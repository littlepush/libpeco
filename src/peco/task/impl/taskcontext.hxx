/*
    taskcontext.hxx
    libpeco
    2022-02-09
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

#ifndef PECO_TASKCONTEXT_HXX
#define PECO_TASKCONTEXT_HXX

#include "peco/pecostd.h"
#include "peco/task/taskdef.h"

#if PECO_TARGET_WIN
#include "peco/task/windows/ucontext.hxx"
#elif PECO_TARGET_APPLE
#include <setjmp.h>
#else
#include <ucontext.h>
#endif

namespace peco {

/**
 * @brief Core Event
*/
#if PECO_TARGET_WIN
typedef long                core_event_t;
#endif

/**
 * @brief Stack Type
*/
typedef void*         stack_ptr_t;

/**
 * @brief Repeat Count
*/
typedef size_t        repeat_count_t;
#define REPEAT_COUNT_ONESHOT      1u
#define REPEAT_COUNT_INFINITIVE   ((size_t)-1)

/**
 * @brief Stack Context Type
*/
#if PECO_TARGET_APPLE
typedef jmp_buf       stack_context_t;
#else
typedef ucontext_t    stack_context_t;
#endif

/**
 * @brief Basic Task
*/
typedef struct __task_context__ {
  /**
   * @brief The ID of the task, usually is the address of the task
  */
  task_id_t                   tid;

  /**
   * @brief The stack
  */
  stack_ptr_t                 stack;

  /**
   * @brief Worker
  */
  worker_t                    worker;

  /**
   * @brief Destroy Worker
  */
  worker_t                    atexit;

  /**
   * @brief Task Status
  */
  TaskStatus                  status;

  /**
   * @brief If the task is marked cancelled
  */
  bool                        cancelled;

  /**
   * @brief Signal of the last monitoring event
  */
  WaitingSignal               signal;

  /**
   * @brief Repeat Count of the task
  */
  repeat_count_t              repeat_count;

  /**
   * @brief Next fire time, or timedout time
  */
  task_time_t                 next_fire_time;

  /**
   * @brief Time interval for repeatable task
  */
  duration_t                  interval;

  /**
   * @brief The stack context
  */
  stack_context_t             ctx;

  /**
   * @brief To make sure the ctx has enough space to save
   * data, remine 2k empty space after it.
  */
  uint64_t                    zero[32];
} task_context_t;

/**
 * @brief Extra info of a task
*/
typedef struct __task_extra__ {
  /**
   * @brief Task name
  */
  char                        name[kTaskNameLength];
  /**
   * @brief Must be zero incase the length of name is 64
  */
  uint16_t                    zero1;
  /**
   * @brief User send args
  */
  void *                      arg;
  /**
   * @brief Running flags
  */
  uint64_t                    flags[kMaxFlagCount];
  /**
   * @brief Parent task's id
  */
  task_id_t                   parent_tid;
} task_extra_t;

enum {
  kTaskContextSize = (sizeof(task_context_t) / sizeof(intptr_t) + 1) * sizeof(intptr_t)
};

}

#endif

// Push Chen
