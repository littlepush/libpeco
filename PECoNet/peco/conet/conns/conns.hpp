/*
    conns.hpp
    PECoNet
    2019-06-13
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#pragma once

#ifndef PE_CO_NET_CONNS_HPP__
#define PE_CO_NET_CONNS_HPP__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/utils/rawf.h>

namespace pe { namespace co { namespace net { 

    typedef struct {
        char *          buf;
        uint32_t *      buflen;
    } conn_buf_t;

    template < typename _ConnType >
    struct conn_data_in {
        // Default timedout
        duration_t                  timedout;

        // init with default timedout
        conn_data_in() : timedout(NET_DEFAULT_TIMEOUT) { }

        conn_data_in& settw( duration_t nt ) { timedout = nt; }

        socket_op_status operator >> ( conn_buf_t b ) {
            return _ConnType::read( b.buf, *b.buflen, timedout );
        }

        socket_op_status operator >> ( std::string& b ) {
            return _ConnType::read( b, timedout );
        }
    };

    template < typename _ConnType >
    struct conn_data_out {
        // Default timedout
        duration_t                  timedout;

        // init with default timedout
        conn_data_out() : timedout(NET_DEFAULT_TIMEOUT) { }

        conn_data_out& settw( duration_t nt ) { timedout = nt; }

        socket_op_status operator << ( conn_buf_t b ) {
            return _ConnType::write( b.buf, *b.buflen, timedout );
        }

        socket_op_status operator << ( std::string&& b ) {
            return _ConnType::write( std::move(b), timedout );
        }
    };

    template < typename _ConnType >
    struct conn_async_item {
        typedef conn_async_item<_ConnType>              self_t;
        typedef std::function< void (self_t&&) >        conn_task_t;
        typedef typename _ConnType::connect_address     conn_address_t;

        // The internal socket object
        net::SOCKET_T       conn_obj;
        task_t              conn_task;

        // Data operator
        conn_data_in<_ConnType>     in;
        conn_data_out<_ConnType>    out;

        self_t& operator += ( conn_task_t t ) {
            if ( SOCKET_NOT_VALIDATE(conn_obj) ) {
                conn_obj = -1;
                conn_task = NULL;
            } else {
                conn_task = loop::main.do_job(conn_obj, [t]() {
                    t(self_t{this_task::get_id(), this_task::get_task()});
                });
            }
            return *this;
        }

        socket_op_status connect( 
            const conn_address_t& addr, 
            duration_t timedout = NET_DEFAULT_TIMEOUT
        ) {
            auto _op = _ConnType::connect( addr, timedout );
            if ( _op == op_timedout ) {
                std::stringstream _ss;
                _ss << "Connection to " << addr << " timedout!";
                throw _ss.str();
            }
            if ( _op == op_failed ) {
                std::stringstream _ss;
                _ss << "Connection to " << addr << " falied!";
                throw _ss.str();
            }
            return _op;
        }

        template < typename _OtherConnType >
        void sync_data( conn_async_item<_OtherConnType> other_item ) {
            _ConnType::redirect_data( 
                other_item.conn_task, 
                &_OtherConnType::write_to 
            );
        }
    };

    template < typename _ConnType >
    struct conn_listen_item {
        typedef conn_async_item<_ConnType>              item_t;
        typedef std::function< void (item_t&&) >        conn_task_t;

        // The internal socket object
        net::SOCKET_T       conn_obj;
        task_t              conn_task;

        conn_listen_item<_ConnType>& operator += ( conn_task_t t ) {
            if ( SOCKET_NOT_VALIDATE(conn_obj) ) {
                conn_obj = -1;
                conn_task = NULL;
            } else {
                conn_task = loop::main.do_job(conn_obj, [t]() {
                    _ConnType::listen([t]() {
                        t(item_t{this_task::get_id(), this_task::get_task()});
                    });
                });
            }
            return *this;
        }

        // Shutdown the listening server
        void shutdown() { task_exit(conn_task); }
    };

    template < typename _ConnType >
    struct conn_factory {
        typedef _ConnType                           conn_type;
        typedef conn_async_item<_ConnType>          item;
        typedef typename _ConnType::address_bind    address_bind;
        typedef typename _ConnType::connect_address connect_address;

        static conn_async_item<_ConnType> create_new_client( ) {
            SOCKET_T _so = _ConnType::create( );
            return conn_async_item<_ConnType>{ _so, NULL };
        }

        static conn_listen_item<_ConnType> startup_server( address_bind addr ) {
            SOCKET_T _so = _ConnType::create( addr );
            return conn_listen_item<_ConnType>{ _so, NULL };
        }

        static struct __client {
            item operator += ( 
                typename item::conn_task_t t
            ) {
                return conn_factory<_ConnType>::create_new_client() += t;
            }
        } client;

        static struct __server {
            conn_listen_item<_ConnType> operator & ( address_bind addr ) {
                return conn_factory<_ConnType>::startup_server(addr);
            }
        } server;
    };

    template < typename _ConnType >
    typename conn_factory<_ConnType>::__client conn_factory<_ConnType>::client;

    template < typename _ConnType >
    typename conn_factory<_ConnType>::__server conn_factory<_ConnType>::server;
}}}

#endif

// Push Chen
//
