/*
    redis.h
    libpeco
    2022-02-26
    Push Chen
*/

#pragma once

#ifndef PECO_EXTENSIONS_REDIS_H__
#define PECO_EXTENSIONS_REDIS_H__

#include "peco/pecostd.h"
#include "peco/net/tcp.h"
#include "peco/task/taskqueue.h"
#include "peco/utils/logs.h"
#include <vector>
#include <ostream>

namespace peco {

// Redis redis_command Concat Utility Class
class redis_command {
private:
  size_t cmd_count_;
  std::string cmd_string_;

public:
  // Default C'Str, empty redis_command
  redis_command();

  // Copy And Move
  redis_command(const redis_command &rhs);
  redis_command(redis_command &lhs);
  redis_command &operator=(const redis_command &rhs);
  redis_command &operator=(redis_command &&lhs);

  // Concat
  redis_command &operator+=(const std::string &cmd);

  // Validate
  bool is_validate() const;
  operator bool() const;

  // Commit and return a string to send, after commit
  // all data in this redis_command object will be removed
  std::string commit();

  // Clear the redis_command and reuse
  void clear();

  // Output
  std::string str() const;
  operator std::string() const;
};

// Stream Concat redis_command with argument
redis_command &operator<<(redis_command &rc, const std::string& cmd);
redis_command &operator<<(redis_command &rc, const char* cmd);
template <typename _TyCmd >
redis_command &operator<<(redis_command &rc, const _TyCmd& cmd) {
  return rc << std::to_string(cmd);
}

// Stream ouput
std::ostream &operator<<(std::ostream &os, const redis_command &rc);

// Auto create the command and query
template < typename cmd_t >
redis_command& redis_build_command(redis_command& rc, const cmd_t& c) {
  return (rc << c);
}
template < typename cmd_t, typename... other_cmd_t >
redis_command& redis_build_command(redis_command& rc, const cmd_t& c, const other_cmd_t&... ocs) {
  return redis_build_command(redis_build_command(rc, c), ocs...);
}

// Reply Type, according to the first char of the response.
typedef enum {
  kReplyTypeStatus,
  kReplyTypeError,
  kReplyTypeInteger,
  kReplyTypeBulk,
  kReplyTypeMultibulk,
  kReplyTypeUnknown
} RelpyType;

// Redis Response Object Parser
class redis_object {
public:
  enum { nopos = (size_t)-1, invalidate = (size_t)0 };

private:
  RelpyType reply_type_;
  std::string reply_content_;
  int content_length_;
  size_t sub_object_count_;
  std::vector<redis_object> sub_objects_;

private:
  // Find the first CRLF flag in the given data,
  // when not found, return object::npos
  size_t find_crlf_(const char *data, size_t length);

  // Parse the response data
  // If the type is multibulk, reply_content_ will be empty
  // Return the shifted size of the data.
  size_t parse_response_(const char *data, size_t length);

public:
  // Property References
  const RelpyType &type;
  const std::string &content;
  const std::vector<redis_object> subObjects;

  // Default C'str, with REPLY_TYPE_UNKNOWN
  redis_object();
  // Parse the given data, and set shifted size to parsed_size
  redis_object(const std::string &data, size_t &parsed_size);
  redis_object(const char *data, size_t length, size_t &parsed_size);

  // If an object is unfinished, append more data to it.
  size_t continue_parse(const char *data, size_t length);

  // Copy & Move
  redis_object(const redis_object &rhs);
  redis_object(redis_object &&lhs);
  redis_object &operator=(const redis_object &rhs);
  redis_object &operator=(redis_object &&lhs);

  // cast, when the type is integer, return the content as integer
  // otherwise, return 0
  operator int() const;

  // Validate
  bool is_validate() const;
  // Tell if parse all done
  bool all_get() const;

  // nil/null, when the type is REPLY_TYPE_BULK and content_length is -1, it's
  // nil or when the type is REPLY_TYPE_MULTIBULK and item count is 0, it's nil
  // otherwise, will alywas be not nil.
  bool is_nil() const;

  // When the type is REPLY_TYPE_MULTIBULK and the item count is 0, it's empty
  // list
  bool is_empty() const;

  // Format display
  std::string str(const std::string &prefix = std::string("")) const;
  operator std::string() const;

  // Nan object
  static redis_object nan;
};

// Query Result
typedef std::vector<redis_object> redis_result_t;
namespace redis_result {
// No result
extern const redis_result_t no_result;
} // namespace redis_result

// Format Stream Output
std::ostream &operator<<(std::ostream &os, const redis_object &ro);

// Output all result
std::ostream &operator<<(std::ostream &os, const redis_result_t &r);

// Output to log
log_obj& operator<<(log_obj& lo, const redis_result_t &rc);

/**
 * @brief Redis Connector
*/
class redis_connector : public std::enable_shared_from_this<redis_connector> {
public:
  /**
   * @brief Static Creator
  */
  static std::shared_ptr<redis_connector> create(const peer_t& server_info, const std::string& pwd = "", int db = 0);
  static std::shared_ptr<redis_connector> create(const std::string& server_connect_string);

  ~redis_connector();

  /**
   * @brief Validate
  */
  bool is_validate() const;
  operator bool() const;

  /**
   * @brief Force to connect again
  */
  bool connect();

  /**
   * @brief Disconnect
  */
  void disconnect();

  /**
   * @brief No-keepalive
  */
  void nokeepalive();

  /**
   * @brief Test Connection
  */
  bool connection_test();

  /**
   * @brief Query command
  */
  redis_result_t query(redis_command&& cmd);

  /**
   * @brief Subscribe
  */
  bool subscribe( 
      redis_command&& cmd, 
      std::function< void (const std::string&, const std::string&) > cb
  );

  // Unsubscribe
  void unsubscribe();

  template <typename... cmd_t>
  redis_result_t query(const cmd_t&... c) {
    redis_command _c; 
    redis_build_command(_c, c...);
    return this->query(std::move(_c));
  }
  template <typename... cmd_t>
  bool subscribe( 
      std::function< void(const std::string&, const std::string&) > cb,
      const cmd_t&... c
  ) {
    redis_command _c;
    redis_build_command(_c, c...);
    return this->subscribe(std::move(_c), cb);
  }

private: 
  peer_t                          server_info_;
  std::string                     password_;
  int                             db_;

  // Cache
  int                             last_obj_count_;
  redis_result_t                  last_result_;

  // Inner TCP Connection
  std::shared_ptr<tcp_connector>  conn_;
  task                            keepalive_task_;
  task                            subscribe_task_;
  std::shared_ptr<taskqueue>      cmd_queue_;

protected:
  /**
   * @brief C'str with server info and password
  */
  redis_connector(const peer_t& server_info, const std::string& pwd = "", int db = 0);
  redis_connector(const std::string& server_connect_string);
  // No Copy or Move
  redis_connector(const redis_connector&) = delete;
  redis_connector(redis_connector&&) = delete;
  redis_connector& operator = ( const redis_connector& ) = delete;
  redis_connector& operator = ( redis_connector&& ) = delete;

protected:
  // Tell if all data get for current querying
  bool is_all_get_();
  // Do job task
  bool do_query_job_(const redis_command& cmd);
  // Begin keepalive task
  void begin_keepalive_();
};

} // namespace peco

#endif

// Push Chen
