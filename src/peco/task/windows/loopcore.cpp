/*
    loopcore.cpp
    libpeco
    2022-09-23
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
}

/**
 * @brief Block and wait for all fd
 */
void loopcore::wait(duration_t duration) {
  // tbd
}

/**
 * @brief Break the waiting
 */
void loopcore::stop() {
  // tbd
}

/**
 * @brief Process the reading event
 */
bool loopcore::add_read_event(fd_t fd) {
  // tbd
  return false;
}
void loopcore::del_read_event(fd_t fd) {
  // tbd
}

/**
 * @brief Process the writing event
 */
bool loopcore::add_write_event(fd_t fd) {
  // tbd
  return false;
}
void loopcore::del_write_event(fd_t fd) {
  // tbd
}

/**
 * @brief Get all time cost on waiting
*/
uint64_t loopcore::get_wait_time() const {
  return time_waited_;
}

} // namespace peco

// Push Chen
