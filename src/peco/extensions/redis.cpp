/*
    redis.cpp
    libpeco
    2022-02-26
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

#include "peco/extensions/redis.h"
#include "peco/utils.h"

namespace peco {

// Default C'Str, empty command
redis_command::redis_command() : cmd_count_(0) {}

// Copy And Move
redis_command::redis_command(const redis_command &rhs)
  : cmd_count_(rhs.cmd_count_), cmd_string_(rhs.cmd_string_) {}
redis_command::redis_command(redis_command &lhs)
  : cmd_count_(lhs.cmd_count_), cmd_string_(move(lhs.cmd_string_)) {
  lhs.cmd_count_ = 0;
  lhs.cmd_string_.clear();
}
redis_command &redis_command::operator=(const redis_command &rhs) {
  if (this == &rhs)
    return *this;
  cmd_count_ = rhs.cmd_count_;
  cmd_string_ = rhs.cmd_string_;
  return *this;
}
redis_command &redis_command::operator=(redis_command &&lhs) {
  if (this == &lhs)
    return *this;
  cmd_count_ = lhs.cmd_count_;
  cmd_string_ = move(lhs.cmd_string_);
  lhs.cmd_count_ = 0;
  lhs.cmd_string_.clear();
  return *this;
}

// Concat
redis_command &redis_command::operator+=(const std::string &cmd) {
  ++cmd_count_;
  cmd_string_.append("$")
    .append(std::to_string(cmd.size()))
    .append("\r\n")
    .append(cmd)
    .append("\r\n");
  return *this;
}

// Validate
bool redis_command::is_validate() const { return cmd_count_ > 0; }
redis_command::operator bool() const { return cmd_count_ > 0; }

// Commit and return a std::string to send
std::string redis_command::commit() {
  std::string _s(this->str());
  cmd_count_ = 0;
  cmd_string_.clear();
  return _s;
}

// Clear the redis_command and reuse
void redis_command::clear() {
  cmd_count_ = 0;
  cmd_string_.clear();
}

// Output
std::string redis_command::str() const {
  std::string _s = "*" + std::to_string(cmd_count_);
  _s += "\r\n";
  _s += cmd_string_;
  return _s;
}
redis_command::operator std::string() const { return this->str(); }

// Stream Concat redis_Command with argument
redis_command &operator<<(redis_command &rc, const std::string &cmd) { return rc += cmd; }
redis_command &operator<<(redis_command &rc, const char* cmd) { return rc += cmd; }

// Stream ouput
std::ostream &operator<<(std::ostream &os, const redis_command &rc) {
  os << rc.str();
  return os;
}

// Find the first CRLF flag in the given data,
// when not found, return redis_object::npos
size_t redis_object::find_crlf_(const char *data, size_t length) {
  for (size_t _p = 0; _p < (length - 1); ++_p) {
    if (data[_p] != '\r')
      continue;
    if (data[_p + 1] == '\n')
      return _p;
  }
  return redis_object::nopos;
}

// Parse the response data
// Return the shifted size of the data.
size_t redis_object::parse_response_(const char *data, size_t length) {
  size_t _shift = 0;
  if (length < 3)
    return redis_object::invalidate;

  size_t _crlf = redis_object::invalidate;
  // Only try to get type when current object is a new one
  // when current type is multibulk and is not finished, direct to
  // parse the following data.
  if (reply_type_ == kReplyTypeUnknown) {
    // Get Type
    if (data[0] == '+')
      reply_type_ = kReplyTypeStatus;
    else if (data[0] == '-')
      reply_type_ = kReplyTypeError;
    else if (data[0] == ':')
      reply_type_ = kReplyTypeInteger;
    else if (data[0] == '$')
      reply_type_ = kReplyTypeBulk;
    else if (data[0] == '*')
      reply_type_ = kReplyTypeMultibulk;
    else
      return redis_object::invalidate;
    _crlf = this->find_crlf_(data + 1, length - 1);
    if (_crlf == redis_object::nopos)
      return redis_object::invalidate;
    if (reply_type_ == kReplyTypeMultibulk) {
      // Try to get the item count
      std::string _count(data + 1, _crlf);
      int _temp_count = stoi(_count);
      sub_object_count_ = (_temp_count < 0 ? 0u : (size_t)_temp_count);
      _shift = 3 + _count.size();
      content_length_ = 0;
    }
  } else {
    // This type must be multibulk and all_get must be false
    if (reply_type_ != kReplyTypeMultibulk)
      return redis_object::invalidate;
    if (this->all_get() == true)
      return 0;

    // Sub item is not all_get
    if (sub_objects_.size() > 0 && sub_objects_.rbegin()->all_get() == false) {
      _shift = sub_objects_.rbegin()->parse_response_(data, length);
      // Do not need to continue if the last sub item still not finished
      if (sub_objects_.rbegin()->all_get() == false)
        return _shift;
    }
  }

  if (reply_type_ < (size_t)kReplyTypeBulk) {
    reply_content_ = std::string(data + 1, _crlf);
    _shift = 3 + reply_content_.size();
    content_length_ = (int)reply_content_.size();
  } else if (reply_type_ == kReplyTypeBulk) {
    std::string _length(data + 1, _crlf);
    content_length_ = stoi(_length);
    _shift = 3 + _length.size();
    // Check if is nil
    if (content_length_ == -1)
      return _shift;
    // When not nil, get the body
    const char *_body = data + _shift;
    // Unfinished
    if ((int)(length - _shift) < content_length_)
      return redis_object::invalidate;
    reply_content_ = std::string(_body, content_length_);
    _body += content_length_;
    _shift += content_length_;
    // Missing \r\n
    if (length - _shift < 2)
      return redis_object::invalidate;
    // Invalidate end of response
    if (_body[0] != '\r' || _body[1] != '\n')
      return redis_object::invalidate;
    // Skip the CRLF
    _shift += 2;
  } else { // multibulk
    // incase sub_object_count is zero
    if (sub_object_count_ == sub_objects_.size())
      return _shift;
    // If is a new multibulk, the begin point should skip the first *<count>\r\n
    // If is to continue parsing, the _shift is 0, which means _body equals to
    // data
    const char *_body = data + _shift;
    // Try to parse all object till unfinished
    do {
      size_t _sub_shift = redis_object::invalidate;
      redis_object _sro(_body, length - _shift, _sub_shift);
      // Unfinish
      if (_sub_shift == redis_object::invalidate)
        return _shift;
      sub_objects_.emplace_back(_sro);
      _shift += _sub_shift;
      _body += _sub_shift;
      // Sub item not finished
      if (sub_objects_.rbegin()->all_get() == false)
        break;
    } while (sub_objects_.size() != sub_object_count_);
    content_length_ += _shift;
  }
  return _shift;
}

// Default C'str, with kReplyTypeUnknown
redis_object::redis_object()
  : reply_type_(kReplyTypeUnknown), content_length_(-1), sub_object_count_(0),
    type(reply_type_), content(reply_content_), subObjects(sub_objects_) {}
// Parse the given data, and set shifted size to parsed_size
redis_object::redis_object(const std::string &data, size_t &parsed_size)
  : reply_type_(kReplyTypeUnknown), content_length_(-1), sub_object_count_(0),
    type(reply_type_), content(reply_content_), subObjects(sub_objects_) {
  parsed_size = this->parse_response_(data.c_str(), data.size());
  if (parsed_size == redis_object::invalidate)
    this->reply_type_ = kReplyTypeUnknown;
}
redis_object::redis_object(const char *data, size_t length, size_t &parsed_size)
  : reply_type_(kReplyTypeUnknown), content_length_(-1), sub_object_count_(0),
    type(reply_type_), content(reply_content_), subObjects(sub_objects_) {
  parsed_size = this->parse_response_(data, length);
  if (parsed_size == redis_object::invalidate)
    this->reply_type_ = kReplyTypeUnknown;
}

// Copy & Move
redis_object::redis_object(const redis_object &rhs)
  : reply_type_(rhs.reply_type_), reply_content_(rhs.reply_content_),
    content_length_(rhs.content_length_),
    sub_object_count_(rhs.sub_object_count_), sub_objects_(rhs.sub_objects_),
    type(reply_type_), content(reply_content_), subObjects(sub_objects_) {}
redis_object::redis_object(redis_object &&lhs)
  : reply_type_(lhs.reply_type_), reply_content_(move(lhs.reply_content_)),
    content_length_(lhs.content_length_),
    sub_object_count_(lhs.sub_object_count_),
    sub_objects_(move(lhs.sub_objects_)), type(reply_type_),
    content(reply_content_), subObjects(sub_objects_) {
  lhs.reply_type_ = kReplyTypeUnknown;
  lhs.content_length_ = -1;
  lhs.sub_object_count_ = 0;
}
redis_object &redis_object::operator=(const redis_object &rhs) {
  if (this == &rhs)
    return *this;
  reply_type_ = rhs.reply_type_;
  reply_content_ = rhs.reply_content_;
  content_length_ = rhs.content_length_;
  sub_object_count_ = rhs.sub_object_count_;
  sub_objects_ = rhs.sub_objects_;
  return *this;
}
redis_object &redis_object::operator=(redis_object &&lhs) {
  if (this == &lhs)
    return *this;
  reply_type_ = lhs.reply_type_;
  lhs.reply_type_ = kReplyTypeUnknown;
  reply_content_ = move(lhs.reply_content_);
  content_length_ = lhs.content_length_;
  lhs.content_length_ = -1;
  sub_object_count_ = lhs.sub_object_count_;
  lhs.sub_object_count_ = 0;
  sub_objects_ = move(lhs.sub_objects_);
  return *this;
}

// cast, when the type is integer, return the content as integer
// otherwise, return 0
redis_object::operator int() const {
  if (reply_type_ == kReplyTypeInteger)
    return stoi(reply_content_);
  return 0;
}

// Validate
bool redis_object::is_validate() const { return reply_type_ != kReplyTypeUnknown; }

// Tell if parse all done
bool redis_object::all_get() const {
  return (
    reply_type_ == kReplyTypeMultibulk
      ? (sub_object_count_ == sub_objects_.size() &&
         (sub_object_count_ == 0 ? true
                                 : (sub_objects_.rbegin()->all_get() == true)))
      : true);
}

// nil/null, when the type is kReplyTypeBulk and content_length is -1, it's nil
// otherwise, will alywas be not nil.
bool redis_object::is_nil() const {
  return ((type == kReplyTypeBulk && content_length_ == -1) ||
          (type == kReplyTypeMultibulk && sub_object_count_ == 0));
}
// When the type is kReplyTypeMultibulk and the item count is 0, it's empty
// list
bool redis_object::is_empty() const {
  return (type == kReplyTypeMultibulk && sub_object_count_ == 0);
}

// If an object is unfinished, append more data to it.
size_t redis_object::continue_parse(const char *data, size_t length) {
  return this->parse_response_(data, length);
}

// Format display
std::string redis_object::str(const std::string &prefix) const {
  if (type == kReplyTypeStatus || type == kReplyTypeError)
    return prefix + content;
  if (type == kReplyTypeInteger)
    return prefix + "(integer)" + content;
  if (type == kReplyTypeBulk) {
    if (content_length_ == -1)
      return prefix + "(nil)";
    if (content.size() == 0)
      return prefix + "(empty_string)";
    return prefix + "(string)" + content;
  }
  if (type == kReplyTypeMultibulk) {
    std::string _p = prefix + "(multibulk)";
    std::string _subprefix = "  " + prefix;
    std::vector<std::string> _sis;
    _sis.emplace_back(_p);
    for (const auto &ro : sub_objects_) {
      _sis.emplace_back(ro.str(_subprefix));
    }
    return join(_sis.begin(), _sis.end(), "\n", str2str());
  }
  return "unknow";
}
redis_object::operator std::string() const { return this->str(); }

// Format Stream Output
std::ostream &operator<<(std::ostream &os, const redis_object &ro) {
  os << ro.str();
  return os;
}

// The static nan object
redis_object redis_object::nan;
namespace redis_result {
const redis_result_t no_result;
} // namespace redis_result

// Output all result
std::ostream &operator<<(std::ostream &os, const redis_result_t &r) {
  for (const auto &o : r) {
    os << o << std::endl;
  }
  return os;
}

log_obj& operator<<(log_obj& lo, const redis_result_t &rc) {
  for (const auto& r : rc) {
    lo << r.str() << std::endl;
  }
  return lo;
}

// Redis Connector

/**
  * @brief Static Creator
*/
std::shared_ptr<redis_connector> redis_connector::create(const peer_t& server_info, const std::string& pwd, int db) {
  return std::shared_ptr<redis_connector>(new redis_connector(server_info, pwd, db));
}
std::shared_ptr<redis_connector> redis_connector::create(const std::string& server_connect_string) {
  return std::shared_ptr<redis_connector>(new redis_connector(server_connect_string));
}

