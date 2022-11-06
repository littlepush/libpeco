/*
    hexdump.cpp
    libpeco
    2019-04-23
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

#include "peco/utils/hexdump.h"
#include <stdio.h>

namespace peco {
void hexdump::__calculate_data() {
  line_count_ =
    (length_ / CHAR_PER_LINE) + ((length_ % CHAR_PER_LINE) > 0 ? 1 : 0);
  line_buf_.resize(LINE_BUF_SIZE);
  // We at least will output one line with begin address.
  if (line_count_ == 0)
    line_count_ = 1;
}
size_t hexdump::__line_size(size_t index) const {
  size_t _ls = (length_ % CHAR_PER_LINE);
  if (_ls == 0)
    _ls = CHAR_PER_LINE;
  return (index == (line_count_ - 1)) ? ((line_count_ == 1) ? length_ : _ls)
                                      : CHAR_PER_LINE; // Normal Line
}
// Constructures
hexdump::hexdump(const char *data, size_t length)
  : data_(data), length_(length) {
  this->__calculate_data();
}
hexdump::hexdump(const std::string &data)
  : data_(data.c_str()), length_(data.size()) {
  this->__calculate_data();
}
hexdump::hexdump(const hexdump &rhs) : data_(rhs.data_), length_(rhs.length_) {
  this->__calculate_data();
}
hexdump::hexdump(hexdump &&lhs) : data_(lhs.data_), length_(lhs.length_) {
  this->__calculate_data();
}
hexdump::hexdump() : data_(NULL), length_(0) { this->__calculate_data(); }

// Operators
hexdump &hexdump::operator=(const hexdump &rhs) {
  if (this == &rhs)
    return *this;
  data_ = rhs.data_;
  length_ = rhs.length_;
  this->__calculate_data();
  return *this;
}
hexdump &hexdump::operator=(hexdump &&lhs) {
  if (this == &lhs)
    return *this;
  data_ = lhs.data_;
  length_ = lhs.length_;
  this->__calculate_data();
  return *this;
}

// Data Methods
const char *hexdump::line(size_t index) const {
  char *_buf = &line_buf_[0];
  const char *_begin_data = data_ + index * CHAR_PER_LINE;
  memset(_buf, 0x20, LINE_BUF_SIZE);
  if (sizeof(intptr_t) == 4) {
// 32Bit
#if !PECO_TARGET_WIN
    snprintf(_buf, LINE_BUF_SIZE, "%08x: ", (unsigned int)(intptr_t)_begin_data);
#else
    sprintf_s(_buf, LINE_BUF_SIZE,
              "%08x: ", (unsigned int)(intptr_t)_begin_data);
#endif
  } else {
// 64Bit
#if !PECO_TARGET_WIN
    snprintf(_buf, LINE_BUF_SIZE, "%016lx: ", (unsigned long)(intptr_t)_begin_data);
#else
    sprintf_s(_buf, LINE_BUF_SIZE,
              "%016lx: ", (unsigned long)(intptr_t)_begin_data);
#endif
  }
  size_t _line_size = this->__line_size(index);
  for (size_t _c = 0; _c < _line_size; ++_c) {
#if !PECO_TARGET_WIN
    snprintf(_buf + _c * 3 + ADDR_SIZE, LINE_BUF_SIZE, "%02x ",
#else
    sprintf_s(_buf + _c * 3 + ADDR_SIZE, LINE_BUF_SIZE, "%02x ",
#endif
            (unsigned char)_begin_data[_c]);
    _buf[(_c + 1) * 3 + ADDR_SIZE] = ' ';
    _buf[CHAR_PER_LINE * 3 + 1 + _c + ADDR_SIZE + 1] =
      (isprint((unsigned char)(_begin_data[_c])) ? _begin_data[_c] : '.');
  }
  _buf[CHAR_PER_LINE * 3 + ADDR_SIZE + 1] = '|';
  _buf[LINE_BUF_SIZE - 3] = '|';
  _buf[LINE_BUF_SIZE - 2] = '\0';
  return line_buf_.c_str();
}
size_t hexdump::line_count() const { return line_count_; }

// Output Methods
void hexdump::print(FILE *fp) const {
  for (size_t i = 0; i < line_count_; ++i) {
    fprintf(fp, "%s\n", this->line(i));
  }
}
void hexdump::print(size_t from_line, size_t to_line, FILE *fp) const {
  if (to_line > line_count_ - 1)
    to_line = (line_count_ - 1);
  for (size_t i = from_line; i <= to_line; ++i) {
    fprintf(fp, "%s\n", this->line(i));
  }
}

// Get specified lines data
hexdump hexdump::lines(size_t from_line, size_t to_line) const {
  if (to_line > line_count_ - 1)
    to_line = (line_count_ - 1);
  size_t _lines = to_line - from_line + 1;
  if (_lines > line_count_)
    _lines = line_count_;
  return hexdump(data_ + (from_line * CHAR_PER_LINE), _lines * CHAR_PER_LINE);
}

// Stream Operator to output the hexdump data.
std::ostream &operator<<(std::ostream &os, const hexdump &h) {
  size_t lc = h.line_count() - 1;
  for (size_t i = 0; i < lc; ++i) {
    os << h.line(i) << std::endl;
  }
  os << h.line(lc);
  return os;
}

} // namespace peco

// Push Chen