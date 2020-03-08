/*
    ssl.cpp
    PECoNet
    2019-05-23
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#include <peco/conet/conns/ssl.h>
#include <peco/conet/conns/tcp.h>
#include <peco/conet/protos/dns.h>
#include <peco/peutils.h>

namespace pe { namespace co { namespace net {
    // Create a RSA key pair
    ssl::rsa_key_pair ssl::create_rsa_key(uint32_t key_size) {
        ssl::ssl_item_wrapper<RSA>       _rsa(RSA_new, RSA_free);
        ssl::ssl_item_wrapper<BIGNUM>    _bne(BN_new, BN_free);

        // Create RSA key
        BN_set_word(_bne.ptr_object, RSA_F4);
        RSA_generate_key_ex(_rsa.ptr_object, key_size, _bne.ptr_object, NULL);

        // Write to BIO
        ssl::ssl_item_wrapper<BIO>       _bio_pri(std::bind(BIO_new, BIO_s_mem()), BIO_free_all);
        ssl::ssl_item_wrapper<BIO>       _bio_pub(std::bind(BIO_new, BIO_s_mem()), BIO_free_all);
        //BIO *_private = BIO_new(BIO_s_mem());
        //BIO *_public = BIO_new(BIO_s_mem());
        PEM_write_bio_RSAPrivateKey(_bio_pri.ptr_object, _rsa.ptr_object, NULL, NULL, 0, NULL, NULL);
        PEM_write_bio_RSA_PUBKEY(_bio_pub.ptr_object, _rsa.ptr_object);

        // Calculate length
        size_t _private_length = BIO_pending(_bio_pri.ptr_object);
        size_t _public_length = BIO_pending(_bio_pub.ptr_object);

        // Create result cache
        string _privateKey, _publicKey;
        _privateKey.resize(_private_length);
        _publicKey.resize(_public_length);

        // Read from BIO to Cache
        BIO_read(_bio_pri.ptr_object, &(_privateKey[0]), _private_length);
        BIO_read(_bio_pub.ptr_object, &(_publicKey[0]), _public_length);

        return (rsa_key_pair){_privateKey, _publicKey};
    }
    // Coder with a given RSA
    std::string ssl::rsa_string_coder(
        const string& data, 
        int padding, 
        RSA* rsa, 
        int (*coder)(int, const unsigned char *, unsigned char*, RSA*, int) 
    ) {
        string _result((size_t)RSA_size(rsa), '\0');
        int _rsize = (*coder)(
            data.size(), (const unsigned char *)data.c_str(), 
            (unsigned char *)&(_result[0]), rsa, padding);
        if ( _rsize == -1 ) {
            char _err[256];
            ERR_load_crypto_strings();
            ERR_error_string(ERR_get_error(), _err);
            ON_DEBUG_CONET(
            std::cout << "ssl coder failed: " << _err << std::endl;
            )
            _result.clear();
        } else {
            _result.resize((size_t)_rsize);
        }
        return _result;
    }

    ssl::tag_rsa_key::tag_rsa_key(const string& key, tag_rsa_key::load_key_t loader) 
        : buffer_io(NULL), rsa(NULL) 
    {
        buffer_io = BIO_new_mem_buf((const void *)key.c_str(), key.size());
        rsa = (*loader)(buffer_io, 0, 0, 0);
    }
    ssl::tag_rsa_key::~tag_rsa_key() {
        BIO_free_all(buffer_io);
        RSA_free(rsa);
    }

    // Load key from string then code the data
    std::string ssl::rsa_string_coder(
        const string& data,
        const string& key,
        int padding,
        RSA* (*load_key)(BIO*, RSA**, pem_password_cb*, void *),
        int (*coder)(int, const unsigned char *, unsigned char*, RSA*, int) 
    ) {
        rsa_key_wrapper_t _kw(key, load_key);
        return rsa_string_coder(data, padding, _kw.rsa, coder);
    }
    std::string ssl::rsa_encrypt_with_publickey(const string& data, const string& pubkey, int padding) {
        return rsa_string_coder(data, pubkey, padding, 
            &PEM_read_bio_RSA_PUBKEY, &RSA_public_encrypt);
    }
    std::string ssl::rsa_encrypt_with_publickey(const string& data, RSA* rsa, int padding) {
        return rsa_string_coder(data, padding, rsa, &RSA_public_encrypt);
    }
    std::string ssl::rsa_decrypt_with_publickey(const string& data, const string& pubkey, int padding) {
        return rsa_string_coder(data, pubkey, padding, 
            &PEM_read_bio_RSA_PUBKEY, &RSA_public_decrypt);
    }
    std::string ssl::rsa_decrypt_with_publickey(const string& data, RSA* rsa, int padding) {
        return rsa_string_coder(data, padding, rsa, &RSA_public_decrypt);
    }
    std::string ssl::rsa_encrypt_with_privatekey(const string& data, const string& pubkey, int padding) {
        return rsa_string_coder(data, pubkey, padding, 
            &PEM_read_bio_RSAPrivateKey, &RSA_private_encrypt);
    }
    std::string ssl::rsa_encrypt_with_privatekey(const string& data, RSA* rsa, int padding) {
        return rsa_string_coder(data, padding, rsa, &RSA_private_encrypt);
    }
    std::string ssl::rsa_decrypt_with_privatekey(const string& data, const string& pubkey, int padding) {
        return rsa_string_coder(data, pubkey, padding, 
            &PEM_read_bio_RSAPrivateKey, &RSA_private_decrypt);
    }
    std::string ssl::rsa_decrypt_with_privatekey(const string& data, RSA* rsa, int padding) {
        return rsa_string_coder(data, padding, rsa, &RSA_private_decrypt);
    }

    // Get the last error string
    string ssl_last_error() {
        static char _em[256];
        unsigned long _le = ERR_get_error();
        ERR_error_string_n(_le, &_em[0], 256);
        return std::string(_em);
    }
    
    // Process a non-blocking socket's ssl event.
    int ssl_nonblocking_process( 
        std::function< int (SSL *) > p, SSL* ssl, 
        duration_t timedout = NET_DEFAULT_TIMEOUT 
    ) {
        int _ret = 0;
        net::SOCKET_T _fd = SSL_get_fd(ssl);
        while ( true ) {
            _ret = p(ssl);
            // if ( _ret == 0 ) {
            //     ON_DEBUG_CONET(
            //     std::cerr << "SSL Error: " << ssl_last_error() << std::endl;
            //     )
            //     return -1;
            // }
            if ( _ret > 0 ) {
                return _ret;
            } else {
                // if ( !rawf::is_still_alive(_fd) ) {
                //     ON_DEBUG_CONET(
                //         std::cerr << "ssl fd: " << _fd << " on nonblocking check "
                //             << "is no longer alive." << std::endl;
                //     )
                //     return -1;
                // }
                int _err = SSL_get_error(ssl, _ret);
                if ( _err == SSL_ERROR_NONE ) {
                    // EINTR, continue
                    ON_DEBUG_CONET(
                        std::cout << "ssl fd: " << _fd << " on nonblocking "
                            << "with SSL_ERROR_NONE, continue" << std::endl;
                    )
                    continue;
                } else if ( _err == SSL_ERROR_ZERO_RETURN ) {
                    // Broken
                    ON_DEBUG_CONET(
                        std::cout << "ssl fd: " << _fd << " on nonblocking "
                            << "with SSL_ERROR_ZERO_RETURN, broken" << std::endl;
                    )
                    return -1;
                } else if ( _err == SSL_ERROR_WANT_READ ) {
                    // if ( ! this_task::yield_if( SSL_pending(ssl) > 0 ) ) {
                    auto _signal = this_task::wait_fd_for_event(
                        _fd, event_type::event_read, timedout);
                    ON_DEBUG_CONET(
                        std::cerr << "ssl fd: " << _fd << " after waiting for readevent, signal is " 
                        << _signal << std::endl;)
                    if ( _signal == waiting_signals::no_signal ) 
                        return 0;
                    if ( _signal == waiting_signals::bad_signal ) 
                        return -1;
                    // }
                    continue;
                } else if ( _err == SSL_ERROR_WANT_WRITE ) {
                    auto _signal = this_task::wait_fd_for_event(
                        _fd, event_type::event_write, timedout);
                    ON_DEBUG_CONET(
                        std::cerr << "ssl fd: " << _fd << " after waiting for writeevent, signal is " 
                        << _signal << std::endl;)
                    if ( _signal == waiting_signals::no_signal ) 
                        return 0;
                    if ( _signal == waiting_signals::bad_signal ) 
                        return -1;
                    continue;
                } else if ( _err == SSL_ERROR_WANT_ACCEPT || _err == SSL_ERROR_WANT_CONNECT ) {
                    auto _signal = this_task::wait_fd_for_event(
                        _fd, event_type::event_write, timedout);
                    if ( _signal == waiting_signals::no_signal ) 
                        return 0;
                    if ( _signal == waiting_signals::bad_signal ) 
                        return -1;
                    continue;
                } else if ( _err == SSL_ERROR_SYSCALL ) {
                    ON_DEBUG_CONET(
                        std::cerr << "ssl fd: " << _fd << " get a syscall error, errno is: "
                            << errno << ", err: " << ::strerror(errno) << std::endl;
                    )
                    return -1;
                } else {
                    ON_DEBUG_CONET(
                    std::cerr << "ssl fd: " << _fd << " on nonblocking ret(" << 
                        _ret << "), get error(" << _err << "), SSL Error: " << 
                        ssl_last_error() << std::endl;
                    )
                    return -1;
                }
            }
        }
        return _ret;
    }

    // Make the TCP socket as an SSL tunnel, and connect it
    bool ssl::connect_socket_to_ssl_env(SOCKET_T so, ssl_t s) {
        // We do connected to the peer, now we have an available tcp socket.
        // then we should transfer it to the ssl socket
        if ( 0 == SSL_set_fd(s, so) ) {
            ON_DEBUG_CONET(
            std::cerr << "Error: failed to set fd: "
                << ssl_last_error() << std::endl;
            )
            return false;
        }
        return ssl_nonblocking_process(&SSL_connect, s) > 0;
    }

    void ssl::__ssl::__clean() {
        lock_guard< mutex > _(__ctx_mutex);
        for ( auto& cc : __ctx_map ) {
            SSL_CTX_free(cc.second);
        }
        __ctx_map.clear();
    }
    ssl::__ssl::__ssl() {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
    }

    ssl::__ssl::~__ssl() { __clean(); }

    ssl::__ssl& ssl::__ssl::mgr() {
        static __ssl _si;
        return _si;
    }

    ssl::ssl_ctx_t ssl::__ssl::ctx_with_method(const string& key, load_cert_t lcf, ssl_new_t snf) {
        lock_guard< mutex > _(__ctx_mutex);
        auto _c = __ctx_map.find(key);
        if ( _c == __ctx_map.end() ) {
            // Create a new context
            // New CTX
            ssl::ssl_ctx_t _ctx = snf();
            if ( _ctx == NULL ) return NULL;
            // Load the pem_file
            if ( lcf ) {
                if ( !lcf(_ctx) ) return NULL;
            }
            __ctx_map[key] = _ctx;
            return _ctx;
        }
        return _c->second;                
    }

    ssl::ssl_method_t ssl::__ssl::f_for_type( long ct ) {
        static map< long, ssl_method_t > g_cm_f = {
        #if SSL_VERSION_1_0
            {ctx_type_server, &TLSv1_2_server_method},
            {ctx_type_client, &TLSv1_2_client_method}
        #else // SSL_VERSION_1_1
            {ctx_type_server, &TLS_server_method},
            {ctx_type_client, &TLS_client_method}
        #endif
        };
        return g_cm_f[ct];
    }

    string ssl::__ssl::k_for_type( long ct ) {
        static map< long, string > g_k_m = {
            {ctx_type_server, "default-server-tls"},
            {ctx_type_client, "default-client-tls"}
        };
        return g_k_m[ct];
    }

    // Load Cert File
    bool ssl::load_certificates_pem(ssl::ssl_ctx_t ctx, const string& pem_file) {
        if ( SSL_CTX_use_certificate_file(ctx, pem_file.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
            ON_DEBUG_CONET(
            std::cerr << "Failed to load certificate file: " << pem_file << ", "
                << ssl_last_error() << std::endl;
            )
            return false;
        }
        if ( SSL_CTX_use_PrivateKey_file(ctx, pem_file.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
            ON_DEBUG_CONET(
            std::cerr << "Failed to laod privitekey: " << pem_file << ", "
                << ssl_last_error() << std::endl;
            )
            return false;
        }
        if ( !SSL_CTX_check_private_key(ctx) ) {
            ON_DEBUG_CONET(
            std::cerr << "Private does not match the certificate file: "
                << pem_file << std::endl;
            )
            return false;
        }
        return true;
    }
    // Dynamically create a cert for the given ctx
    bool ssl::create_certificates(ssl::ssl_ctx_t ctx, uint32_t keysize) {
        ssl_item_wrapper<EVP_PKEY>  _pkey(EVP_PKEY_new, nullptr);
        ssl_item_wrapper<RSA>       _rsa(RSA_new, RSA_free);
        ssl_item_wrapper<BIGNUM>    _bne(BN_new, BN_free);
        ssl_item_wrapper<X509>      _x509(X509_new, X509_free);

        // Create RSA key
        BN_set_word(_bne.ptr_object, RSA_F4);
        RSA_generate_key_ex(_rsa.ptr_object, keysize, _bne.ptr_object, NULL);


        EVP_PKEY_assign_RSA(_pkey.ptr_object, _rsa.ptr_object);
        // Change serial number to 1
        ASN1_INTEGER_set(X509_get_serialNumber(_x509.ptr_object), 1);
        // Set the cert's time
        X509_gmtime_adj(X509_get_notBefore(_x509.ptr_object), 0);
        X509_gmtime_adj(X509_get_notAfter(_x509.ptr_object), 315360000L);   // 10 years
        X509_set_pubkey(_x509.ptr_object, _pkey.ptr_object);

        // Set name
        X509_NAME * _name = X509_get_subject_name(_x509.ptr_object);
        X509_NAME_add_entry_by_txt(_name, "C",  MBSTRING_ASC, (unsigned char *)"CN", -1, -1, 0);
        X509_NAME_add_entry_by_txt(_name, "O",  MBSTRING_ASC, (unsigned char *)"MeetU Inc.", -1, -1, 0);
        X509_NAME_add_entry_by_txt(_name, "CN", MBSTRING_ASC, (unsigned char *)"LIBSSL", -1, -1, 0);
        X509_set_issuer_name(_x509.ptr_object, _name);

        // Sign
        X509_sign(_x509.ptr_object, _pkey.ptr_object, EVP_sha1());

        BIO *_tfbio = BIO_new(BIO_s_file());
        if ( _tfbio == NULL ) {
            ON_DEBUG_CONET(
                std::cout << "failed to create file BIO" << std::endl;
            )
            return false;
        }
        std::string __random__ = std::to_string((uint32_t)time(NULL));
        std::string _tfname = "." + utils::md5(__random__);
        char *_ptfname = &_tfname[0];
        if ( !BIO_write_filename(_tfbio, _ptfname) ) {
            ON_DEBUG_CONET(
                std::cout << "failed to create pem file: " << _tfname << std::endl;
            )
            return false;
        }
        PEM_write_bio_X509(_tfbio, _x509.ptr_object);
        PEM_write_bio_PrivateKey(_tfbio, _pkey.ptr_object, NULL, NULL, 0, NULL, NULL);
        BIO_free(_tfbio);

        // Set
        bool _r = load_certificates_pem(ctx, _tfname);
        utils::fs_remove(_tfname);

        return _r;
    }

    // Create a socket, and bind to the given peer info
    SOCKET_T ssl::create( peer_t bind_addr ) { return tcp::create( bind_addr ); }

    // Listen on the socket, the socket must has been bound
    // to the listen port, which means the parameter of the
    // create function cannot be nan
    void ssl::listen(
        std::function< void( ) > on_accept
    ) {
        if ( -1 == ::listen((SOCKET_T)this_task::get_id(), CO_MAX_SO_EVENTS) ) {
            ON_DEBUG_CONET(
            std::cerr << "failed to listen on socket(" <<
                this_task::get_id() << "), error: " << ::strerror(errno) 
                << std::endl;
            )
            return;
        }
        while ( true ) {
            auto _signal = pe::co::this_task::wait_for_event(
                pe::co::event_type::event_read,
                std::chrono::hours(1)
            );
            if ( _signal == pe::co::waiting_signals::bad_signal ) {
                ON_DEBUG_CONET(
                std::cerr << "failed to listen! " << ::strerror(errno) << std::endl;
                )
                return;
            }
            if ( _signal == pe::co::waiting_signals::no_signal ) continue;

            ssl::ssl_ctx_t _ctx = (ssl::ssl_ctx_t)this_task::get_task()->arg;
            // Get the incoming socket
            while ( true ) {
                struct sockaddr _inaddr;
                socklen_t _inlen = 0;
                memset(&_inaddr, 0, sizeof(_inaddr));
                SOCKET_T _inso = accept( 
                    (SOCKET_T)pe::co::this_task::get_id(), &_inaddr, &_inlen );
                if ( _inso == -1 ) {
                    // No more incoming
                    if ( errno == EAGAIN || errno == EWOULDBLOCK ) break;
                    // Or error
                    return;
                } else {
                    // Set non-blocking
                    rawf::nonblocking(_inso, true);
                    rawf::nodelay(_inso, true);
                    rawf::reusable(_inso, true);
                    rawf::keepalive(_inso, true);
                    // Set a 32K buffer
                    rawf::buffersize(_inso, 1024 * 32, 1024 * 32);

                    this_loop.do_job(_inso, [_ctx, on_accept]() {
                        ssl_item_wrapper<SSL> _sslw( std::bind(SSL_new, _ctx), SSL_free );
                        set_ssl(_sslw.ptr_object);

                        if ( SSL_set_fd(_sslw.ptr_object, (SOCKET_T)this_task::get_id()) == 0 ) {
                            // Failed
                            ON_DEBUG_CONET(
                            std::cerr << "Failed to set fd for ssl: " << ssl_last_error() << std::endl;
                            )
                            return;
                        }
                        if ( 0 >= ssl_nonblocking_process(&SSL_accept, _sslw.ptr_object) ) {
                            ON_DEBUG_CONET(
                            std::cerr << "Failed to accept the incoming SSL connection(" 
                                << this_task::get_id() << ")." << std::endl;
                            )
                            return;
                        }
                        on_accept();
                    });
                }
            }
        }
    }

    // Connect to peer, default 3 seconds for timedout
    socket_op_status ssl::connect( 
        peer_t remote,
        pe::co::duration_t timedout
    ) {
        SOCKET_T _so = (SOCKET_T)pe::co::this_task::get_id();
        ssl::ssl_t _ssl = (ssl_t)pe::co::this_task::get_task()->arg;
        auto _op = tcp::connect( remote, timedout );
        if ( op_done != _op ) return _op;
        if ( ! connect_socket_to_ssl_env(_so, _ssl) ) return op_failed;
        return op_done;
    }

    // Connect to a host and port
    socket_op_status ssl::connect(
        const string& hostname, 
        uint16_t port,
        pe::co::duration_t timedout
    ) {
        ip_t _ip(hostname);
        if ( !_ip.is_valid() ) {
            _ip = pe::co::net::get_hostname(hostname);
        }
        peer_t _p(_ip, port);
        return connect(_p, timedout);
        return op_failed;
    }

    // Read data from the socket
    socket_op_status ssl::read_from(
        task* ptask,
        string& buffer,
        pe::co::duration_t timedout 
    ) {
        uint32_t _buf_size = 1024;     // 1K buffer size
        size_t _origin_size = buffer.size();
        buffer.resize( _origin_size + _buf_size );

        ssl::ssl_t _ssl = (ssl_t)ptask->arg;

        int _ret = ssl_nonblocking_process( [&buffer](ssl_t ssl) {
            return SSL_read(ssl, (&buffer[0]), 1024);
        }, _ssl, timedout );
        if ( _ret > 0 ) {
            buffer.resize(_origin_size + _ret);
        } else {
            buffer.resize(_origin_size);
        }
        if ( _ret < 0 ) {
            this_task::yield(); // force to yield incase this task direct return from read
            return op_failed;
        }
        if ( _ret == 0 ) return op_timedout;
        // Get some data
        ON_DEBUG_CONET(std::cout << "<= " << 
            ptask->id << ": " << 
            _ret << std::endl;)
        return op_done;
    }
    socket_op_status ssl::read( 
        string& buffer,
        pe::co::duration_t timedout
    ) {
        uint32_t _buf_size = 1024;
        size_t _origin_size = buffer.size();
        buffer.resize( _origin_size + _buf_size );
        auto _op = ssl::read( 
            (&buffer[0]) + _origin_size, _buf_size, timedout
        );
        if ( _op == op_done ) {
            buffer.resize( _origin_size + _buf_size );
        } else {
            // Recover the buffer
            buffer.resize( _origin_size );
        }
        return _op;
    }
    socket_op_status ssl::read(
        char* buffer,
        uint32_t& buflen,
        pe::co::duration_t timedout
    ) {
        if ( buflen == 0 ) return op_failed;
        ssl::ssl_t _ssl = (ssl_t)pe::co::this_task::get_task()->arg;

        int _ret = ssl_nonblocking_process( [buffer, buflen](ssl_t ssl) {
            return SSL_read(ssl, buffer, buflen);
        }, _ssl, timedout );
        if ( _ret < 0 ) {
            this_task::yield();
            return op_failed;
        }
        if ( _ret == 0 ) return op_timedout;
        // Get some data
        buflen = _ret;
        ON_DEBUG_CONET(std::cout << "<= " << 
            this_task::get_id() << ": " << 
            _ret << std::endl;)
        return op_done;
    }

    // Write the given data to peer
    socket_op_status ssl::write(
        string&& data, 
        pe::co::duration_t timedout
    ) {
        return write(data.c_str(), data.size(), timedout);
    }
    socket_op_status ssl::write(
        const char* data, 
        uint32_t length,
        pe::co::duration_t timedout
    ) {
        return write_to(this_task::get_task(), data, length, timedout);
    }
    // Set the arg as ctx
    void ssl::set_ctx( ssl_ctx_t ctx ) {
        task *_ptask = this_task::get_task();
        ssl::set_ctx( _ptask, ctx );
    }
    void ssl::set_ctx( task * ptask, ssl_ctx_t ctx ) {
        if ( ptask == NULL ) return;
        ptask->arg = (void *)ctx;
    }
    // Set the ssl
    void ssl::set_ssl( ssl_t ssl ) {
        task *_ptask = this_task::get_task();
        ssl::set_ssl(_ptask, ssl);
    }
    void ssl::set_ssl( task * ptask, ssl_t ssl ) {
        if ( ptask == NULL ) return;
        ptask->arg = (void *)ssl;
    }

    socket_op_status ssl::write_to( 
        task* ptask, 
        const char* data, 
        uint32_t length, 
        pe::co::duration_t timedout 
    ) {
        if ( length == 0 ) return op_done;

        ssl::ssl_t _ssl = (ssl_t)ptask->arg;

        uint32_t _sent = 0;
        do {
            uint32_t _next = ((_sent + 1024) < length) ? 1024 : (length - _sent);
            const char *_s_data = data + _sent;
            int _ret = ssl_nonblocking_process( [_s_data, _next](ssl_t ssl) {
                return SSL_write(ssl, _s_data, _next);
            }, _ssl, timedout );
            if ( _ret < 0 ) return op_failed;
            if ( _ret == 0 ) return op_timedout;
            _sent += _next;
        } while ( this_task::yield_if(_sent < length) );
        // Write all data
        ON_DEBUG_CONET(std::cout << "=> " << 
            ptask->id << ": " << 
            _sent << std::endl;)
        return op_done;
    }


    // Redirect data between two socket use the
    // established tunnel
    // If there is no tunnel, return false, otherwise
    // after the tunnel has broken, return true
    bool ssl::redirect_data( task * ptask, write_to_t hwt ) {
        // Both mark to 1
        ptask->reserved_flags.flag0 = 1;
        this_task::get_task()->reserved_flags.flag0 = 1;

        buffer_guard_16k _sbuf;
        uint32_t _blen = buffer_guard_16k::blen;
        socket_op_status _op = op_failed;
        while ( (_op = ssl::read(_sbuf.buf, _blen)) != op_failed ) {
            // Peer has been closed
            if ( this_task::get_task()->reserved_flags.flag0 == 0 ) break;
            
            if ( _op == op_timedout ) continue;
            if ( op_done != hwt(
                ptask, _sbuf.buf, _blen, 
                NET_DEFAULT_TIMEOUT) ) break;
            _blen = buffer_guard_16k::blen;
        }
        if ( this_task::get_task()->reserved_flags.flag0 == 1 ) {
            ptask->reserved_flags.flag0 = 0;
        }

        return true;
    }

}}}

 // Push Chen
 //
