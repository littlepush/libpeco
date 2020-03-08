/*
    redis.cpp
    PECoNet
    2019-05-22
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/conns/tcp.h>
#include <peco/conet/protos/redis.h>
#include <peco/peutils.h>
#include <peco/conet/conns/netadapter.h>
#include <peco/conet/protos/dns.h>

namespace pe { namespace co { namespace net { namespace proto { namespace redis {

    // Default C'Str, empty command
    command::command() : cmd_count_(0) { }

    // Copy And Move
    command::command( const command& rhs ) : 
        cmd_count_(rhs.cmd_count_), cmd_string_(rhs.cmd_string_) 
    { }
    command::command( command& lhs ) : 
        cmd_count_(lhs.cmd_count_), cmd_string_(move(lhs.cmd_string_))
    {
        lhs.cmd_count_ = 0;
        lhs.cmd_string_.clear();
    }
    command& command::operator = (const command& rhs) {
        if ( this == &rhs ) return *this;
        cmd_count_ = rhs.cmd_count_;
        cmd_string_ = rhs.cmd_string_;
        return *this;
    }
    command& command::operator = (command&& lhs) {
        if ( this == &lhs ) return *this;
        cmd_count_ = lhs.cmd_count_;
        cmd_string_ = move(lhs.cmd_string_);
        lhs.cmd_count_ = 0;
        lhs.cmd_string_.clear();
        return *this;
    }

    // Concat
    command& command::operator += (const string& cmd) {
        ++cmd_count_;
        cmd_string_
            .append("$")
            .append(to_string(cmd.size()))
            .append("\r\n")
            .append(cmd)
            .append("\r\n");
        return *this;
    }

    // Validate
    bool command::is_validate() const { return cmd_count_ > 0; }
    command::operator bool() const { return cmd_count_ > 0; }

    // Commit and return a string to send
    string command::commit() {
        string _s(this->str());
        cmd_count_ = 0;
        cmd_string_.clear();
        return _s;
    }
    
    // Clear the command and reuse
    void command::clear() {
        cmd_count_ = 0;
        cmd_string_.clear();
    }

    // Output
    string command::str() const { 
        string _s = "*" + to_string(cmd_count_);
        _s += "\r\n";
        _s += cmd_string_;
        return _s;
    }
    command::operator std::string() const { return this->str(); }

    // Stream Concat Command with argument
    command& operator << (command& rc, const string& cmd) { return rc += cmd; }

    // Stream ouput
    ostream& operator << ( ostream& os, const command& rc ) { 
        os << rc.str(); return os;
    }

    // Find the first CRLF flag in the given data,
    // when not found, return object::npos
    size_t object::find_crlf_(const char* data, size_t length) {
        for ( size_t _p = 0; _p < (length - 1); ++_p ) {
            if ( data[_p] != '\r' ) continue;
            if ( data[_p + 1] == '\n' ) return _p;
        }
        return object::nopos;
    }

    // Parse the response data
    // Return the shifted size of the data.
    size_t object::parse_response_(const char* data, size_t length) {
        size_t _shift = 0;
        if ( length < 3 ) return object::invalidate;

        size_t _crlf = object::invalidate;
        // Only try to get type when current object is a new one
        // when current type is multibulk and is not finished, direct to
        // parse the following data. 
        if ( reply_type_ == REPLY_TYPE_UNKNOWN ) {
            // Get Type
            if ( data[0] == '+' ) reply_type_ = REPLY_TYPE_STATUS;
            else if ( data[0] == '-' ) reply_type_ = REPLY_TYPE_ERROR;
            else if ( data[0] == ':' ) reply_type_ = REPLY_TYPE_INTEGER;
            else if ( data[0] == '$' ) reply_type_ = REPLY_TYPE_BULK;
            else if ( data[0] == '*' ) reply_type_ = REPLY_TYPE_MULTIBULK;
            else return object::invalidate;
            _crlf = this->find_crlf_(data + 1, length - 1);
            if ( _crlf == object::nopos ) return object::invalidate;
            if ( reply_type_ == REPLY_TYPE_MULTIBULK ) {
                // Try to get the item count
                string _count(data + 1, _crlf);
                int _temp_count = stoi(_count);
                sub_object_count_ = (_temp_count < 0 ? 0u : (size_t)_temp_count);
                _shift = 3 + _count.size();
                content_length_ = 0;
            }
        } else {
            // This type must be multibulk and all_get must be false
            if ( reply_type_ != REPLY_TYPE_MULTIBULK ) return object::invalidate;
            if ( this->all_get() == true ) return 0;
            
            // Sub item is not all_get
            if ( sub_objects_.size() > 0 && sub_objects_.rbegin()->all_get() == false ) {
                _shift = sub_objects_.rbegin()->parse_response_(data, length);
                // Do not need to continue if the last sub item still not finished
                if ( sub_objects_.rbegin()->all_get() == false ) return _shift;
            }
        }

        if ( reply_type_ < (size_t)REPLY_TYPE_BULK ) {
            reply_content_ = string(data + 1, _crlf);
            _shift = 3 + reply_content_.size();
            content_length_ = (int)reply_content_.size();
        } else if ( reply_type_ == REPLY_TYPE_BULK) {
            string _length(data + 1, _crlf);
            content_length_ = stoi(_length);
            _shift = 3 + _length.size();
            // Check if is nil
            if ( content_length_ == -1 ) return _shift;
            // When not nil, get the body
            const char* _body = data + _shift;
            // Unfinished
            if ( (int)(length - _shift) < content_length_ ) return object::invalidate;
            reply_content_ = string(_body, content_length_);
            _body += content_length_;
            _shift += content_length_;
            // Missing \r\n
            if ( length - _shift < 2 ) return object::invalidate;
            // Invalidate end of response
            if ( _body[0] != '\r' || _body[1] != '\n' ) return object::invalidate;
            // Skip the CRLF
            _shift += 2;
        } else {    // multibulk
            // incase sub_object_count is zero
            if ( sub_object_count_ == sub_objects_.size() ) return _shift;
            // If is a new multibulk, the begin point should skip the first *<count>\r\n
            // If is to continue parsing, the _shift is 0, which means _body equals to data
            const char* _body = data + _shift;
            // Try to parse all object till unfinished
            do {
                size_t _sub_shift = object::invalidate;
                object _sro(_body, length - _shift, _sub_shift);
                // Unfinish
                if ( _sub_shift == object::invalidate ) return _shift;
                sub_objects_.emplace_back(_sro);
                _shift += _sub_shift;
                _body += _sub_shift;
                // Sub item not finished
                if ( sub_objects_.rbegin()->all_get() == false ) break;
            } while ( sub_objects_.size() != sub_object_count_ );
            content_length_ += _shift;
        }
        return _shift;
    }

    // Default C'str, with REPLY_TYPE_UNKNOWN
    object::object() : 
        reply_type_(REPLY_TYPE_UNKNOWN), content_length_(-1), sub_object_count_(0),
        type(reply_type_), content(reply_content_), subObjects(sub_objects_)
    { }
    // Parse the given data, and set shifted size to parsed_size
    object::object(const string& data, size_t& parsed_size) : 
        reply_type_(REPLY_TYPE_UNKNOWN), content_length_(-1), sub_object_count_(0),
        type(reply_type_), content(reply_content_), subObjects(sub_objects_)
    {
        parsed_size = this->parse_response_(data.c_str(), data.size());
        if ( parsed_size == object::invalidate ) this->reply_type_ = REPLY_TYPE_UNKNOWN;
    }
    object::object(const char* data, size_t length, size_t& parsed_size) : 
        reply_type_(REPLY_TYPE_UNKNOWN), content_length_(-1), sub_object_count_(0),
        type(reply_type_), content(reply_content_), subObjects(sub_objects_)
    {
        parsed_size = this->parse_response_(data, length);
        if ( parsed_size == object::invalidate ) this->reply_type_ = REPLY_TYPE_UNKNOWN;
    }

    // Copy & Move
    object::object(const object& rhs) : 
        reply_type_(rhs.reply_type_), reply_content_(rhs.reply_content_), content_length_(rhs.content_length_),
        sub_object_count_(rhs.sub_object_count_), sub_objects_(rhs.sub_objects_),
        type(reply_type_), content(reply_content_), subObjects(sub_objects_)
    { }
    object::object(object&& lhs) : 
        reply_type_(lhs.reply_type_), 
        reply_content_(move(lhs.reply_content_)), 
        content_length_(lhs.content_length_),
        sub_object_count_(lhs.sub_object_count_),
        sub_objects_(move(lhs.sub_objects_)),
        type(reply_type_), 
        content(reply_content_),
        subObjects(sub_objects_)
    { 
        lhs.reply_type_ = REPLY_TYPE_UNKNOWN;
        lhs.content_length_ = -1;
        lhs.sub_object_count_ = 0;
    }
    object& object::operator = (const object& rhs) {
        if ( this == &rhs ) return *this;
        reply_type_ = rhs.reply_type_;
        reply_content_ = rhs.reply_content_;
        content_length_ = rhs.content_length_;
        sub_object_count_ = rhs.sub_object_count_;
        sub_objects_ = rhs.sub_objects_;
        return *this;
    }
    object& object::operator = (object&& lhs) {
        if ( this == &lhs ) return *this;
        reply_type_ = lhs.reply_type_;
        lhs.reply_type_ = REPLY_TYPE_UNKNOWN;
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
    object::operator int() const {
        if ( reply_type_ == REPLY_TYPE_INTEGER ) return stoi(reply_content_);
        return 0;
    }

    // Validate
    bool object::is_validate() const { return reply_type_ != REPLY_TYPE_UNKNOWN; }

    // Tell if parse all done
    bool object::all_get() const {
        return (
            reply_type_ == REPLY_TYPE_MULTIBULK ?
            (
                sub_object_count_ == sub_objects_.size() && 
                (sub_object_count_ == 0 ? true : (
                    sub_objects_.rbegin()->all_get() == true
                ))
            ) : 
            true
        );
    }

    // nil/null, when the type is REPLY_TYPE_BULK and content_length is -1, it's nil
    // otherwise, will alywas be not nil.
    bool object::is_nil() const { 
        return (
            (type == REPLY_TYPE_BULK && content_length_ == -1) || 
            (type == REPLY_TYPE_MULTIBULK && sub_object_count_ == 0)
        );
    }
    // When the type is REPLY_TYPE_MULTIBULK and the item count is 0, it's empty list
    bool object::is_empty() const {
        return (type == REPLY_TYPE_MULTIBULK && sub_object_count_ == 0);
    }

    // If an object is unfinished, append more data to it.
    size_t object::continue_parse(const char* data, size_t length) {
        return this->parse_response_(data, length);
    }

    // Format display
    string object::str(const string& prefix) const {
        if ( type == REPLY_TYPE_STATUS || type == REPLY_TYPE_ERROR ) return prefix + content;
        if ( type == REPLY_TYPE_INTEGER ) return prefix + "(integer)" + content;
        if ( type == REPLY_TYPE_BULK ) {
            if ( content_length_ == -1 ) return prefix + "(nil)";
            if ( content.size() == 0 ) return prefix + "(empty_string)";
            return prefix + "(string)" + content;
        }
        if ( type == REPLY_TYPE_MULTIBULK ) {
            string _p = prefix + "(multibulk)";
            string _subprefix = "  " + prefix;
            vector<string> _sis;
            _sis.emplace_back(_p);
            for ( const auto& ro : sub_objects_ ) {
                _sis.emplace_back(ro.str(_subprefix));
            }
            return pe::utils::join(_sis, "\n");
        }
        return "unknow";
    }
    object::operator std::string() const { return this->str(); }

    // Format Stream Output
    ostream& operator << ( ostream& os, const object& ro ) {
        os << ro.str(); return os;
    }

    // The static nan object
    object object::nan;
    const result_t no_result;

    // Output all result
    ostream& operator << ( ostream& os, const result_t& r ) {
        for ( auto& o : r ) {
            os << o << std::endl;
        }
        return os;
    }

}}}}}

namespace pe { namespace co { namespace net { namespace redis {

    // Connector
    bool connector::is_all_get_() {
        if ( this->last_obj_count_ == -1 ) return false;
        if ( (int)this->last_result_.size() != this->last_obj_count_ ) return false;
        return this->last_result_.rbegin()->all_get();
    }
    // Do job task
    bool connector::do_query_job_( tcpadapter* adapter, command&& cmd ) {

        // std::cout << "query: " << cmd.str() << std::endl;
        // If not validate, just wait for reading
        // usually this is a subscribe session
        if ( cmd.is_validate() ) {
            if ( !adapter->write( cmd.commit(), std::chrono::milliseconds(100) ) ) {
                return false;
            }
        }

        std::string _sbuf;
        do {

            // std::cout << "pending to read" << std::endl;
            auto _r = adapter->read( );
            if ( _r.first == op_timedout ) continue;
            if ( _r.first == op_failed ) return false;
            // std::cout << "response: " << _r.second << std::endl;
            // Append to current pending buffer
            _sbuf.append(_r.second);

            const char *_body = _sbuf.c_str();
            uint32_t _allsize = _sbuf.size();
            uint32_t _shifted_size = 0;

            do {
                if ( this->last_result_.size() > 0 ) {
                    object& _lastro = (*this->last_result_.rbegin());
                    if ( _lastro.all_get() == false ) {
                        // Continue parse
                        size_t _shift = _lastro.continue_parse(_body, _allsize);
                        _sbuf.erase(0, _shift);
                        _body = _sbuf.c_str();
                        // check if still not finished
                        if ( _lastro.all_get() == false ) break;    // continue to read
                        // Now we get all, break to main loop and will get all received flag
                        if ( this->last_obj_count_ == (int)this->last_result_.size() ) break;
                    }
                }
                // Get the response object count
                if ( this->last_obj_count_ == -1 ) {
                    // start a new parser
                    if ( _body[0] == '*' ) {
                        if ( _allsize < 4 ) break;
                        // Multibulk
                        _body++; _shifted_size++;
                        string _scount;
                        while ( _shifted_size < _allsize && *_body != '\r' ) {
                            _scount += *_body;
                            ++_body; ++_shifted_size;
                        }
                        // Maybe the server is error.
                        if ( *_body != '\r' ) { return false; }
                        this->last_obj_count_ = stoi(_scount);
                        _body += 2; _shifted_size += 2;
                    } else {
                        this->last_obj_count_ = 1;
                    }
                }
                // Try to parse all object till unfinished
                if ( this->last_obj_count_ > 0 ) {
                    do {
                        size_t _shift = 0;
                        object _ro(_body, _allsize - _shifted_size, _shift);
                        if ( _shift == object::invalidate ) break;
                        this->last_result_.emplace_back(_ro);
                        _shifted_size += _shift;
                        _body += _shift;
                        if ( this->last_result_.rbegin()->all_get() == false ) break;
                    } while ( (int)this->last_result_.size() != this->last_obj_count_ );
                }
                _sbuf.erase(0, _shifted_size);
            } while ( false );
        } while( ! this->is_all_get_() );

        // Tell the querying task to go on
        return true;
    }

    void connector::connect_( ) {
        if ( keepalive_ != NULL ) {
            task_exit(keepalive_);
        }
        this_loop.do_job([this]() {
            tcpadapter _adapter;
            do {
                parent_task::guard _pg;
                if ( !_adapter.connect( this->server_info_.str() ) ) return;

                if ( this->password_.size() > 0 ) {
                    command _c_auth;
                    _c_auth << "AUTH" << this->password_;
                    if ( ! _adapter.write( _c_auth.commit() ) ) return;
                    auto _r = _adapter.read();
                    if ( _r.first != op_done ) return;
                    if ( _r.second != "+OK\r\n" ) return;
                }

                // Try to ping
                command _c_ping;
                _c_ping << "PING";
                if ( !_adapter.write(_c_ping.commit()) ) return;
                auto _r = _adapter.read();
                if ( _r.first != op_done ) return;
                if ( _r.second != "+PONG\r\n" ) return;

                if ( this->db_ != 0 ) {
                    command _c_select;
                    _c_select << "SELECT" << std::to_string(this->db_);
                    if ( ! _adapter.write(_c_select.commit()) ) return;
                    auto _s = _adapter.read();
                    if ( _s.first != op_done ) return;
                    if ( _s.second != "+OK\r\n" ) return;
                }
                // Now we know we do connected to the redis server
                this->connected_ = true;
                _pg.job_done();
            } while ( false );

            // Begin Job Loop
            this->equeue_.begin_loop([this, &_adapter]( command&& cmd, result_t& res) {
                this->action_time_ = task_time_now();
                this->last_result_.clear();
                this->last_obj_count_ = -1;
                if ( !this->do_query_job_(&_adapter, std::move(cmd)) ) return false;
                res = this->last_result_;
                return true;
            });

            this->connected_ = false;
        });
        this_task::holding();
        cond_connected_.notify();
        keepalive_ = this_loop.do_loop([this]() {
            this->connection_test();
        }, std::chrono::seconds(30));
    }

    connector::connector(const peer_t& server_info, const string& pwd, int db) : 
        server_info_(server_info), password_(pwd), db_(db),
        connecting_(false), connected_(false), keepalive_(NULL), last_obj_count_(-1), 
        action_time_(task_time_now())
    { }
    connector::~connector() {
        this->equeue_.stop_loop();
        if ( keepalive_ != NULL ) task_exit(keepalive_);
    }

    // Disconnect
    void connector::disconnect() {
        this->equeue_.stop_loop();
    }

    // No-keepalive
    void connector::nokeepalive() {
        if ( keepalive_ != NULL ) task_exit(keepalive_);
        keepalive_ = NULL;
    }

    // Validate
    bool connector::is_validate() const { return this->connected_; }
    connector::operator bool() const { return this->is_validate(); }

    // Test Connection
    bool connector::connection_test() {
        command _ping;
        _ping << "PING";
        result_t _res;
        if ( !this->equeue_.ask(std::move(_ping), _res) ) return false;
        return _res[0].content == "PONG";
    }

    // Get the last result
    const result_t& connector::last_result() const { return this->last_result_; }

    // Query
    result_t connector::query( command&& cmd ) {
        result_t _res;
        // Query the command, if failed, reconnect
        // only when the command is validate
        if ( !this->equeue_.ask(std::move(cmd), _res) && cmd.is_validate() ) {
            // Re-connect
            if ( !this->connect() && this->connecting_ ) {
                if ( !cond_connected_.wait([this]() { return this->connected_ == true; }) ) {
                    // Connection failed
                    return _res;
                } else {
                    // Query again
                    this->equeue_.ask(std::move(cmd), _res);
                }
            }
            // connection broken
        }
        return _res;
    }

    // Get the load average
    const task_time_t& connector::action_time() const { return action_time_; }

    // Force to connect again
    bool connector::connect(const peer_t& server_info, const string& pwd, int db) {
        if ( this->connected_ ) return true;
        if ( this->connecting_ ) return false;
        this->connecting_ = true;
        if ( server_info != peer_t::nan ) {
            server_info_ = server_info; // Update the server info
        }
        if ( pwd.size() != 0 ) {
            password_ = pwd;    // Update the password
        }
        if ( db != -1 ) {
            db_ = db;
        }
        this->connect_();
        this->connecting_ = false;
        return this->connected_;
    }

    // Connection Group
    group::group( const peer_t& server_info, const std::string& pwd, size_t cc ) 
        : lcon_(cc, NULL), db_(0)
    {
        sinfo_ = server_info;
        spwd_ = pwd;

        assert( cc != 0 );
        for ( size_t i = 0; i < cc; ++i ) {
            lcon_[i] = new connector(server_info, pwd, db_);
            lcon_[i]->connect();
        }
    }
    group::group( const std::string& server_connect_string, size_t cc )
        : lcon_(cc, NULL), db_(0) 
    {
        size_t _schema_pos = server_connect_string.find("://");
        if ( _schema_pos == std::string::npos ) {
            // No redis://
        } else {
            _schema_pos += 3;   // skip ://
        }
        std::string _infos = server_connect_string.substr(_schema_pos);
        auto _components = utils::split(_infos, "/");
        if ( _components.size() > 1 ) {
            db_ = std::stoi(_components[1]);
        }
        auto _pah = utils::split(_components[0], "@");
        std::string _server = *_pah.rbegin();
        std::string _password;
        if ( _pah.size() > 1 ) {
            _pah.pop_back();
            _password = utils::join(_pah, "@");
            if ( _password[0] == '\'' && *_password.rbegin() == '\'' ) {
                _password = _password.substr(1, _password.size() - 2);
            } else if ( _password[0] == '\"' && *_password.rbegin() == '\"' ) {
                _password = _password.substr(1, _password.size() - 2);
            }
        }
        auto _ap = utils::split(_server, ":");
        uint16_t _port = 6379;
        if ( _ap.size() > 1 ) {
            _port = (uint16_t)std::stoi(_ap[1]);
        }
        net::ip_t _ip = net::get_hostname(_ap[0]);
        net::peer_t _sinfo(_ip, _port);

        sinfo_ = _sinfo;
        spwd_ = _password;

        for ( size_t i = 0; i < cc; ++ i ) {
            lcon_[i] = new connector(_sinfo, _password, db_);
            lcon_[i]->connect();
        }
    }
    group::~group() {
        for ( connector *_pconn : lcon_ ) {
            delete _pconn;
        }
    }

    int group::dbIndex() const { return db_; }
    // Switch db index
    void group::select_db( int db ) {
        db_ = db;
        for ( size_t i = 0; i < lcon_.size(); ++i ) {
            command _c;
            _c << "SELECT" << std::to_string(db);
            lcon_[i]->query(std::move(_c));
        }
    }

    // Get the lowest load average connector
    connector& group::lowest_load_connector() {
        size_t _i = 0;
        for ( size_t i = 0; i < lcon_.size(); ++ i ) {
            if ( !lcon_[i]->is_validate() ) {
                // Try to re-connect
                connector *_pc = lcon_[i];
                this_loop.do_job([_pc, this]() {
                    if ( _pc->connect() ) {
                        command _c;
                        _c << "SELECT" << std::to_string(this->db_);
                        _pc->query(std::move(_c));
                    }
                });
                continue;
            }
            if ( lcon_[i]->action_time() < lcon_[_i]->action_time() ) {
                _i = i;
            }
        }

        return *lcon_[_i];
    }
    // Query
    result_t group::query( command&& cmd ) {
        return lowest_load_connector().query( std::move(cmd) );
    }

    // Subscribe
    task * group::subscribe( 
        command&& cmd, 
        std::function< void (const std::string&, const std::string&) > cb
    ) {
        if ( cb == nullptr ) return NULL;

        // We need to create a new session
        return this_loop.do_job([cmd, cb, this]() {
            task *_tt = this_task::get_task();
            _tt->reserved_flags.flag0 = 1;
            while ( _tt->reserved_flags.flag0 == 1 ) {
                _tt->arg = NULL;
                // Keep the connection alive
                // reconnect when the socket has reach
                // system's timeout and been dropped
                command _cmd = cmd;
                std::shared_ptr< connector > _sconn = std::make_shared<connector>(
                    sinfo_, spwd_, db_
                );
                if ( !_sconn->connect() ) break;
                // an subscribe session does not allow to
                // send any command beside "subscribe", so
                // we must cancel the keepalive task
                _sconn->nokeepalive();

                auto _r = _sconn->query( std::move(_cmd) );
                if ( _r.size() == 0 ) break;

                // Set current loop's connection
                _tt->arg = (void *)(_sconn.get());

                command _empty_cmd;
                while ( _tt->reserved_flags.flag0 == 1 ) {
                    auto _r = _sconn->query( std::move(_empty_cmd) );
                    // Connection closed
                    if ( _r.size() == 0 ) break;
                    // Invalidate message
                    if ( _r.size() == 3 ) {
                        if ( _r[0].content != "message" ) break;
                        // Call the notification
                        cb( _r[1].content, _r[2].content );
                    } else if ( _r.size() == 4 ) {
                        if ( _r[0].content != "pmessage" ) break;
                        // Call the notification
                        cb( _r[2].content, _r[3].content );
                    } else {
                        break;
                    }
                }
                if ( _sconn->is_validate() ) {
                    // Unsubscribe the session
                    _empty_cmd += "UNSUBSCRIBE";
                    ignore_result( _sconn->query( std::move(_empty_cmd) ) );
                }
            }
        });
    }

    // Unsubscribe
    void group::unsubscribe( task * subtask ) {
        if ( subtask == NULL ) return;
        subtask->reserved_flags.flag0 = 0;
        if ( subtask->arg == NULL ) return;
        // Disconnect the connector if it's not null
        ((connector *)(subtask->arg))->disconnect();
    }

    // Check the result of a redis query
    bool result_check( const result_t r, result_check_rule_t&& rule ) {
        if ( r.size() < rule.min_count ) return false;
        if ( rule.must_count != 0 && r.size() != rule.must_count ) return false;
        for ( auto& ir : rule.item_rule ) {
            if ( ir.index >= r.size() ) continue;
            if ( r[ir.index].is_nil() ) return false;
            if ( r[ir.index].content != ir.value ) return false;
        }
        for ( auto& nr : rule.nil_rule ) {
            if ( nr.index >= r.size() ) continue;
            if ( r[nr.index].is_nil() != nr.can_nil ) return false;
        }
        return true;
    }

}}}}

// Push Chen
//
