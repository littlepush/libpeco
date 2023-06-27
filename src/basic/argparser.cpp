/*
    argparser.cpp
    libpeco
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

#include "basic/argparser.h"
#include "basic/stringutil.h"
#include "basic/any.h"
#include <fstream>

namespace peco {
// Singleton
argparser &argparser::_() {
  static argparser _;
  return _;
}
argparser::argparser() {}

// Set the parser info
void argparser::set_parser(const std::string &key, set_arg_t setter) {
  argparser::set_parser(key, "", setter);
}
void argparser::set_parser(const std::string &key, std::string &getter) {
  argparser::set_parser(key, "", getter);
}
void argparser::set_parser(const std::string &key, const std::string &attr,
                           set_arg_t setter) {
  _().setter_map_[key] = setter;
  if (attr.size() > 0)
    _().setter_map_[attr] = setter;
}
void argparser::set_parser(const std::string &key, const std::string &attr,
                           std::string &getter) {
  std::string *_pgetter = &getter;
  argparser::set_parser(
    key, attr, [_pgetter](std::string &&arg) { *_pgetter = std::move(arg); });
}

// Get all individual args
std::vector<std::string> argparser::individual_args() {
  return _().individual_args_;
}

// Do the parser
bool argparser::parse(int argc, char *argv[]) {
  std::map<std::string, any> args_map;
  for (int i = 1; i < argc; ++i) {
    std::string arg_data = argv[i];
    if (is_string_start(arg_data, "-")) {
      // this is a key
      std::string key_may_with_value;
      if (is_string_start(arg_data, "--")) {
        key_may_with_value = arg_data.substr(2);
      } else {
        key_may_with_value = arg_data.substr(1);
      }
      auto parts = split(key_may_with_value, "=");
      if (parts.size() > 1) {
        std::string value = parts[1];
        if (parts.size() > 2) {
          value = join(parts.begin() + 1, parts.end(), "=", str2str());
        }
        args_map[parts[0]] = any(value);
      } else {
        if ((i + 1) < argc) {
          if (argv[i + 1][0] == '-') {
            args_map[key_may_with_value] = any();
          } else {
            args_map[key_may_with_value] = std::string(argv[i + 1]);
            i += 1;
          }
        } else {
          // no more args
          args_map[key_may_with_value] = any();
        }
      }
    } else {
      _().individual_args_.push_back(arg_data);
    }
  }
  for (auto& kv : args_map) {
    auto s = _().setter_map_.find(kv.first);
    if (s == _().setter_map_.end()) {
      std::cerr << "Unknow command: " << kv.first << std::endl;
      return false;
    }
    std::string value;
    if (kv.second.has_value()) {
      value = any_cast<std::string>(kv.second);
    }
    s->second(std::move(value));
  }
  return true;
}

// Do the parse from a config file
bool argparser::parse(const std::string &config_file) {
  static std::vector<std::string> _remarks = {"=", " ", ":", "\t"};
  std::ifstream _cfg(config_file);
  if (!_cfg)
    return false;

  std::string _line;
  while (std::getline(_cfg, _line)) {
    peco::trim(_line);
    if (_line.size() == 0)
      continue;
    if (_line[0] == '#')
      continue; // Comment Line

    // Sub comment in the line
    size_t _comment_pos = _line.find("#");
    if (_comment_pos != std::string::npos) {
      // Do have comment
      _line = _line.substr(0, _comment_pos);
      // Remove the end whitespace
      peco::trim(_line);
    }

    auto _fmt_parts = peco::split(_line, _remarks);
    auto s = _().setter_map_.find(_fmt_parts[0]);
    if (s == _().setter_map_.end()) {
      std::cerr << "Unknow config key: " << _fmt_parts[0] << std::endl;
      continue;
    }
    size_t _skip_size = _fmt_parts[0].size();
    while (_skip_size < _line.size()) {
      if (_line[_skip_size] == '=' || _line[_skip_size] == ' ' ||
          _line[_skip_size] == ':' || _line[_skip_size] == '\t')
        ++_skip_size;
      else
        break;
    }
    std::string _value =
      (_skip_size == _line.size() ? "" : _line.substr(_skip_size));
    peco::trim(_value);
    s->second(std::move(_value));
  }
  return true;
}

// Clear
void argparser::clear() {
  _().setter_map_.clear();
  _().individual_args_.clear();
}
} // namespace peco

// Push Chen
//
