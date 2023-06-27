/*
    iprange.cpp
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

#include "net/iprange.h"

namespace peco {

void iprange_t::__parse_range(const std::string &fmt) {
  // This is invalidate range
  size_t _slash_pos = fmt.find('/');
  std::string _ipstr, _maskstr;
  if (_slash_pos == std::string::npos) {
    _ipstr = fmt;
    _maskstr = "32";
  } else {
    _ipstr = fmt.substr(0, _slash_pos);
    _maskstr = fmt.substr(_slash_pos + 1);
  }

  ip_t _lowip(_ipstr);
  if ((uint32_t)_lowip == 0)
    return; // Invalidate

  // Mask part
  if (_maskstr.size() > 2) {
    ip_t _highip(_maskstr);
    if ((uint32_t)_highip == 0)
      return; // Invalidate Second Part
    if (_highip < _lowip)
      return; // Invalidate order of the range
    low_ = std::move(_lowip);
    high_ = std::move(_highip);

    // Try to calculate the mask_
    uint32_t _d = (uint32_t)high_ - (uint32_t)low_;
    uint32_t _bc = 0;
    while (_d != 0) {
      _bc += 1;
      _d >>= 1;
    }
    mask_ = 32 - _bc;
  } else {
    uint32_t _mask = stoi(_maskstr, nullptr, 10);
    if (_mask > 32)
      return;
    uint32_t _fmask = 0xFFFFFFFF;
    _fmask <<= (32 - _mask);

    uint32_t _low = ntohl((uint32_t)_lowip) & _fmask;
    uint32_t _high = _low | (~_fmask);
    low_ = htonl(_low);
    high_ = htonl(_high);

    mask_ = _mask;
  }
}

iprange_t::iprange_t() : low_((uint32_t)0), high_((uint32_t)0), mask_(32u) {}

// Copy
iprange_t::iprange_t(const iprange_t &rhs)
  : low_(rhs.low_), high_(rhs.high_), mask_(rhs.mask_) {}
iprange_t::iprange_t(iprange_t &&lhs)
  : low_(std::move(lhs.low_)), high_(std::move(lhs.high_)), mask_(std::move(lhs.mask_)) {}
iprange_t &iprange_t::operator=(const iprange_t &rhs) {
  if (this == &rhs)
    return *this;
  low_ = rhs.low_;
  high_ = rhs.high_;
  mask_ = rhs.mask_;
  return *this;
}
iprange_t &iprange_t::operator=(iprange_t &&lhs) {
  if (this == &lhs)
    return *this;
  low_ = std::move(lhs.low_);
  high_ = std::move(lhs.high_);
  mask_ = std::move(lhs.mask_);
  return *this;
}

// Format
iprange_t::iprange_t(const std::string &fmt) { this->__parse_range(fmt); }
iprange_t::iprange_t(const ip_t &from, const ip_t &to) {
  if (from < to) {
    low_ = from;
    high_ = to;
  } else {
    low_ = to;
    high_ = from;
  }
  // Try to calculate the mask_
  uint32_t _d = ntohl((uint32_t)high_) - ntohl((uint32_t)low_);
  uint32_t _bc = 0;
  while (_d != 0) {
    _bc += 1;
    _d >>= 1;
  }
  mask_ = 32 - _bc;
}

// Validation
iprange_t::operator bool() const {
  return (uint32_t)low_ != (uint32_t)0 && (uint32_t)high_ != (uint32_t)0;
}

// Get the mask
uint32_t iprange_t::mask() const { return mask_; }
// Get Origin
const ip_t &iprange_t::origin() const { return low_; }

// Output string
iprange_t::operator const std::string() const {
  return low_.str() + "/" + std::to_string(mask_);
}

std::string iprange_t::str() const {
  return low_.str() + "/" + std::to_string(mask_);
}
// Check if an ip is in the range
bool iprange_t::isIpInRange(const ip_t &ip) const {
  return low_ <= ip && ip <= high_;
}

// Output the ip range
std::ostream &operator<<(std::ostream &os, const iprange_t &range) {
  os << range.str();
  return os;
}

} // namespace peco
