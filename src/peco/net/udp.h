/*
    udp.h
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

#ifndef PECO_NET_UDP_H__
#define PECO_NET_UDP_H__

#include "peco/net/adapter.h"
#include "peco/net/bind.h"

namespace peco {

class udp_connector : public connector_adapter {
public:
  virtual ~udp_connector();
  /**
   * @brief Factory
  */
  static std::shared_ptr<udp_connector> create();

  /**
   * @brief Create a tcp connector with a given fd and mark it as connected
  */
  static std::shared_ptr<udp_connector> create(SOCKET_T fd);

  /**
   * @brief Bind address to this socket, different socket type should use
   * different bind slot
  */
  bool bind(const peer_t& local_addr);
  bool bind(const std::string& local_addr);

  /**
   * @brief Connect to peer
  */
  bool connect(const peer_t& dest_addr);
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
  udp_connector();
  udp_connector(SOCKET_T fd);
};

/**
 * @brief Package data recived from a listener
*/
class udp_packet {
public:
  /**
   * @brief Get peer info of this packet
  */
  const peer_t& peer_info() const;

  /**
   * @brief Get the data in packet
  */
  const std::string& data() const;

  /**
   * @brief Write data back to origin addr
  */
  bool write(const char* data, size_t length);
  bool write(const std::string& data);
protected:
  friend class udp_listener;

  /**
   * @brief Static create for udp_listener
  */
  static std::shared_ptr<udp_packet> create(SOCKET_T fd, const peer_t& addr, std::string&& pkt);
  /**
   * @brief Build a udp packet
  */
  udp_packet(SOCKET_T fd, const peer_t& addr, std::string&& pkt);

protected:
  SOCKET_T fd_;
  peer_t source_addr_;
  std::string pkt_;
};

class udp_listener : public listener_adapter, public std::enable_shared_from_this<udp_listener> {
public:
  ~udp_listener();
  /**
   * @brief Factory
  */
  static std::shared_ptr<udp_listener> create();

  /**
   * @brief Bind address to this socket, different socket type should use
   * different bind slot
  */
  bool bind(const std::string& local_addr);

  /**
   * @brief Listen on the binded socket
  */
  bool listen(std::function<void(std::shared_ptr<udp_connector>)> accept_slot);

  /**
   * @brief Listen on the binded socket in packet mode
  */
  bool listen(std::function<void(std::shared_ptr<udp_packet>)> accept_slot);

protected:
  /**
   * @brief Listen on the binded socket
  */
  bool listen(listener_adapter::slot_accept_t accept_slot) override;

protected:
  /**
   * @brief Not allowed
  */
  udp_listener();
protected:
  /**
   * @brief Cached bind slot operator
  */
  peer_bind bind_slot_;
  bool listened_ = false;
};

} // namespace peco

#endif

// Push Chen
