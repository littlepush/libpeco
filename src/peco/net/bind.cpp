/*
    bind.cpp
    libpeco
    2022-02-21
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

#include "peco/net/bind.h"
#include "peco/utils.h"

#if !PECO_TARGET_WIN
#include <fcntl.h>
#include <sys/un.h>
#endif

namespace peco {

peer_bind::peer_bind(const peer_t& addr): bind_addr_(addr) { }
peer_bind::peer_bind(const std::string& addr): bind_addr_(addr) { }
peer_bind::peer_bind(const peer_bind& other) : bind_addr_(other.bind_addr_) { }
peer_bind::peer_bind(peer_bind&& other): bind_addr_(std::move(other.bind_addr_)) { }
peer_bind& peer_bind::operator = (const peer_bind& other) {
  if (this == &other) return *this;
  bind_addr_ = other.bind_addr_;
  return *this;
}
peer_bind& peer_bind::operator = (peer_bind&& other) {
  if (this == &other) return *this;
  bind_addr_ = std::move(other.bind_addr_);
  return *this;
}


/**
 * @brief Bind on the given socket
*/
bool peer_bind::operator()(SOCKET_T fd) {
  if (! (bool)bind_addr_ ) {
    return false;
  }

  if (!net_utils::reusable(fd, true)) {
    return false;
  }

  // Bind to the address
  struct sockaddr_in sock_addr;
  memset((char *)&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(bind_addr_.port);
  sock_addr.sin_addr.s_addr = (uint32_t)bind_addr_.ip;

  if ( ::bind(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1 ) {
    log::error << "Failed to bind tcp socket on: " << bind_addr_
        << ", " << get_sys_error( errno ) << std::endl;
    return false;
  }
  return true;
}

path_bind::path_bind(const std::string& addr): bind_path_(addr) { }
path_bind::path_bind(const path_bind& other) : bind_path_(other.bind_path_) { }
path_bind::path_bind(path_bind&& other): bind_path_(std::move(other.bind_path_)) { }
path_bind& path_bind::operator = (const path_bind& other) {
  if (this == &other) return *this;
  bind_path_ = other.bind_path_;
  return *this;
}
path_bind& path_bind::operator = (path_bind&& other) {
  if (this == &other) return *this;
  bind_path_ = std::move(other.bind_path_);
  return *this;
}

/**
 * @brief Bind on the given socket
*/
bool path_bind::operator()(SOCKET_T fd) {
#if !PECO_TARGET_WIN
  std::shared_ptr<mode_t> m = std::shared_ptr<mode_t>(
    new mode_t(umask(S_IXUSR | S_IXGRP | S_IXOTH)), 
    [](mode_t* pm) {
      umask(*pm);
      delete pm;
    }
  );
#endif
  // Get the path part, and create it with "mkdir -p"
  std::string folder = dir_name(bind_path_);
  if (!rek_make_dir(folder)) {
    log::error << "Failed to create UDS socket path: " << folder << std::endl;
    return false;
  }

  struct sockaddr_un  addr;
  bzero(&addr, sizeof(addr));
  strcpy(addr.sun_path, bind_path_.c_str());

  addr.sun_family = AF_UNIX;
#if PECO_TARGET_APPLE
  int addrlen = bind_path_.size() + sizeof(addr.sun_family) + sizeof(addr.sun_len);
  addr.sun_len = sizeof(addr);
#else
  int addrlen = bind_path_.size() + sizeof(addr.sun_family);
#endif
  unlink(bind_path_.c_str());

  if ( ::bind(fd, (struct sockaddr*)&addr, addrlen) < 0 ) {
    log::error << "Failed to bind uds socket on: " << bind_path_
        << ", " << get_sys_error( errno ) << std::endl;
    return false;
  }
  return true;
}


} // namespace peco

// Push Chen
