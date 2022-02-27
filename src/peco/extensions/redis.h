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
redis_command &operator<<(redis_command &rc, const std::string &cmd);

// Stream ouput
std::ostream &operator<<(std::ostream &os, const redis_command &rc);

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
typedef std::vector<redis_object> result_t;
// No result
extern const result_t no_result;

// Format Stream Output
std::ostream &operator<<(std::ostream &os, const redis_object &ro);

// Output all result
std::ostream &operator<<(std::ostream &os, const result_t &r);

} // namespace peco

#endif

// Push Chen
