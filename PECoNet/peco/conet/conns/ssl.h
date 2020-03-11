/*
    ssl.h
    PECoNet
    2019-05-23
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_SSL_H__
#define PE_CO_NET_SSL_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <peco/conet/conns/conns.hpp>

namespace pe { namespace co { namespace net { 
    struct ssl {

        typedef SSL_CTX*        ssl_ctx_t;
        typedef SSL*            ssl_t;

        typedef peer_t          address_bind;
        typedef peer_t          connect_address;

        // Load cert function point
        typedef std::function< bool (ssl_ctx_t) >           load_cert_t;
        typedef std::function< ssl_ctx_t () >               ssl_new_t;
        typedef std::function< const SSL_METHOD *() >       ssl_method_t;

        // Make the TCP socket as an SSL tunnel, and connect it
        static bool connect_socket_to_ssl_env(SOCKET_T so, ssl_t s);

        // Create RSA Key Pair
        typedef struct {
            string          private_key;
            string          public_key;
        } rsa_key_pair;

        // RSA key load wrapper
        typedef struct tag_rsa_key {
            typedef RSA* (*load_key_t)(BIO*, RSA**, pem_password_cb*, void *);
            BIO*            buffer_io;
            RSA*            rsa;
            tag_rsa_key(const string& key, load_key_t loader);
            ~tag_rsa_key();
        } rsa_key_wrapper_t;

        template < typename _Object_t >
        struct ssl_item_wrapper {
            typedef std::function< _Object_t* () >      create_t;
            typedef std::function< void (_Object_t*) >  destory_t;
            _Object_t *             ptr_object;
            destory_t               d;
            ssl_item_wrapper( create_t _c, destory_t _d ) : d(_d) {
                ptr_object = _c();
            }
            ~ssl_item_wrapper() {
                #ifdef DEBUG
                std::cout << "ssl item wrapper destoried" << std::endl;
                #endif
                if ( ptr_object != NULL && d ) d(ptr_object);
            }
        };

        // Create a RSA key pair
        static rsa_key_pair create_rsa_key(uint32_t key_size = 1024);

        // Coder with a given RSA
        static std::string rsa_string_coder(
            const string& data, 
            int padding, 
            RSA* rsa, 
            int (*coder)(int, const unsigned char *, unsigned char*, RSA*, int) 
        );

        // Load key from string then code the data
        static std::string rsa_string_coder(
            const string& data,
            const string& key,
            int padding,
            RSA* (*load_key)(BIO*, RSA**, pem_password_cb*, void *),
            int (*coder)(int, const unsigned char *, unsigned char*, RSA*, int) 
        );

        static std::string rsa_encrypt_with_publickey(
            const string& data, 
            const string& pubkey, 
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_encrypt_with_publickey(
            const string& data,
            RSA* rsa,
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_decrypt_with_publickey(
            const string& data, 
            const string& pubkey, 
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_decrypt_with_publickey(
            const string& data,
            RSA* rsa,
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_encrypt_with_privatekey(
            const string& data, 
            const string& pubkey, 
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_encrypt_with_privatekey(
            const string& data,
            RSA* rsa,
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_decrypt_with_privatekey(
            const string& data, 
            const string& pubkey, 
            int padding = RSA_PKCS1_PADDING
        );
        static std::string rsa_decrypt_with_privatekey(
            const string& data,
            RSA* rsa,
            int padding = RSA_PKCS1_PADDING
        );

        enum {
            ctx_type_server,
            ctx_type_client
        };

        // Internal ssl context manager class
        class __ssl {
        private: 
            map< string, ssl_ctx_t >            __ctx_map;
            mutex                               __ctx_mutex;

            // Free all ctx
            void __clean();

            // Private C'str and D'str
            __ssl();
            ~__ssl();
        public: 
            // Singleton instance API
            static __ssl& mgr();

            // Create or fetch the ctx
            ssl_ctx_t ctx_with_method(const string& key, load_cert_t lcf, ssl_new_t snf);

            // Return the new ctx function according to context type
            static ssl_method_t f_for_type( long ct );
            // Get the default key for each ctx type
            static string k_for_type( long ct );
        };

        // Get the ctx
        template < long ctx_type >
        static ssl_ctx_t get_ctx(const string& key, load_cert_t lcf) {
            return __ssl::mgr().ctx_with_method(key, lcf, []() {
                return SSL_CTX_new(__ssl::f_for_type(ctx_type)());
            });
        }

        template < long ctx_type > 
        static ssl_ctx_t get_ctx() {
            return __ssl::mgr().ctx_with_method(__ssl::k_for_type(ctx_type), nullptr, []() {
                return SSL_CTX_new(__ssl::f_for_type(ctx_type)());
            });
        }

        // Load Cert File
        static bool load_certificates_pem(ssl_ctx_t ctx, const string& pem_file);

        // Dynamically create a cert for the given ctx
        static bool create_certificates(ssl_ctx_t ctx, uint32_t keysize = 2048);

        // Create a socket, and bind to the given peer info
        // All following functions must be invoked in a task
        // Usage:
        //  SOCKET_T _so = create();
        //  pe::co::loop::main.do_job(_so, [](){
        //      listen(...);
        //  });
        static SOCKET_T create( peer_t bind_addr = peer_t::nan );

        // Set the arg as ctx
        static void set_ctx( ssl_ctx_t ctx );
        static void set_ctx( task_t ptask, ssl_ctx_t ctx );
        // Set the arg as ssl
        static void set_ssl( ssl_t ssl );
        static void set_ssl( task_t ptask, ssl_t ssl );


        // Listen on the socket, the socket must has been bound
        // to the listen port, which means the parameter of the
        // create function cannot be nan
        static void listen(
            std::function< void( ) > on_accept
        );

        // Connect to peer, default 3 seconds for timedout
        static socket_op_status connect( 
            peer_t remote,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        // Connect to a host and port
        static socket_op_status connect(
            const string& hostname, 
            uint16_t port,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Read data from the socket
        static socket_op_status read_from(
            task_t ptask,
            string& buffer,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
        );
        static socket_op_status read( 
            string& buffer,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT 
        );
        static socket_op_status read(
            char* buffer,
            uint32_t& buflen,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Write the given data to peer
        static socket_op_status write(
            string&& data, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        static socket_op_status write(
            const char* data, 
            uint32_t length,
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );
        static socket_op_status write_to( 
            task_t ptask, 
            const char* data, 
            uint32_t length, 
            pe::co::duration_t timedout = NET_DEFAULT_TIMEOUT
        );

        // Redirect data between two socket use the
        // established tunnel
        // If there is no tunnel, return false, otherwise
        // after the tunnel has broken, return true
        static bool redirect_data( task_t ptask, write_to_t hwt = ssl::write_to );
    };
    // The tcp factory
    typedef conn_factory< ssl >  ssl_factory;
    typedef typename ssl_factory::item  ssl_item;
}}}

#endif

// Push Chen
//