/**
  * @brief C'str with server info and password
*/
redis_connector::redis_connector(const peer_t& server_info, const std::string& pwd, int db)
  : server_info_(server_info), password_(pwd), db_(db), cmd_queue_(taskqueue::create()) {}
redis_connector::redis_connector(const std::string& server_connect_string) 
  : db_(0), cmd_queue_(taskqueue::create()) {
  size_t schema_pos = server_connect_string.find("://");
  if (schema_pos != std::string::npos) schema_pos += 3;
  std::string conn_info = server_connect_string.substr(schema_pos);
  auto path_components = split(conn_info, "/");
  // have db info
  if (path_components.size() > 1) {
    if (is_number(path_components[1])) {
      db_ = std::stoi(path_components[1]);
    }
  }

  auto pass_components = split(path_components[0], "@");
  std::string server_info = *pass_components.rbegin();
  if (server_info.find(':') == std::string::npos) {
    server_info += ":6379";
  }
  server_info_ = server_info;

  // No password
  if (pass_components.size() == 1) return;
  size_t pass_length = path_components[0].size() - 1 - pass_components.rbegin()->size();
  size_t pass_begin = 0;
  if (path_components[0][0] == '\'' && path_components[0][0] == '\"') {
    pass_begin += 1;
    pass_length -= 2;
  }
  password_ = path_components[0].substr(pass_begin, pass_length);
}
redis_connector::~redis_connector() {
  // cancel keep live task
  keepalive_task_.cancel();
  subscribe_task_.cancel();
  conn_ = nullptr;
}

