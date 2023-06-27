/*
    argparser.h
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

#pragma once

#ifndef PECO_ARGPARSER_H__
#define PECO_ARGPARSER_H__

#include "pecostd.h"
#include "basic/any.h"

#include <map>
#include <vector>

namespace peco {

// class argparser {
// public:

// protected:
//   std::map<std::string, peco::any> args_;
// };

class argparser {
public:
  typedef std::function<void(std::string &&)> set_arg_t;

protected:
  std::map<std::string, set_arg_t> setter_map_;
  std::vector<std::string> individual_args_;

protected:
  // Singleton
  static argparser &_();
  argparser();

public:
  // no copy
  argparser(const argparser &ref) = delete;
  argparser(argparser &&rref) = delete;
  argparser &operator=(const argparser &) = delete;
  argparser &operator=(argparser &&) = delete;

  // Set the parser info
  static void set_parser(const std::string &key, set_arg_t setter);
  static void set_parser(const std::string &key, std::string &getter);
  static void set_parser(const std::string &key, const std::string &attr,
                         set_arg_t setter);
  static void set_parser(const std::string &key, const std::string &attr,
                         std::string &getter);

  // Do the parser
  static bool parse(int argc, char *argv[]);

  // Do the parse from a config file
  static bool parse(const std::string &config_file);

  // Get all individual args
  static std::vector<std::string> individual_args();

  // Clear
  static void clear();
};

} // namespace peco

#endif

// Push Chen
//
