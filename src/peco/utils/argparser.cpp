/*
    argparser.cpp
    PECoUtils
    2018-04-09
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

#include <peco/peutils.h>

namespace pe { namespace utils {
    // Singleton
    argparser& argparser::_() { static argparser _; return _; }
    argparser::argparser() { }

    // Set the parser info
    void argparser::set_parser(const string& key, set_arg_t setter) {
        argparser::set_parser(key, "", setter);
    }
    void argparser::set_parser(const string& key, string& getter) {
        argparser::set_parser(key, "", getter);
    }
    void argparser::set_parser(const string& key, const string& attr, set_arg_t setter) {
        _().setter_map_[key] = setter;
        if ( attr.size() > 0 ) _().setter_map_[attr] = setter;
    }
    void argparser::set_parser(const string& key, const string& attr, string& getter) {
        string* _pgetter = &getter;
        argparser::set_parser(key, attr, [_pgetter](string && arg) {
            *_pgetter = move(arg);
        });
    }

    // Get all individual args
    std::vector< std::string > argparser::individual_args() {
        return _().individual_args_;
    }

    // Do the parser
    bool argparser::parse(int argc, char* argv[]) {
        static vector< string > _remarks = {"=", " "};
        for ( int i = 1; i < argc; ++i ) {
            string _arg = argv[i];
            string _key_with_equal;
            if ( is_string_start(_arg, "--") ) {
                _key_with_equal = _arg.substr(2);
            } else if ( is_string_start(_arg, "-") ) {
                _key_with_equal = _arg.substr(1);
            } else {
                _().individual_args_.push_back(_arg);
                continue;
            }

            auto _fmt_parts = split(_key_with_equal, _remarks);
            auto s = _().setter_map_.find(_fmt_parts[0]);
            if ( s == _().setter_map_.end() ) {
                std::cerr << "Unknow command: " << _arg << std::endl;
                return false;
            }
            string _value;
            if ( _fmt_parts.size() > 1 ) {
                auto _b = _fmt_parts.begin();
                ++_b;
                _value = join(_b, _fmt_parts.end(), " ");
            };
            s->second(move(_value));
        }
        return true;
    }

    // Do the parse from a config file
    bool argparser::parse( const std::string& config_file ) {
        static vector< string > _remarks = {"=", " ", ":", "\t"};
        std::ifstream _cfg(config_file);
        if ( !_cfg ) return false;

        std::string _line;
        while ( std::getline(_cfg, _line) ) {
            pe::utils::trim(_line);
            if ( _line.size() == 0 ) continue;
            if ( _line[0] == '#' ) continue;    // Comment Line

            // Sub comment in the line
            size_t _comment_pos = _line.find("#");
            if ( _comment_pos != std::string::npos ) {
                // Do have comment
                _line = _line.substr(0, _comment_pos);
                // Remove the end whitespace
                pe::utils::trim(_line);
            }

            auto _fmt_parts = pe::utils::split(_line, _remarks);
            auto s = _().setter_map_.find(_fmt_parts[0]);
            if ( s == _().setter_map_.end() ) {
                std::cerr << "Unknow config key: " << _fmt_parts[0] << std::endl;
                continue;
            }
            size_t _skip_size = _fmt_parts[0].size();
            while ( _skip_size < _line.size() ) {
                if ( 
                    _line[_skip_size] == '=' || 
                    _line[_skip_size] == ' ' ||
                    _line[_skip_size] == ':' ||
                    _line[_skip_size] == '\t'
                    ) ++_skip_size;
                else break;
            }
            std::string _value = (_skip_size == _line.size() ? 
                "" : _line.substr(_skip_size));
            pe::utils::trim(_value);
            s->second(move(_value));
        }
        return true;
    }

    // Clear
    void argparser::clear() {
        _().setter_map_.clear();
        _().individual_args_.clear();
    }
}}

// Push Chen
// 
