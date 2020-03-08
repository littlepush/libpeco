/*
    stringutil.cpp
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

#include <peco/utils/stringutil.h>
#include <zlib.h>
#include <fcntl.h>
#include <cstdio>

extern "C" {
    int PEUtils_Autoconf() { return 0; }
}

namespace pe { namespace utils { 
    // String trim white space
    std::string& left_trim(std::string& s) {
        s.erase(
            s.begin(),
            std::find_if(
                s.begin(),
                s.end(),
                [](int c) { return !std::isspace(c); }
            )
        );
        return s;
    }
    std::string& right_trim(std::string& s) {
        s.erase(
            std::find_if(
                s.rbegin(),
                s.rend(),
                [](int c) { return !std::isspace(c); }
            ).base(),
            s.end()
        );
        return s;
    }
    std::string& trim(std::string& s) { return left_trim(right_trim(s)); }

    // Convert to lower, to upper, and capitalize
    std::string& string_tolower(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }
    std::string string_tolower(const std::string& s) {
        std::string _s(s); return string_tolower(_s);
    }
    std::string& string_toupper(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }
    std::string string_toupper(const std::string& s) {
        std::string _s(s); return string_toupper(_s);
    }
    std::string& string_capitalize(std::string& s, std::function<bool(char)> reset_checker) {
        bool _is_first_char = true;
        if ( reset_checker == nullptr ) reset_checker = [](char) { return false; };
        for ( size_t i = 0; i < s.size(); ++i ) {
            if ( reset_checker(s[i]) ) { _is_first_char = true; continue; }
            if ( _is_first_char ) {
                s[i] = toupper(s[i]); _is_first_char = false;
            } else {
                s[i] = tolower(s[i]);
            }
        }
        return s;
    }
    std::string& string_capitalize(std::string& s) {
        return string_capitalize(s, [](char c) -> bool { return (bool)isspace(c); });
    }
    std::string& string_capitalize(std::string& s, const string& reset_carry) {
        return string_capitalize(s, [&reset_carry](char c) { return reset_carry.find(c) != string::npos; });
    }

    // Check if string is start with sub string
    bool is_string_start(const std::string& s, const std::string& p) {
        if ( s.size() >= p.size() ) {
            for ( size_t i = 0; i < p.size(); ++i ) {
                if ( s[i] != p[i] ) return false;
            }
            return true;
        }
        return false;
    }
    std::string check_string_start_and_remove(const std::string& s, const std::string& p) {
        if ( is_string_start(s, p) ) return s.substr(p.size());
        return s;
    }

    // Check if string is end with sub string
    bool is_string_end(const std::string& s, const std::string& p) {
        if ( s.size() >= p.size() ) {
            size_t _begin = s.size() - p.size();
            for ( size_t i = _begin; i < s.size(); ++i ) {
                if ( s[i] != p[i - _begin] ) return false;
            }
            return true;
        }
        return false;
    }
    std::string check_string_end_and_remove(const std::string& s, const std::string& p) {
        if ( is_string_end(s, p) ) return s.substr(0, (s.size() - p.size()));
        return s;
    };  

    unsigned char __url_encode_convert_to_hex(unsigned char x) {
        return x > 9 ? x + 55 : x + 48;
    }
    unsigned char __url_encode_convert_from_hex(unsigned char x) {
        return (
            ( x >= 'A' && x <= 'Z' ) ? x - 'A' + 10 : 
            ( x >= 'a' && x <= 'z' ) ? x - 'a' + 10 : 
            ( x >= '0' && x <= '9' ) ? x - '0' : x
            );
    }
    std::string url_encode(const std::string& str) {
        std::string _temp;
        for ( const auto c : str ) {
            if ( std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ) _temp += c;
            else if ( c == ' ' ) _temp += '+';
            else {
                _temp += '%';
                _temp += __url_encode_convert_to_hex((unsigned char)c >> 4);
                _temp += __url_encode_convert_to_hex((unsigned char)c % 16);
            }
        }
        return _temp;
    }
    std::string url_decode(const std::string& str) {
        std::string _temp;
        for ( size_t i = 0; i < str.size(); ++i ) {
            if ( str[i] == '+' ) _temp += ' ';
            else if ( str[i] == '%' ) {
                if ( i + 2 >= str.size() ) break;
                unsigned char _h = __url_encode_convert_from_hex((unsigned char)str[++i]);
                unsigned char _l = __url_encode_convert_from_hex((unsigned char)str[++i]);
                _temp += ((_h << 4) + _l);
            } else _temp += str[i];
        }
        return _temp;
    }

    std::string md5(const std::string& str) {
        string _result(32, '\0');
        unsigned char _md[16] = {0};
    // #if PZC_TARGET_LINUX
        MD5_CTX _ctx;

        MD5_Init( &_ctx );
        MD5_Update( &_ctx, str.c_str(), str.size() );
        MD5_Final( _md, &_ctx );

    // #endif
    // #if PZC_TARGET_MAC
    //     CC_MD5_CTX _ctx;
    //     CC_MD5_Init( &_ctx );
    //     CC_MD5_Update( &_ctx, str.c_str(), str.size() );
    //     CC_MD5_Final( _md, &_ctx );
    // #endif
        for ( int i = 0; i < 16; ++i ) {
            sprintf( &_result[0] + i * 2, "%02x", (uint8_t)_md[i]);
        }
        return _result;
    }

    std::string base64_encode(const string& str) {
        int _wrap_width = 0;
        static char _lookup_dict[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        unsigned long long _input_str_len = (unsigned long long)str.size();
        const unsigned char *_input_str = (const unsigned char *)str.c_str();
        
        unsigned long long _max_output_len = (_input_str_len / 3 + 1) * 4;
        _max_output_len += _wrap_width ? (_max_output_len / _wrap_width) * 2: 0;
        string _result((size_t)_max_output_len, '\0');
        char *_output_str = &_result[0];
        
        unsigned long long i = 0;
        unsigned long long _output_str_len = 0;
        if ( _input_str_len >= 2 ) {
            for (i = 0; i < _input_str_len - 2; i += 3)
            {
                _output_str[_output_str_len++] = _lookup_dict[(_input_str[i] & 0xFC) >> 2];
                _output_str[_output_str_len++] = _lookup_dict[((_input_str[i] & 0x03) << 4) | ((_input_str[i + 1] & 0xF0) >> 4)];
                _output_str[_output_str_len++] = _lookup_dict[((_input_str[i + 1] & 0x0F) << 2) | ((_input_str[i + 2] & 0xC0) >> 6)];
                _output_str[_output_str_len++] = _lookup_dict[_input_str[i + 2] & 0x3F];
                
                //add line break
                if (_wrap_width && (_output_str_len + 2) % (_wrap_width + 2) == 0) 
                {
                    _output_str[_output_str_len++] = '\r';
                    _output_str[_output_str_len++] = '\n';
                }
            }
        }
        
        //handle left-over data
        if (i == _input_str_len - 2)
        {
            // = termiator
            _output_str[_output_str_len++] = _lookup_dict[(_input_str[i] & 0xFC) >> 2];
            _output_str[_output_str_len++] = _lookup_dict[((_input_str[i] & 0x03) << 4) | ((_input_str[i + 1] & 0xF0) >> 4)];
            _output_str[_output_str_len++] = _lookup_dict[(_input_str[i + 1] & 0x0F) << 2];
            _output_str[_output_str_len++] =   '=';
        }
        else if (i == _input_str_len - 1)
        {
            // == termnator
            _output_str[_output_str_len++] = _lookup_dict[(_input_str[i] & 0xFC) >> 2];
            _output_str[_output_str_len++] = _lookup_dict[(_input_str[i] & 0x03) << 4];
            _output_str[_output_str_len++] = '=';
            _output_str[_output_str_len++] = '=';
        }
        
        if (_output_str_len >= 4)
        {
            //truncatedata to match actual output lengt
            _result.resize(_output_str_len);
        }
        else
        {
            _result.clear();
        }
        return _result;
    }

    std::string base64_decode(const string& str) {
        static const char _loopup_dict[] =
        {
            99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
            99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 62, 99, 99, 99, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 99, 99, 99, 99, 99, 99,
            99,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 99, 99, 99, 99, 99,
            99, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 99, 99, 99, 99, 99
        };
        
        size_t _input_str_len = str.size();
        const unsigned char *_input_str = (const unsigned char *)str.c_str();
        
        size_t _max_output_len = (_input_str_len / 4 + 1) * 3;
        string _result(_max_output_len, '\0');
        unsigned char *_output_str = (unsigned char *)&_result[0];
        
        int accumulator = 0;
        size_t _output_str_len = 0;
        unsigned char accumulated[] = {0, 0, 0, 0};
        for (size_t i = 0; i < _input_str_len; i++)
        {
            unsigned char decoded = _loopup_dict[_input_str[i] & 0x7F];
            if (decoded != 99)
            {
                accumulated[accumulator] = decoded;
                if (accumulator == 3)
                {
                    _output_str[_output_str_len++] = (accumulated[0] << 2) | (accumulated[1] >> 4);
                    _output_str[_output_str_len++] = (accumulated[1] << 4) | (accumulated[2] >> 2);
                    _output_str[_output_str_len++] = (accumulated[2] << 6) | accumulated[3];
                }
                accumulator = (accumulator + 1) % 4;
            }
        }
        
        //handle left-over data
        if (accumulator > 0) _output_str[_output_str_len] = (accumulated[0] << 2) | (accumulated[1] >> 4);
        if (accumulator > 1) _output_str[++_output_str_len] = (accumulated[1] << 4) | (accumulated[2] >> 2);
        if (accumulator > 2) _output_str_len++;
        
        //truncate data to match actual output length
        _result.resize(_output_str_len);
        return _result;
    }

    // Number Check
    // Check if a given string is a number, the string can be a float string
    bool is_number( const std::string& nstr ) {
        if ( string_tolower(nstr) == "nan" ) return false;
        char *_mend = 0;
        double _dmo = std::strtod(nstr.c_str(), &_mend);
        return ! ( _mend == nstr.c_str() || _dmo == HUGE_VAL );
    }

    // Date Convert
    // Default format is "yyyy-mm-dd hh:mm:ss"
    // This function use `strptime` to parse the string
    time_t dtot( const std::string& dstr ) {
        return dtot( dstr, "%Y-%m-%d %H:%M:%S");
    }
    time_t dtot( const std::string& dstr, const std::string& fmt ) {
        std::tm _tm = {};
        strptime(dstr.c_str(), fmt.c_str(), &_tm);
        return std::mktime(&_tm);
    }

    // Filter the code and remove all comment
    std::string& code_filter_comment( std::string& code ) {
        while ( true ) {
            size_t _b = code.find("/*");
            if ( _b != std::string::npos ) {
                size_t _e = code.find("*/", _b);
                if ( _e == std::string::npos ) return code;
                code.erase(_b, _e - _b + 2);
            } else break;
        }
        size_t _last = 0;
        size_t _last_quote = 0;
        bool _is_string = false;
        while ( _last != std::string::npos && _last < code.size() ) {
            size_t _b = code.find("//", _last);
            _last = _b + 1;
            if ( _b == std::string::npos ) break;
            for ( size_t _lq = _last_quote; _lq < _b; ++_lq ) {
                if ( code[_lq] == '"' ) {
                    _last_quote = _lq;
                    _is_string = !_is_string;
                }
            }
            // Now '//' is in a string, may be is a part of url
            if ( _is_string ) continue;
            size_t _e = code.find("\n", _b);
            if ( _e == std::string::npos ) {
                code.erase(_b);
                break;
            } else {
                // Erase the line left
                code.erase(_b, _e - _b + 1);
            }
        }
        return code;
    }

}}

// Push Chen 
