/*
    any.h
    libpeco
    2022-0203
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

#ifndef PECO_ANY_H__
#define PECO_ANY_H__

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
// just use std::any
#include <any>

namespace peco {
typedef std::any  any;
}
#else

#include <iostream>
#include <memory>
#include <functional>

namespace peco {

template< typename ValueType >
struct no_type_copy {
  static std::shared_ptr<void> cast(void* value) {
    if (value == nullptr) return nullptr;
    std::shared_ptr<ValueType> data = std::make_shared<ValueType>(*static_cast<ValueType *>(value));
    return std::static_pointer_cast<void>(data);
  }
};

typedef std::function<std::shared_ptr<void>(void *)> no_type_cast;

class any {
public:
  // C'str
  any() noexcept;
  any( const any& other );
  any( any&& other ) noexcept;

  template< class ValueType >
  any( std::shared_ptr<ValueType> ptr ) 
    : data_(std::static_pointer_cast<void>(ptr)),
      cast_(no_type_copy<typename std::decay<ValueType>::type>::cast) {}

  template< class ValueType >
    any( ValueType&& value ) 
      : data_(std::static_pointer_cast<void>(
          std::make_shared<typename std::decay<ValueType>::type>(std::forward<ValueType>(value))
        )), 
        cast_(no_type_copy<typename std::decay<ValueType>::type>::cast) {}

  // Copy
  any& operator=( const any& rhs );
  any& operator=( any&& rhs ) noexcept;

  template<typename ValueType> any& operator=( ValueType&& rhs ) {
    data_ = std::static_pointer_cast<void>(std::make_shared<typename std::decay<ValueType>::type>(std::move(rhs)));
    cast_ = no_type_copy<typename std::decay<ValueType>::type>::cast;
    return *this;
  }

  // D'str
  ~any() = default;

  // Members
  // change the contained object, constructing the new object directly
  template<typename ValueType, class... Args>
  typename std::decay<ValueType>::type& emplace(Args&& ...args) {
    cast_ = no_type_copy<typename std::decay<ValueType>::type>::cast;
    std::shared_ptr<ValueType> value = std::make_shared<typename std::decay<ValueType>::type>(std::forward<Args>(args)...);
    data_ = std::static_pointer_cast<void>(value);
    return *value;
  }
  template<typename ValueType, class U, class... Args>
  typename std::decay<ValueType>::type& emplace(std::initializer_list<U> il, Args&&... args) {
    cast_ = no_type_copy<typename std::decay<ValueType>::type>::cast;
    std::shared_ptr<ValueType> value = std::make_shared<typename std::decay<ValueType>::type>(il, std::forward<Args>(args)...);
    data_ = std::static_pointer_cast<void>(value);
    return *value;
  }

  // destroys contained object 
  void reset() noexcept;

  // swaps two any objects 
  void swap(any& other) noexcept;

  // Checks whether the object contains a value.
  bool has_value() const noexcept;

public:
  // friend cast function
  template<class T> friend const T* any_cast(const any*) noexcept;
  template<class T> friend T* any_cast(any*) noexcept;
protected:
  std::shared_ptr<void> data_;
  no_type_cast cast_;
};

template<class T> const T* any_cast(const any* operand) noexcept {
  return std::static_pointer_cast<T>(operand->data_).get();
}
template<class T> T* any_cast(any* operand) noexcept {
  return std::static_pointer_cast<T>(operand->data_).get();
}
template<class T> T any_cast(const any& operand) {
  return *any_cast<T>(&operand);
}
template<class T> T any_cast(any& operand) {
  return *any_cast<T>(&operand);
}
template<class T> T any_cast(any&& operand) {
  return static_cast<T>(std::move(*any_cast<T>(&operand)));
}

} // namespace peco

#endif
#endif

// Push Chen
