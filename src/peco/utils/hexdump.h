/*
    hexdump.h
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

#pragma once

#ifndef PECO_HEXDUMP_H__
#define PECO_HEXDUMP_H__

#include "peco/pecostd.h"

namespace peco {

// Convert data to hexdump in the following format:
//  0xADDR: 00 00 00 00 ...(16)   |xxx...(16)|
class hexdump {
public:
  enum {
    CHAR_PER_LINE = 16,
    ADDR_SIZE = sizeof(intptr_t) * 2 + 2, // 0x00
    LINE_BUF_SIZE = CHAR_PER_LINE * 4 + 3 + ADDR_SIZE + 2
  };

protected:
  const char *data_;
  size_t length_;

  size_t line_count_;
  mutable std::string line_buf_;

protected:
  // Calculate line count
  void __calculate_data();
  // Get the data length in specified line
  size_t __line_size(size_t index) const;

public:
  // Constructures
  hexdump(const char *data, size_t length);
  explicit hexdump(const std::string &data);
  hexdump(const hexdump &rhs);
  hexdump(hexdump &&lhs);
  hexdump();

  // Operators
  hexdump &operator=(const hexdump &rhs);
  hexdump &operator=(hexdump &&lhs);

  // Data Methods
  const char *line(size_t index) const;
  size_t line_count() const;

  // Output Methods
  void print(FILE *fp = stdout) const;
  void print(size_t from_line, size_t to_line, FILE *fp = stdout) const;

  // Get specified lines data
  hexdump lines(size_t from_line, size_t to_line) const;
};

// Stream Operator to output the hexdump data.
std::ostream &operator<<(std::ostream &os, const hexdump &h);

} // namespace peco

#endif

// Push Chen