/**
  * @brief Validate
*/
bool redis_connector::is_validate() const {
  return conn_ != nullptr && conn_->is_connected();
}
redis_connector::operator bool() const {
  return this->is_validate();
}

/**
  * @brief Force to connect again
*/
bool redis_connector::connect() {
  if (!server_info_) return false;
  // already connected
  if (this->is_validate()) return true;
  // connect to server
  if (!conn_) {
    conn_ = tcp_connector::create();
  }

  bool connected = false;
  do {
    if (!conn_->connect(server_info_)) {
      break;
    }

    if (password_.size() > 0) {
      redis_command c_auth;
      c_auth << "AUTH" << password_;
      if (!conn_->write(c_auth.commit())) {
        break;
      }
      auto r_auth = conn_->read();
      if (r_auth.status != kNetOpStatusOK) {
        break;
      }
      // check password response
      if (r_auth.data != "+OK\r\n") {
        break;
      }
    }

    if (db_ != 0) {
      redis_command c_select;
      c_select << "SELECT" << std::to_string(db_);
      if (!conn_->write(c_select.commit())) {
        break;
      }
      auto r_select = conn_->read();
      if (r_select.status != kNetOpStatusOK) {
        break;
      }
      if (r_select.data != "+OK\r\n") {
        break;
      }
    }
    connected = true;
  } while (false);

  if (!connected) {
    conn_ = nullptr;
    return false;
  }

  this->begin_keepalive_();
  return true;
}

