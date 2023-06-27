/*
    any.cpp
    libpeco
    2022-02-03
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

#include "any.h"
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
  // Do nothing
#else

namespace peco {

any::any() noexcept {
  // do nothing, data_ is null
}
any::any( const any& other ) {
  if (other.cast_ != nullptr) {
    data_ = other.cast_(other.data_.get());
    cast_ = other.cast_;
  }
}

any::any( any&& other ) noexcept : data_(std::move(other.data_)), cast_(other.cast_) 
{ }

any& any::operator=( const any& rhs ) {
  if (this == &rhs) return *this;
  if (rhs.cast_ == nullptr) {
    data_ = nullptr;
    cast_ = nullptr;
  } else {
    data_ = rhs.cast_(rhs.data_.get());
    cast_ = rhs.cast_;
  }
  return *this;
}
any& any::operator=( any&& rhs ) noexcept {
  if (this == &rhs) return *this;
  data_ = std::move(rhs.data_);
  cast_ = rhs.cast_;
  return *this;
}

void any::reset() noexcept {
  data_ = nullptr;
  cast_ = nullptr;
}

void any::swap(any& other) noexcept {
  auto _tmp_data = other.data_;
  auto _tmp_cast = other.cast_;
  other.data_ = data_;
  other.cast_ = cast_;
  data_ = _tmp_data;
  cast_ = _tmp_cast;
}

bool any::has_value() const noexcept {
  return (data_ != nullptr);
}

}

#endif

