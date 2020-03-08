/*
    stringutil.h
    PECoUtils
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

#ifndef PE_CO_UTILS_STRINGUTIL_H__
#define PE_CO_UTILS_STRINGUTIL_H__

#include <peco/utils/mustd.hpp>
#include <openssl/md5.h>

namespace std {
    // Override to_string to support char * and string
    inline std::string to_string(const char* __val) { return string(__val); }
    inline const std::string& to_string(const string& __val) { return __val; }

    template < typename T >
    inline void ignore_result( T unused_result ) { (void) unused_result; }
}

namespace pe { namespace utils {
    // String trim white space
    std::string& left_trim(std::string& s);
    std::string& right_trim(std::string& s);
    std::string& trim(std::string& s);

    // Convert to lower, to upper, and capitalize
    std::string& string_tolower(std::string& s);
    std::string string_tolower(const std::string& s);
    std::string& string_toupper(std::string& s);
    std::string string_toupper(const std::string& s);
    std::string& string_capitalize(std::string& s, std::function<bool(char)> reset_checker);
    std::string& string_capitalize(std::string& s);
    std::string& string_capitalize(std::string& s, const string& reset_carry);

    // Address string size
    enum {
        ADDR_SIZE       = sizeof(intptr_t) * 2 + 2
    };

    template < typename T >
    inline std::string ptr_str(const T* p) {
        string _ptrs(ADDR_SIZE, '\0');
        if ( sizeof(intptr_t) == 4 ) {
            // 32Bit
            sprintf(&_ptrs[0], "0x%08x", (unsigned int)(intptr_t)p);
        } else {
            // 64Bit
            sprintf(&_ptrs[0], "0x%016lx", (unsigned long)(intptr_t)p);
        }
        return _ptrs;
    }

    // Get the carry size
    template < typename carry_t > inline size_t carry_size( const carry_t& c ) { return sizeof(c); }
    template < > inline size_t carry_size<std::string>(const std::string& c) { return c.size(); }

    template < typename carry_iterator_t >
    inline std::vector< std::string > split(const string& value, carry_iterator_t b, carry_iterator_t e) {
        std::vector< std::string > components_;
        if ( value.size() == 0 ) return components_;
        string::size_type _pos = 0;
        do {
            string::size_type _lastPos = string::npos;
            size_t _carry_size = 0;
            for ( carry_iterator_t i = b; i != e; ++i ) {
                string::size_type _nextCarry = value.find(*i, _pos);
                if ( _nextCarry != string::npos && _nextCarry < _lastPos ) {
                    _lastPos = _nextCarry;
                    _carry_size = carry_size(*i);
                }
            }
            if ( _lastPos == string::npos ) _lastPos = value.size();
            if ( _lastPos > _pos ) {
                string _com = value.substr( _pos, _lastPos - _pos );
                components_.emplace_back(_com);
            }
            _pos = _lastPos + _carry_size;
        } while( _pos < value.size() );
        return components_;
    }

    template < typename carry_t >
    inline std::vector< std::string > split(const string& value, const carry_t& carry) {
        return split(value, std::begin(carry), std::end(carry));
    }

    // Join Items as String
    template< typename ComIterator, typename Connector_t >
    inline std::string join(ComIterator begin, ComIterator end, Connector_t c) {
        std::string _final_string;
        if ( begin == end ) return _final_string;
        std::string _cstr = std::to_string(c);
        auto i = begin, j = (++begin);
        for ( ; j != end; ++i, ++j ) {
            _final_string += std::to_string(*i);
            _final_string += _cstr;
        }
        _final_string += std::to_string(*i);
        return _final_string;
    }

    template < typename Component_t, typename Connector_t >
    inline std::string join(const Component_t& parts, Connector_t c) {
        return join(begin(parts), end(parts), c);
    }

    // Check if string is start with sub string
    bool is_string_start(const std::string& s, const std::string& p);
    std::string check_string_start_and_remove(const std::string& s, const std::string& p);

    // Check if string is end with sub string
    bool is_string_end(const std::string& s, const std::string& p);
    std::string check_string_end_and_remove(const std::string& s, const std::string& p);

    // URL encode and decode
    std::string url_encode(const std::string& str);
    std::string url_decode(const std::string& str);

    // Md5 Sum
    std::string md5(const std::string& str);

    // Base64
    std::string base64_encode(const string& str);
    std::string base64_decode(const string& str);

    // Number Check
    // Check if a given string is a number, the string can be a float string
    bool is_number( const std::string& nstr );

    // Date Convert
    // Default format is "yyyy-mm-dd hh:mm:ss"
    // This function use `strptime` to parse the string
    time_t dtot( const std::string& dstr );
    time_t dtot( const std::string& dstr, const std::string& fmt );

    // Code Format

    // Filter the code and remove all comment
    std::string& code_filter_comment( std::string& code );
}}

#endif 

// Push Chen