/**
  * @brief Disconnect
*/
void redis_connector::disconnect() {
  keepalive_task_.cancel();
  conn_ = nullptr;
  // reset the keepalive task
  keepalive_task_ = task(kInvalidateTaskId);
}

/**
  * @brief Test Connection
*/
bool redis_connector::connection_test() {
  if (conn_ == nullptr) return false;
  // copy the conn so during connection testing, conn_ won't be destroied
  auto ref_conn = conn_;
  redis_command c_ping;
  c_ping << "PING";
  if (!ref_conn->write(c_ping.commit())) {
    return false;
  }
  auto r_ping = ref_conn->read();
  if (r_ping.status != kNetOpStatusOK) {
    return false;
  }
  if (r_ping.data != "+PONG\r\n") {
    return false;
  }
  return true;
}

/**
  * @brief Query command
*/
redis_result_t redis_connector::query(redis_command&& cmd) {
  redis_result_t r = redis_result::no_result;
  auto self = this->shared_from_this();
  cmd_queue_->sync([cmd = std::move(cmd), &r, self]() {
    // do inner query
    if (self->do_query_job_(std::move(cmd))) {
      r = std::move(self->last_result_);
    }
  });
  return r;
}

/**
  * @brief Subscribe
*/
bool redis_connector::subscribe( 
    redis_command&& cmd, 
    std::function< void (const std::string&, const std::string&) > cb
) {
  // already 
  if (subscribe_task_.is_alive()) return false;
  // cancel keep alive task
  if (keepalive_task_.task_id() != kInvalidateTaskId) {
    keepalive_task_.cancel();
    keepalive_task_ = task(kInvalidateTaskId);
  }

  std::weak_ptr<redis_connector> weak_self = this->shared_from_this();
  subscribe_task_ = loop::shared()->run([weak_self, cmd = std::move(cmd), cb]() {
    while (!task::this_task().is_cancelled()) {
      auto strong_self = weak_self.lock();
      // the redis connector has already been destroied
      if (!strong_self) break;
      if (!strong_self->is_validate()) break;
      // Send subscribe command
      if (!strong_self->do_query_job_(cmd)) break;
      // ACK for subscribe
      if (strong_self->last_result_.size() == 0) break;
      redis_command empty_cmd;
      while (!task::this_task().is_cancelled()) {
        if (!strong_self->do_query_job_(empty_cmd)) {
          break;
        }
        if (strong_self->last_result_.size() == 0) {
          break;
        }
        if (strong_self->last_result_.size() == 3) {
          if (strong_self->last_result_[0].content != "message") break;
          cb(strong_self->last_result_[1].content, strong_self->last_result_[2].content);
        } else if (strong_self->last_result_.size() == 4) {
          if (strong_self->last_result_[0].content != "pmessage") break;
          cb(strong_self->last_result_[2].content, strong_self->last_result_[3].content);
        } else {
          break;
        }
      }
      if (strong_self->is_validate()) {
        empty_cmd += "UNSUBSCRIBE";
        ignore_result(strong_self->do_query_job_(empty_cmd));
      }
    }
    if (auto strong_self = weak_self.lock()) {
      strong_self->begin_keepalive_();
    }
  });
  return true;
}

