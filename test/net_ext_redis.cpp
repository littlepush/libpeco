/*
    net_ext_redis.cpp
    libpeco
    2022-03-02
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

#include "peco.h"
#include <cstdlib>

void test_case() {
  const char* redis_password = getenv("PECO_TEST_REDIS_PASSWORD");
  const char* redis_server = getenv("PECO_TEST_REDIS_SERVER");
  const char* redis_db = getenv("PECO_TEST_REDIS_DB");
  std::string redis_connection_string = "redis://";
  if (redis_password != NULL) {
    redis_connection_string += std::string(redis_password);
    redis_connection_string += "@";
  }
  if (redis_server != NULL) {
    redis_connection_string += std::string(redis_server);
  } else {
    redis_connection_string += std::string("127.0.0.1");
  }
  if (redis_db != NULL) {
    redis_connection_string += "/";
    redis_connection_string += std::string(redis_db);
  }
  auto rc = peco::redis_connector::create(redis_connection_string);
  if (!rc->connect()) {
    peco::log::error << "failed to connect to redis server" << std::endl;
    peco::current_loop::exit(1);
    return;
  }
  peco::log::info << "connected to redis server" << std::endl;
  rc->query("SET", "PECO_TEST", "net_ext_redis");
  auto r = rc->query("GET", "PECO_TEST");
  peco::log::info << r << std::endl;
  peco::current_loop::exit(0);
}

int main() {
  peco::current_loop::run(test_case);
  return peco::current_loop::main();
}