/*
    redis.h
    PECoNet
    2019-05-22
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_NET_PROTOS_REDIS_H__
#define PE_CO_NET_PROTOS_REDIS_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/conns/tcp.h>
#include <peco/conet/conns/netadapter.h>

namespace pe { namespace co { namespace net { namespace proto { namespace redis {

    // Redis Command Concat Utility Class
    class command {
    private: 
        size_t              cmd_count_;
        string              cmd_string_;

    public: 
        // Default C'Str, empty command
        command();

        // Copy And Move
        command( const command& rhs );
        command( command& lhs );
        command& operator = (const command& rhs);
        command& operator = (command&& lhs);

        // Concat
        command& operator += (const string& cmd);

        // Validate
        bool is_validate() const;
        operator bool() const;

        // Commit and return a string to send, after commit
        // all data in this command object will be removed
        string commit();

        // Clear the command and reuse
        void clear();

        // Output
        string str() const;
        operator std::string() const;
    };

    // Stream Concat Command with argument
    command& operator << (command& rc, const string& cmd);

    // Stream ouput
    ostream& operator << ( ostream& os, const command& rc );

    // Reply Type, according to the first char of the response.
    typedef enum {
        REPLY_TYPE_STATUS,
        REPLY_TYPE_ERROR,
        REPLY_TYPE_INTEGER,
        REPLY_TYPE_BULK,
        REPLY_TYPE_MULTIBULK,
        REPLY_TYPE_UNKNOWN
    } replyType_t;

    // Redis Response Object Parser
    class object {
    public: 
        enum { nopos = (size_t)-1, invalidate = (size_t)0 };

    private: 
        replyType_t                     reply_type_;
        string                          reply_content_;
        int                             content_length_;
        size_t                          sub_object_count_;
        vector< object >                sub_objects_;

    private: 

        // Find the first CRLF flag in the given data,
        // when not found, return object::npos
        size_t find_crlf_(const char* data, size_t length);

        // Parse the response data
        // If the type is multibulk, reply_content_ will be empty
        // Return the shifted size of the data.
        size_t parse_response_(const char* data, size_t length);

    public: 

        // Property References
        const replyType_t&              type;
        const string&                   content;
        const vector< object >          subObjects;

        // Default C'str, with REPLY_TYPE_UNKNOWN
        object();
        // Parse the given data, and set shifted size to parsed_size
        object(const string& data, size_t& parsed_size);
        object(const char* data, size_t length, size_t& parsed_size);

        // If an object is unfinished, append more data to it.
        size_t continue_parse(const char* data, size_t length);

        // Copy & Move
        object(const object& rhs);
        object(object&& lhs);
        object& operator = (const object& rhs);
        object& operator = (object&& lhs);

        // cast, when the type is integer, return the content as integer
        // otherwise, return 0
        operator int() const;

        // Validate
        bool is_validate() const;
        // Tell if parse all done
        bool all_get() const;

        // nil/null, when the type is REPLY_TYPE_BULK and content_length is -1, it's nil
        // or when the type is REPLY_TYPE_MULTIBULK and item count is 0, it's nil
        // otherwise, will alywas be not nil.
        bool is_nil() const;

        // When the type is REPLY_TYPE_MULTIBULK and the item count is 0, it's empty list
        bool is_empty() const;

        // Format display
        string str(const string& prefix = string("")) const;
        operator std::string() const;

        // Nan object
        static object nan;
    };

    // Query Result
    typedef std::vector< object >           result_t;
    // No result
    extern const result_t                   no_result;

    // Format Stream Output
    ostream& operator << ( ostream& os, const object& ro );

    // Output all result
    ostream& operator << ( ostream& os, const result_t& r );

}}}}}

namespace pe { namespace co { namespace net { namespace redis {

    typedef pe::co::net::proto::redis::result_t         result_t;
    using pe::co::net::proto::redis::object;
    using pe::co::net::proto::redis::command;

    typedef struct {
        size_t                  index;
        const std::string &     value;
    } result_item_rule_t;

    typedef struct {
        size_t                  index;
        bool                    can_nil;
    } result_item_nil_t;

    typedef struct {
        size_t                          min_count;
        size_t                          must_count;
        std::list< result_item_rule_t > item_rule;
        std::list< result_item_nil_t >  nil_rule;
    } result_check_rule_t;

    // Check the result of a redis query
    bool result_check( const result_t r, result_check_rule_t&& rule );

    // Redis Connection Class
    class connector {
    private: 
        peer_t                      server_info_;
        string                      password_;
        int                         db_;

        // Connected flag
        bool                        connecting_;
        bool                        connected_;
        condition                   cond_connected_;
        task_t                      keepalive_;

        // // Query Command Pool
        // std::queue< query_item_t >  query_queue_;
        // buffer_guard_64k            bufgd_;
        // other_task                  job_task_;
        // Replace old std::queue with new eventqueue
        eventqueue< command, result_t >     equeue_;

        // Cache
        int                         last_obj_count_;
        result_t                    last_result_;

        // The load average of this connector
        task_time_t                 action_time_;

        // Cannot create a connection without server info
        connector() = delete;
        // No Copy or Move
        connector(const connector&) = delete;
        connector(connector&&) = delete;
        connector& operator = ( const connector& ) = delete;
        connector& operator = ( connector&& ) = delete;

        // Main Connect Function
        // This function will try to connect to the server with or without password
        // then it will maintains an infinitive incoming poll to process all response.
        void connect_();

        // Tell if all data get for current querying
        bool is_all_get_();
        // Do job task
        bool do_query_job_( tcpadapter* adapter, command&& cmd );
    public:
        // C'str with server info and password
        connector(const peer_t& server_info, const string& pwd = "", int db = 0);
        ~connector();

        // Validate
        bool is_validate() const;
        operator bool() const;

        // Force to connect again
        bool connect(const peer_t& server_info = peer_t::nan, const string& pwd = "", int db = -1);

        // Disconnect
        void disconnect();

        // No-keepalive
        void nokeepalive();

        // Test Connection
        bool connection_test();

        // Query
        result_t query( command&& cmd );

        // Get the last result
        const result_t& last_result() const;

        // Get the load average
        const task_time_t& action_time() const;
    };

    // Connection Group
    class group {
        std::vector< connector * >          lcon_;
        peer_t                              sinfo_;
        std::string                         spwd_;
        int                                 db_;
    public: 
        group( const peer_t& server_info, const std::string& pwd = "", size_t cc = 1 );
        group( const std::string& server_connect_string, size_t cc = 1 );
        ~group();

        // Get current database index
        int dbIndex() const;

        // Switch db index
        void select_db( int db );
        // Get the lowest load average connector
        connector& lowest_load_connector();

        // Query
        result_t query( command&& cmd );

        // Subscribe
        task_t subscribe( 
            command&& cmd, 
            std::function< void (const std::string&, const std::string&) > cb
        );

        // Unsubscribe
        void unsubscribe( task_t subtask );

        // Auto create the command and query
        template < typename cmd_t >
        command& __build_command( command& rc, const cmd_t& c ) {
            return (rc << std::to_string(c));
        }
        template < typename cmd_t, typename... other_cmd_t >
        command& __build_command( command& rc, const cmd_t& c, const other_cmd_t&... ocs ) {
            return __build_command(__build_command(rc, c), ocs...);
        }
        template < typename... cmd_t >
        result_t query( const cmd_t&... c ) {
            command _c; 
            __build_command(_c, c...);
            return query( std::move(_c) );
        }
        template < typename... cmd_t >
        task_t subscribe( 
            std::function< void(const std::string&, const std::string&) > cb,
            const cmd_t&... c
        ) {
            command _c;
            __build_command(_c, c...);
            return subscribe( std::move(_c), cb );
        }
    };
}}}}

#endif

// Push Chen
