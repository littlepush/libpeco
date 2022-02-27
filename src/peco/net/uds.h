/*
    uds.h
    libpeco
    2022-02-24
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

#ifndef PECO_NET_UDS_H__
#define PECO_NET_UDS_H__

#include "peco/net/adapter.h"
#include "peco/net/utils.h"

namespace peco {

class uds_connector : public connector_adapter {
public:
  virtual ~uds_connector();
  /**
   * @brief Factory
  */
  static std::shared_ptr<uds_connector> create();

  /**
   * @brief Create a tcp connector with a given fd and mark it as connected
  */
  static std::shared_ptr<uds_connector> create(SOCKET_T fd);

  /**
   * @brief Bind address to this socket, different socket type should use
   * different bind slot
  */
  bool bind(const std::string& local_addr);

  /**
   * @brief Connect to peer
  */
  bool connect(const std::string& dest_addr);

  /**
   * @brief Read/Write data from peer socket
  */
  using connector_adapter::read;
  using connector_adapter::write;

protected:
  /**
   * @brief Not Allowed
  */
  uds_connector();
  uds_connector(SOCKET_T fd);
};

class uds_listener : public listener_adapter, public std::enable_shared_from_this<uds_listener> {
public:
  ~uds_listener();
  /**
   * @brief Factory
  */
  static std::shared_ptr<uds_listener> create();

  /**
   * @brief Bind address to this socket, different socket type should use
   * different bind slot
  */
  bool bind(const std::string& local_addr);

  /**
   * @brief Listen on the binded socket
  */
  bool listen(std::function<void(std::shared_ptr<uds_connector>)> accept_slot);

protected:
  /**
   * @brief Listen on the binded socket
  */
  bool listen(listener_adapter::slot_accept_t accept_slot) override;

protected:
  /**
   * @brief Not allowed
  */
  uds_listener();

protected:
  std::string local_path_;
};

} // namespace peco

#endif

// Push Chen
