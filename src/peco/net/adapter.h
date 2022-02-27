/*
    adapter.h
    libpeco
    2022-02-18
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

#ifndef PECO_NET_ADAPTER_H__
#define PECO_NET_ADAPTER_H__

#include "peco/net/utils.h"
#include "peco/task/taskdef.h"

namespace peco {

/**
 * @brief Status of the network operation
*/
enum NetOpStatus {
  /**
   * @brief Get incoming or write all data
  */
  kNetOpStatusOK,
  /**
   * @brief Operation has been timedout
  */
  kNetOpStatusTimedout,
  /**
   * @brief The socket has been broken
  */
  kNetOpStatusFailed
};

struct inet_incoming {
  /**
   * @brief The status of the reading operation
  */
  NetOpStatus status;
  /**
   * @brief The data received
  */
  std::string data;

  /**
   * @brief C'stors
  */
  inet_incoming(NetOpStatus s, const std::string& d);
  inet_incoming(NetOpStatus s, std::string&& d);

  /**
   * @brief Move
  */
  inet_incoming(inet_incoming&& other);
  inet_incoming& operator = (inet_incoming&& other);

  /**
   * @brief Bool cast
  */
  operator bool() const;
};

class inet_adapter {
public:
  typedef std::function<bool(SOCKET_T)>   slot_bind_t;
public:
  /**
   * @brief Copy is not allowed
  */
  inet_adapter(const inet_adapter&) = delete;
  inet_adapter& operator = (const inet_adapter&) = delete;

  /**
   * @brief Move is allowed
  */
  inet_adapter(inet_adapter&& other);
  inet_adapter& operator = (inet_adapter&& other);

  /**
   * @brief Virtual d'stor, will save close the socket
  */
  virtual ~inet_adapter();

  /**
   * @brief Bind address to this socket, different socket type should use
   * different bind slot
  */
  bool bind(slot_bind_t bind_slot);
protected:
  /**
   * @brief The default c'stor is not allowed to be invoked outside the class
  */
  inet_adapter(SOCKET_T related_fd = INVALIDATE_SOCKET);

  /**
   * @brief Create the adapter's socket
  */
  void create_socket(SocketType sock_type);

  /**
   * @brief Final close of the socket fd.
  */
  void close();
protected:
  SOCKET_T fd_ = INVALIDATE_SOCKET;
};

class connector_adapter : public inet_adapter {
public:
  /**
   * @brief Typedef inherit
  */
  typedef inet_adapter::slot_bind_t       slot_bind_t;

  /**
   * @brief Real connnect function type
  */
  typedef std::function<bool(SOCKET_T)>   slot_connect_t;
public:
  virtual ~connector_adapter();

  /**
   * @brief Connect to peer
  */
  bool connect(slot_connect_t connect_slot);

  /**
   * @brief Read data from peer socket
  */
  inet_incoming read(duration_t timedout = PECO_TIME_S(10), size_t bufsize = 4096);

  /**
   * @brief Write data to peer socket
  */
  bool write(const char* data, size_t length, duration_t timedout = PECO_TIME_S(10));
  bool write(std::string&& data, duration_t timedout = PECO_TIME_S(10));
  bool write(const std::string& data, duration_t timedout = PECO_TIME_S(10));

  /**
   * @brief Tell if the connector is already connected to peer
  */
  bool is_connected() const;
private:
  bool is_connected_ = false;
protected:
  connector_adapter(SOCKET_T related_fd = INVALIDATE_SOCKET, bool connected = false);
  /**
   * @brief Reset connect status
  */
  void reset_connect_();
};

class listener_adapter : public inet_adapter {
public:
  /**
   * @brief Typedef inherit
  */
  typedef inet_adapter::slot_bind_t             slot_bind_t;

  /**
   * @brief Slot for accept new incoming
  */
  typedef std::function<void(SOCKET_T, const peer_t&)> slot_accept_t;

public:
  virtual ~listener_adapter();

  /**
   * @brief Listen on the binded socket
  */
  virtual bool listen(slot_accept_t accept_slot) = 0;
};

} // namespace peco

#endif

// Push Chen
