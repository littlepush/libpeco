/*
    iprange.h
    libpeco
    2022-02-17
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

#ifndef PECO_NET_IPRANGE_H__
#define PECO_NET_IPRANGE_H__

#include "peco/pecostd.h"
#include "peco/net/ip.h"

namespace peco {

/*!
IP Range and IP Mask
support format:
  x.x.x.x
  x.x.x.x/24
mask should not bigger than 32
*/
class iprange_t {
private:
  ip_t            low_;
  ip_t            high_;
  uint32_t        mask_;

  void __parse_range(const std::string &fmt);
public:
  iprange_t();

  // Copye
  iprange_t(const iprange_t& rhs);
  iprange_t(iprange_t&& lhs);
  iprange_t& operator =(const iprange_t& rhs);
  iprange_t& operator =(iprange_t&& lhs);

  // Format
  explicit iprange_t(const std::string & fmt);
  iprange_t(const ip_t& from, const ip_t& to);

  // Validation
  explicit operator bool() const;

  // Output string
  explicit operator const std::string() const;
  std::string str() const;

  // Get the mask
  uint32_t mask() const;
  // Get Origin
  const ip_t& origin() const;

  // Check if an ip is in the range
  bool isIpInRange(const ip_t& ip) const;
};

// Output the ip range
std::ostream & operator << (std::ostream &os, const iprange_t &range);

} // namespace peco

#endif

// Push Chen
