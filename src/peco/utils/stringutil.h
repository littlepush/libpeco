/*
    stringutil.h
    libpeco
    2017-01-22
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

#ifndef PECO_UTILS_STRINGUTIL_H__
#define PECO_UTILS_STRINGUTIL_H__

#include "peco/pecostd.h"
#include <vector>

namespace peco {

// String trim white space
inline std::string &left_trim(std::string &s) {
  return (
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int c) { return !std::isspace(c); })),
    s);
}
inline std::string &right_trim(std::string &s) {
  return (s.erase(std::find_if(s.rbegin(), s.rend(),
                               [](int c) { return !std::isspace(c); })
                    .base(),
                  s.end()),
          s);
}
inline std::string &trim(std::string &s) { return (left_trim(right_trim(s))); }

// Convert to lower, to upper, and capitalize
inline std::string &string_tolower(std::string &s) {
  return (std::transform(s.begin(), s.end(), s.begin(), ::tolower), s);
}
inline std::string string_tolower(const std::string &s) {
  std::string _s(s);
  return string_tolower(_s);
}
inline std::string &string_toupper(std::string &s) {
  return (std::transform(s.begin(), s.end(), s.begin(), ::toupper), s);
}
inline std::string string_toupper(const std::string &s) {
  std::string _s(s);
  return string_toupper(_s);
}

inline std::string &string_capitalize(std::string &s,
                                      std::function<bool(char)> reset_checker) {
  if (!reset_checker)
    reset_checker = [](char) -> bool { return false; };
  bool _r = true;
  return (std::transform(s.begin(), s.end(), s.begin(),
                         [&](char c) -> char {
                           return (reset_checker(c)
                                     ? (_r = true, c)
                                     : (_r ? (_r = false, ::toupper(c))
                                           : ::tolower(c)));
                         }),
          s);
}
inline std::string &string_capitalize(std::string &s) {
  return string_capitalize(s, [](char) -> bool { return false; });
}
inline std::string &string_capitalize(std::string &s,
                                      const std::string &reset_carry) {
  return string_capitalize(s, [&](char c) -> bool {
    return reset_carry.find(c) != std::string::npos;
  });
}

// Address string size
enum { kAddrSize = sizeof(intptr_t) * 2 + 2 };

template <typename T> inline std::string ptr_str(const T *p) {
  std::string _ptrs(kAddrSize + 1, '\0');
  if (sizeof(intptr_t) == 4) {
    // 32Bit
    snprintf(&_ptrs[0], kAddrSize, "0x%08x", (unsigned int)(intptr_t)p);
  } else {
    // 64Bit
    snprintf(&_ptrs[0], kAddrSize, "0x%016lx", (unsigned long)(intptr_t)p);
  }
  return _ptrs;
}

// Get the carry size
template <typename carry_t> inline size_t carry_size(const carry_t &c) {
  return sizeof(c);
}
template <> inline size_t carry_size<std::string>(const std::string &c) {
  return c.size();
}

template <typename carry_iterator_t>
inline std::vector<std::string> split(const std::string &value,
                                      carry_iterator_t b, carry_iterator_t e) {
  typedef std::string::size_type size_type_t;
  std::vector<std::string> components_;
  if (value.size() == 0)
    return components_;
  size_type_t _pos = 0;
  do {
    size_type_t _last_pos = std::string::npos;
    size_t _carry_size = 0;
    for (carry_iterator_t i = b; i != e; ++i) {
      size_type_t _next_carry = value.find(*i, _pos);
      if (_next_carry != std::string::npos && _next_carry < _last_pos) {
        _last_pos = _next_carry;
        _carry_size = carry_size(*i);
      }
    }
    if (_last_pos == std::string::npos)
      _last_pos = value.size();
    if (_last_pos > _pos) {
      std::string _com = value.substr(_pos, _last_pos - _pos);
      components_.emplace_back(_com);
    }
    _pos = _last_pos + _carry_size;
  } while (_pos < value.size());
  return components_;
}

template <typename carry_t>
inline std::vector<std::string> split(const std::string &value,
                                      const carry_t &carry) {
  return split(value, std::begin(carry), std::end(carry));
}

/**
 * @brief String convertoir
*/
struct str2str {
  const std::string& operator()(const std::string& s) const;
};

struct char2str {
  std::string operator()(const char* cs) const;
};

// Join Items as String
template <typename ComIterator, typename ToStringConvertor>
inline std::string join(ComIterator begin, ComIterator end, std::string c, ToStringConvertor cvrt = str2str()) {
  std::string _final_string;
  if (begin == end)
    return _final_string;
  std::string _cstr = c;
  auto i = begin, j = (++begin);
  for (; j != end; ++i, ++j) {
    _final_string += cvrt(*i);
    _final_string += _cstr;
  }
  _final_string += cvrt(*i);
  return _final_string;
}

template <typename Component_t, typename ToStringConvertor>
inline std::string join(const Component_t &parts, std::string c, ToStringConvertor cvrt = str2str()) {
  return join(begin(parts), end(parts), c, cvrt);
}

// Check if string is start with sub string
bool is_string_start(const std::string &s, const std::string &p);
std::string check_string_start_and_remove(const std::string &s,
                                          const std::string &p);

// Check if string is end with sub string
bool is_string_end(const std::string &s, const std::string &p);
std::string check_string_end_and_remove(const std::string &s,
                                        const std::string &p);

// URL encode and decode
std::string url_encode(const std::string &str);
std::string url_decode(const std::string &str);

// Base64
std::string base64_encode(const std::string &str);
std::string base64_decode(const std::string &str);

// Number Check
// Check if a given string is a number, the string can be a float string
bool is_number(const std::string &nstr);

// Date Convert
// Default format is "yyyy-mm-dd hh:mm:ss"
// This function use `strptime` to parse the string
time_t dtot(const std::string &dstr);
time_t dtot(const std::string &dstr, const std::string &fmt);

/**
 * @brief Get system error code's string message
*/
std::string get_sys_error(int code);

} // namespace peco

#endif

// Push Chen
