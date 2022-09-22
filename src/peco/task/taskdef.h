/*
    taskdef.h
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

#ifndef PECO_TASKDEF_HXX
#define PECO_TASKDEF_HXX

#include "peco/pecostd.h"

namespace peco {

#define __PECO_S1(x) #x
#define __PECO_S2(x) __PECO_S1(x)
#define PECO_CODE_LOCATION __FILE__ " : " __PECO_S2(__LINE__)

/**
 * @brief task id
*/
typedef intptr_t task_id_t;
enum {
  kInvalidateTaskId   = -1ll
};

#if PECO_TARGET_WIN
typedef long long           fd_t;
#else
typedef long                fd_t;
#endif

/**
 * @brief time stamp
*/
typedef std::chrono::steady_clock                 steady_clock_t;
typedef std::chrono::time_point<steady_clock_t>   task_time_t;
typedef std::chrono::nanoseconds                  duration_t;
#define TASK_TIME_NOW   std::chrono::steady_clock::now
#define PECO_TIME_S(x)      std::chrono::seconds(x)
#define PECO_TIME_MS(x)     std::chrono::milliseconds(x)
#define PECO_TIME_NS(x)     std::chrono::nanoseconds(x)

/**
 * @brief The wroker in the task
*/
typedef std::function< void(void) >               worker_t;

#ifndef TASK_STACK_SIZE
// Usually a 512KB Stack is enough for most programs
#define TASK_STACK_SIZE     524288   // 512Kb
#endif

#ifndef STACK_RESERVED_SIZE
// We will reserve at least 8KB data in the front of the stack
#define STACK_RESERVED_SIZE 8192     // 8KB
#endif

/**
 * @brief Task Status
*/
typedef enum {
  /**
   * @brief Pending task, not been started yet
  */
  kTaskStatusPending,
  /**
   * @brief Current task is running, using CPU now
  */
  kTaskStatusRunning,
  /**
   * @brief Current task has been paused, maybe waiting for some signal
  */
  kTaskStatusPaused,
  /**
   * @brief Stopped but not destoried, will never be waked up again
  */
  kTaskStatusStopped,
} TaskStatus;

/**
 * @brief Task Waiting Signal
*/
typedef enum {
  /**
   * @brief There is a signal to wake up current task
  */
  kWaitingSignalReceived,
  /**
   * @brief Until all given time has passed, there is no signal
  */
  kWaitingSignalNothing,
  /**
   * @brief There is a signal to break the waiting event
  */
  kWaitingSignalBroken
} WaitingSignal;

/**
 * @brief Task Waiting Event
*/
typedef enum {
  /**
   * @brief The file descriptor is ready for reading
  */
  kEventTypeRead      = 0x01,
  /**
   * @brief The file descriptor is ready for writing
  */
  kEventTypeWrite     = 0x02
} EventType;

enum {
  /**
   * @brief Max User Flag Count
  */
  kMaxFlagCount       = 16,

  /**
   * @brief Task name's max length
  */
  kTaskNameLength     = 64
};

} // namespace peco

#endif

// Push Chen
