/*
    netadapter.h
    PECoNet
    2019-10-16
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_NETADAPTER_H__
#define PE_CO_NET_NETADAPTER_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>

#include <peco/conet/conns/tcp.h>
#include <peco/conet/conns/ssl.h>
#include <peco/conet/conns/udp.h>
#include <peco/conet/conns/uds.h>

#include <peco/conet/protos/socks5.h>

namespace pe { namespace co { namespace net {

    /*
        basic network adapter interface
    */
    class iadapter {
    protected: 
        std::string                     strbuf_;
        bool                            destory_flag_;
        task_t                          quit_task_;
    public: 
        typedef std::pair< socket_op_status, std::string >      data_type;

    public: 
        iadapter();
        virtual ~iadapter() = 0;

        // Task Interface
        virtual task_t get_task() = 0;
        // IO Interfaces

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;

        // Wait for data
        virtual data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;

        // Read a word
        virtual data_type read_word( duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Read a line
        virtual data_type read_line( duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Read until crlf
        virtual data_type read_crlf( duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Read until
        virtual data_type read_until( const std::string& p, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Read enough
        virtual data_type read_enough( size_t l, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Switch current adapter's data with other adapter,
        // this method will block current task
        virtual void switch_data( iadapter* other_adapter );

        // Block current task and make an infinitive read
        // send the incoming data to another adapter
        // Different between `swtich_data` is this method will not
        // try to receive any data from other adapter.
        virtual void transfer_data( iadapter * other_adapter );
    };

    /*
        The net adapter will only work under sending mode
        A listener will never be suitable for an adapter
        This is just an interface for all net adapter object.
    */
    class netadapter : public taskadapter, public iadapter {
    public: 
        netadapter( long fd );
        virtual ~netadapter() = 0;

        // Task Interface
        virtual task_t get_task();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT ) = 0;
    };

    // Server adapter
    class serveradapter : public iadapter {
    protected: 
        task_t              t_;
    public: 
        serveradapter();
        virtual ~serveradapter() = 0;
        
        // Task Interface
        virtual task_t get_task();
    };

    // TCP Connection adapter
    class tcpadapter : public netadapter {
    public: 
        // Create a tcp task and bind the adapter
        tcpadapter();
        virtual ~tcpadapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    class tcp_serveradapter : public serveradapter {
    public: 
        virtual ~tcp_serveradapter();

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    // SSL Connection adapter·
    class ssladapter : public netadapter {
        net::ssl::ssl_item_wrapper<SSL>     ssl_;
    public: 
        // Create an ssl task and bind the adapter
        ssladapter();
        virtual ~ssladapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    class ssl_serveradapter : public serveradapter {
    public: 
        virtual ~ssl_serveradapter();

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    // UDP Connection adapter
    class udpadapter : public netadapter {
        struct sockaddr_in      peer_addr;

    public: 
        // Create a udp task and bind the adapter
        udpadapter();
        virtual ~udpadapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    // UDS Connection adapter
    class udsadapter : public netadapter {
    public: 
        // Create a uds task and bind the adapter
        udsadapter();
        virtual ~udsadapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    class uds_serveradapter : public serveradapter {
    public: 
        virtual ~uds_serveradapter();

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    // Socks5 Connection adapter
    class socks5adapter : public netadapter {
        peer_t              socks5_addr;
    public: 
        // Create a socks5 task and bind the adapter
        socks5adapter(const peer_t& socks5);
        virtual ~socks5adapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    class socks5_serveradapter : public serveradapter {
    public: 
        virtual ~socks5_serveradapter();

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

    // SOcks5 SSL Connection adapter
    class socks5sadapter : public netadapter {
        peer_t                              socks5_addr;
        net::ssl::ssl_item_wrapper<SSL>     ssl_;
    public: 
        // Create a socks5 task and bind the adapter
        socks5sadapter(const peer_t& socks5);
        virtual ~socks5sadapter();

        // Parse the host to connection's host address
        virtual bool connect( const std::string& host, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Write Data
        virtual bool write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Direct write data without block
        virtual bool direct_write( std::string&& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const std::string& data, duration_t timedout = NET_DEFAULT_TIMEOUT );
        virtual bool direct_write( const char* data, size_t len, duration_t timedout = NET_DEFAULT_TIMEOUT );

        // Wait for data
        virtual iadapter::data_type read( duration_t timedout = NET_DEFAULT_TIMEOUT );
    };

}}}

#endif 

// Push Chen