// Unsubscribe
void redis_connector::unsubscribe() {
  this->begin_keepalive_();
}


// Tell if all data get for current querying
bool redis_connector::is_all_get_() {
  if (this->last_obj_count_ == -1) return false;
  if ((int)this->last_result_.size() != this->last_obj_count_) return false;
  return this->last_result_.rbegin()->all_get();
}
// Do job task
bool redis_connector::do_query_job_(const redis_command& cmd) {
  last_obj_count_ = -1;
  last_result_.clear();
  if (cmd.is_validate()) {
    // write failed
    if (!conn_->write(cmd.str())) {
      conn_ = nullptr;
      return false;
    }
  }

  std::string sbuf;
  do {
    auto r = conn_->read();
    // No data, maybe subscribe
    if (r.status == kNetOpStatusTimedout) continue;
    if (r.status == kNetOpStatusFailed) {
      // broken connection
      conn_ = nullptr;
      return false;
    }
    sbuf.append(r.data);

    const char *resp_body = sbuf.c_str();
    uint32_t resp_size = sbuf.size();
    uint32_t resp_shifted_size = 0;

    do {
      if ( this->last_result_.size() > 0 ) {
        redis_object& _lastro = (*this->last_result_.rbegin());
        if ( _lastro.all_get() == false ) {
          // Continue parse
          size_t _shift = _lastro.continue_parse(resp_body, resp_size);
          sbuf.erase(0, _shift);
          resp_body = sbuf.c_str();
          // check if still not finished
          if ( _lastro.all_get() == false ) break;    // continue to read
          // Now we get all, break to main loop and will get all received flag
          if ( this->last_obj_count_ == (int)this->last_result_.size() ) break;
        }
      }
      // Get the response redis_object count
      if ( this->last_obj_count_ == -1 ) {
        // start a new parser
        if ( resp_body[0] == '*' ) {
          if ( resp_size < 4 ) break;
          // Multibulk
          resp_body++; resp_shifted_size++;
          std::string _scount;
          while ( resp_shifted_size < resp_size && *resp_body != '\r' ) {
            _scount += *resp_body;
            ++resp_body; ++resp_shifted_size;
          }
          // Maybe the server is error.
          if ( *resp_body != '\r' ) { return false; }
          this->last_obj_count_ = std::stoi(_scount);
          resp_body += 2; resp_shifted_size += 2;
        } else {
          this->last_obj_count_ = 1;
        }
      }
      // Try to parse all object till unfinished
      if ( this->last_obj_count_ > 0 ) {
        do {
          size_t _shift = 0;
          redis_object _ro(resp_body, resp_size - resp_shifted_size, _shift);
          if ( _shift == redis_object::invalidate ) break;
          this->last_result_.emplace_back(_ro);
          resp_shifted_size += _shift;
          resp_body += _shift;
          if ( this->last_result_.rbegin()->all_get() == false ) break;
        } while ( (int)this->last_result_.size() != this->last_obj_count_ );
      }
      sbuf.erase(0, resp_shifted_size);
    } while (false);
  } while(!this->is_all_get_());

  // Tell the querying task to go on
  return true;
}
// Begin keepalive task
void redis_connector::begin_keepalive_() {
  std::weak_ptr<redis_connector> weak_self = this->shared_from_this();
  if (subscribe_task_.task_id() != kInvalidateTaskId) {
    subscribe_task_.cancel();
    subscribe_task_ = task(kInvalidateTaskId);
  }

  if (!this->is_validate()) return;
  keepalive_task_ = loop::shared()->run_loop([weak_self]() {
    if (auto strong_self = weak_self.lock()) {
      // all correct
      if (strong_self->connection_test()) return;
      // disconnected or d'str-ed
      if (task::this_task().is_cancelled()) return;
      // clear the status
      strong_self->conn_ = nullptr;
      strong_self->keepalive_task_ = task(kInvalidateTaskId);
      task::this_task().cancel();
    }
  }, PECO_TIME_S(10));
}

} // namespace peco

// Push Chen
