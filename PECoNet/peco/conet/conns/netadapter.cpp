/*
    netadapter.h
    PECoNet
    2019-10-16
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#include <peco/conet/conns/netadapter.h>
#include <peco/conet/protos/dns.h>

namespace pe { namespace co { namespace net {
    /*
        basic network adapter interface
    */
    iadapter::iadapter() : destory_flag_(true), quit_task_(NULL) { }
    iadapter::~iadapter() { 
        ON_DEBUG_CONET(
            std::cout << "destory iadapter" << std::endl;
        )
    }

    // Read a word
    iadapter::data_type iadapter::read_word( duration_t timedout ) {
        size_t _spos = 0;
        do {
            while ( _spos < strbuf_.size() && !std::isspace(strbuf_[_spos]) ) ++_spos;
            if ( _spos == strbuf_.size() ) {
                auto _new_incoming = this->read( timedout );
                if ( _new_incoming.first != op_done ) return _new_incoming;
                strbuf_.append(_new_incoming.second);
            } else {
                std::string _result = strbuf_.substr(0, _spos);
                strbuf_.erase(0, _spos + 1);
                return std::make_pair< socket_op_status, std::string >(
                    op_done, std::move(_result));
            }
        } while ( true );
    }

    // Read a line
    iadapter::data_type iadapter::read_line( duration_t timedout ) {
        return this->read_until( "\n", timedout );
    }
    // Read until crlf
    iadapter::data_type iadapter::read_crlf( duration_t timedout ) {
        return this->read_until( "\r\n", timedout );
    }
    // Read until
    iadapter::data_type iadapter::read_until( const std::string& p, duration_t timedout ) {
        do {
            size_t _pos = strbuf_.find(p);
            if ( _pos == std::string::npos ) {
                auto _new_incoming = this->read( timedout );
                if ( _new_incoming.first != op_done ) return _new_incoming;
                strbuf_.append(_new_incoming.second);
            } else {
                std::string _result = strbuf_.substr(0, _pos);
                strbuf_.erase(0, _pos + p.size());
                return std::make_pair< socket_op_status, std::string >(
                    op_done, std::move(_result));
            }
        } while ( true );
    }
    // Read enough
    iadapter::data_type iadapter::read_enough( size_t l, duration_t timedout ) {
        std::string _result;

        ON_DEBUG_CONET(
            std::cout << "=>> read enough: l(" << l << "), cached: " << strbuf_.size() << std::endl;
        )

        if ( _result.size() == 0 ) {
            _result = std::move(strbuf_);
            strbuf_.clear();
        }

        while ( _result.size() < l ) {
            auto _new_incoming = this->read( timedout );
            if ( _new_incoming.first != op_done ) return _new_incoming;
            _result.append(_new_incoming.second);
        }

        // If too many
        if ( _result.size() > l ) {
            strbuf_ = _result.substr(l);
            _result.erase(l);
        }

        ON_DEBUG_CONET(
            std::cout << "=>> read enough: l(" << l << "), result: " << _result.size() << std::endl;
        )

        return std::make_pair< socket_op_status, std::string >(
            op_done, std::move(_result));
    }
    // Switch current adapter's data with other adapter,
    // this method will block current task
    void iadapter::switch_data( iadapter* other_adapter ) {
        loop::main.do_job([this, other_adapter]() {
            other_adapter->transfer_data(this);
        });
        this->transfer_data(other_adapter);
    }
    // Block current task and make an infinitive read
    // send the incoming data to another adapter
    // Different between `swtich_data` is this method will not
    // try to receive any data from other adapter.
    void iadapter::transfer_data( iadapter * other_adapter ) {
        // Can not be destoried
        destory_flag_ = false;

        task_t _i_task = this->get_task();
        task_t _o_task = other_adapter->get_task();
        task_set_user_flag0(_i_task, 1);
        task_set_user_flag0(_o_task, 1);

        ON_DEBUG(
            long _i_id = (long)task_get_id(_i_task);
            long _o_id = (long)task_get_id(_o_task);
        )

        rawf::buffersize(task_get_id(_i_task), 262144, 262144); // 256K buffer

        if ( strbuf_.size() > 0 ) {
            if ( !other_adapter->direct_write(std::move(strbuf_)) ) {
                if ( task_get_user_flag0(_i_task) == 1 ) {
                    task_set_user_flag0(_o_task, 0);
                    task_stop(_o_task);
                }
                destory_flag_ = true;
                if ( quit_task_ != NULL ) task_go_on(quit_task_);
                return;
            }
            strbuf_.clear();
        }
        do {
            auto _r = this->read();
            ON_DEBUG_CONET(
                std::cout << "on transfer from " << _i_id << " -> " 
                    << _o_id << ", read flag: " << _r.first 
                    << ", data size: " << _r.second.size() << ", my flag is: "
                    << (int)task_get_user_flag0(_i_task) << std::endl;
            )
            if ( task_get_user_flag0(_i_task) == 0 ) break;
            if ( _r.first == op_timedout ) continue;
            if ( _r.first == op_failed ) break;
            // Write failed, means the other adapter has been disconnected
            if ( !other_adapter->direct_write(std::move(_r.second)) ) {
                ON_DEBUG_CONET(
                    std::cout << "on transfer from " << _i_id << " -> "
                    << _o_id << ", direct_write failed, broken" << std::endl;
                )
                break;
            }
        } while ( !destory_flag_ );

        // Check if this task is broken first.
        if ( task_get_user_flag0(_i_task) == 1 ) {
            task_set_user_flag0(_o_task, 0);
            task_stop(_o_task);
        }
        destory_flag_ = true;
        if ( quit_task_ != NULL ) task_go_on(quit_task_);
    }

    /*
        The net adapter will only work under sending mode
        A listener will never be suitable for an adapter
        This is just an interface for all net adapter object.
    */
    netadapter::netadapter( long fd ) : taskadapter( fd ) { }
    netadapter::~netadapter() { 
        ON_DEBUG_CONET(
            std::cout << "destory netadapter" << std::endl;
        )
    }
    task_t netadapter::get_task() { return t; }

    // Server adapter
    serveradapter::serveradapter() : t_(this_task::get_task()) { }
    serveradapter::~serveradapter() {
        ON_DEBUG_CONET(
            std::cout << "destory serveradapter" << std::endl;
        )
    }
    task_t serveradapter::get_task() { return t_; }

    // Create a tcp socket and bind to the adapter
    tcpadapter::tcpadapter() : netadapter( tcp::create() ) { }
    tcpadapter::~tcpadapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Parse the host to connection's host address
    bool tcpadapter::connect( const std::string& host, duration_t timedout ) {
        size_t _pp = host.find(":");
        if ( _pp == std::string::npos ) return false;
        std::string _addr = host.substr(0, _pp);
        uint16_t _port = (uint16_t)std::stoi(host.substr(_pp + 1));
        task_t _t = this_task::get_task();
        this->step([_addr, _port, _t, timedout]() {
            task_helper _ot(_t);
            if ( op_done != tcp::connect( _addr, _port, timedout ) ) return;
            _ot.job_done();
        });
        return this_task::holding();
    }

    // Write Data
    bool tcpadapter::write( std::string&& data, duration_t timedout ) {
        return (op_done == tcp::write_to(t, data.c_str(), data.size(), timedout));
    }
    bool tcpadapter::write( const std::string& data, duration_t timedout ) {
        return (op_done == tcp::write_to(t, data.c_str(), data.size(), timedout));
    }
    bool tcpadapter::write( const char* data, size_t len, duration_t timedout) {
        return (op_done == tcp::write_to(t, data, len, timedout) );
    }
    // Direct write data without block
    bool tcpadapter::direct_write( std::string&& data, duration_t timedout ) {
        return (op_done == tcp::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool tcpadapter::direct_write( const std::string& data, duration_t timedout ) {
        return (op_done == tcp::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool tcpadapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return (op_done == tcp::write_to(t, data, len, timedout) );
    }

    // Wait for data
    iadapter::data_type tcpadapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = tcp::read_from( t, _d.second, timedout );
        return _d;
    }

    tcp_serveradapter::~tcp_serveradapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }        
    }
    // Write Data
    bool tcp_serveradapter::write( std::string&& data, duration_t timedout ) {
        return op_done == tcp::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool tcp_serveradapter::write( const std::string& data, duration_t timedout ) {
        return op_done == tcp::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool tcp_serveradapter::write( const char* data, size_t len, duration_t timedout) {
        return (op_done == tcp::write_to(t_, data, len, timedout) );
    }
    // Direct write data without block
    bool tcp_serveradapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == tcp::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool tcp_serveradapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == tcp::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool tcp_serveradapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return (op_done == tcp::write_to(t_, data, len, timedout) );
    }

    // Wait for data
    iadapter::data_type tcp_serveradapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = tcp::read_from(t_, _d.second, timedout);
        return _d;
    }

    // Create an ssl socket and bind to the adapter
    ssladapter::ssladapter() : 
        netadapter( ssl::create() ),
        ssl_(std::bind(SSL_new, ssl::get_ctx< ssl::ctx_type_client >()), SSL_free) { 
        // Bind the SSL context
        ssl::set_ssl( this->t, ssl_.ptr_object );
    }
    ssladapter::~ssladapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }        
    }

    // Parse the host to connection's host address
    bool ssladapter::connect( const std::string& host, duration_t timedout ) {
        size_t _pp = host.find(":");
        if ( _pp == std::string::npos ) return false;
        std::string _addr = host.substr(0, _pp);
        uint16_t _port = (uint16_t)std::stoi(host.substr(_pp + 1));
        task_t _t = this_task::get_task();
        this->step([_addr, _port, _t, timedout]() {
            task_helper _ot(_t);
            if ( op_done != ssl::connect( _addr, _port, timedout ) ) return;
            _ot.job_done();
        });
        return this_task::holding();
    }

    // Write Data
    bool ssladapter::write( std::string&& data, duration_t timedout ) {
        return (op_done == ssl::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool ssladapter::write( const std::string& data, duration_t timedout ) {
        return (op_done == ssl::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool ssladapter::write( const char* data, size_t len, duration_t timedout) {
        return (op_done == ssl::write_to(t, data, len, timedout) );
    }
    // Direct write data without block
    bool ssladapter::direct_write( std::string&& data, duration_t timedout ) {
        return (op_done == ssl::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool ssladapter::direct_write( const std::string& data, duration_t timedout ) {
        return (op_done == ssl::write_to(t, data.c_str(), data.size(), timedout) );
    }
    bool ssladapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return (op_done == ssl::write_to(t, data, len, timedout) );
    }

    // Wait for data
    iadapter::data_type ssladapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = ssl::read_from(t, _d.second, timedout);
        return _d;
    }

    ssl_serveradapter::~ssl_serveradapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Write Data
    bool ssl_serveradapter::write( std::string&& data, duration_t timedout ) {
        return op_done == ssl::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool ssl_serveradapter::write( const std::string& data, duration_t timedout ) {
        return op_done == ssl::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool ssl_serveradapter::write( const char* data, size_t len, duration_t timedout) {
        return (op_done == ssl::write_to(t_, data, len, timedout) );
    }
    bool ssl_serveradapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == ssl::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool ssl_serveradapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == ssl::write_to(t_, data.c_str(), data.size(), timedout);
    }
    bool ssl_serveradapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return (op_done == ssl::write_to(t_, data, len, timedout) );
    }

    // Wait for data
    iadapter::data_type ssl_serveradapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = ssl::read_from(t_, _d.second, timedout);
        return _d;
    }

    // Create a udp socket and bind to the adapter
    udpadapter::udpadapter() : netadapter( udp::create() ) { }
    udpadapter::~udpadapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Parse the host to connection's host address
    bool udpadapter::connect( const std::string& host, duration_t timedout ) {
        size_t _pp = host.find(":");
        if ( _pp == std::string::npos ) return false;
        std::string _addr = host.substr(0, _pp);
        uint16_t _port = (uint16_t)std::stoi(host.substr(_pp + 1));
        ip_t _ip = get_hostname(_addr);
        if ( !_ip.is_valid() ) return false;
        this->peer_addr = (struct sockaddr_in)peer_t(_ip, _port);
        return true;
    }

    // Write Data
    bool udpadapter::write( std::string&& data, duration_t timedout ) {
        return op_done == udp::write_to(t, data.c_str(), data.size(), this->peer_addr, timedout);
    }
    bool udpadapter::write( const std::string& data, duration_t timedout ) {
        return op_done == udp::write_to(t, data.c_str(), data.size(), this->peer_addr, timedout);
    }
    bool udpadapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == udp::write_to(t, data, len, this->peer_addr, timedout);
    }
    bool udpadapter::direct_write( std::string && data, duration_t timedout ) {
        return op_done == udp::write_to(t, data.c_str(), data.size(), this->peer_addr, timedout);
    }
    bool udpadapter::direct_write( const std::string & data, duration_t timedout ) {
        return op_done == udp::write_to(t, data.c_str(), data.size(), this->peer_addr, timedout);
    }
    bool udpadapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == udp::write_to(t, data, len, this->peer_addr, timedout);
    }

    // Wait for data
    iadapter::data_type udpadapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = udp::read_from(t, _d.second, &this->peer_addr, timedout);
        return _d;
    }

    // Create a uds socket and bind to the adapter
    udsadapter::udsadapter() : netadapter( uds::create() ) { }
    udsadapter::~udsadapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Parse the host to connection's host address
    bool udsadapter::connect( const std::string& host, duration_t timedout ) {
        task_t _t = this_task::get_task();
        this->step([host, _t, timedout]() {
            task_helper _ot(_t);
            if ( op_done != uds::connect( host, timedout ) ) return;
            _ot.job_done();
        });
        return this_task::holding();
    }

    // Write Data
    bool udsadapter::write( std::string&& data, duration_t timedout ) {
        return op_done == uds::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool udsadapter::write( const std::string& data, duration_t timedout ) {
        return op_done == uds::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool udsadapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == uds::write_to(t, data, len, timedout);
    }
    bool udsadapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == uds::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool udsadapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == uds::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool udsadapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == uds::write_to(t, data, len, timedout);
    }

    // Wait for data
    iadapter::data_type udsadapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = uds::read_from(t, _d.second, timedout);
        return _d;
    }

    uds_serveradapter::~uds_serveradapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Write Data
    bool uds_serveradapter::write( std::string&& data, duration_t timedout ) {
        return op_done == uds::write_to( t_, data.c_str(), data.size(), timedout );
    }
    bool uds_serveradapter::write( const std::string& data, duration_t timedout ) {
        return op_done == uds::write_to( t_, data.c_str(), data.size(), timedout );
    }
    bool uds_serveradapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == uds::write_to(t_, data, len, timedout);
    }
    bool uds_serveradapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == uds::write_to( t_, data.c_str(), data.size(), timedout );
    }
    bool uds_serveradapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == uds::write_to( t_, data.c_str(), data.size(), timedout );
    }
    bool uds_serveradapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == uds::write_to(t_, data, len, timedout);
    }

    // Wait for data
    iadapter::data_type uds_serveradapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = uds::read_from(t_, _d.second, timedout);
        return _d;
    }
    // Create a socks5 socket and bind to the adapter
    socks5adapter::socks5adapter( const peer_t& socks5 ) 
        : netadapter( socks5::create() ), socks5_addr(socks5) { }
    socks5adapter::~socks5adapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Parse the host to connection's host address
    bool socks5adapter::connect( const std::string& host, duration_t timedout ) {
        size_t _pp = host.find(":");
        if ( _pp == std::string::npos ) return false;
        std::string _addr = host.substr(0, _pp);
        uint16_t _port = (uint16_t)std::stoi(host.substr(_pp + 1));
        task_t _t = this_task::get_task();
        this->step([_addr, _port, this, _t, timedout]() {
            task_helper _ot(_t);
            socks5::connect_address _ca;
            _ca.proxy.addr = this->socks5_addr.ip.str();
            _ca.proxy.port = this->socks5_addr.port;
            _ca.target.addr = _addr;
            _ca.target.port = _port;
            if ( op_done != socks5::connect( _ca, timedout ) ) return;
            _ot.job_done();
        });
        return this_task::holding();
    }

    // Write Data
    bool socks5adapter::write( std::string&& data, duration_t timedout ) {
        return op_done == socks5::write_to(t, data.c_str(), data.size(), timedout );
    }
    bool socks5adapter::write( const std::string& data, duration_t timedout ) {
        return op_done == socks5::write_to(t, data.c_str(), data.size(), timedout );
    }
    bool socks5adapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == socks5::write_to(t, data, len, timedout);
    }
    bool socks5adapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == socks5::write_to(t, data.c_str(), data.size(), timedout );
    }
    bool socks5adapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == socks5::write_to(t, data.c_str(), data.size(), timedout );
    }
    bool socks5adapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == socks5::write_to(t, data, len, timedout);
    }

    // Wait for data
    iadapter::data_type socks5adapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = socks5::read_from( t, _d.second, timedout );
        return _d;
    }

    socks5_serveradapter::~socks5_serveradapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Write Data
    bool socks5_serveradapter::write( std::string&& data, duration_t timedout ) {
        return op_done == socks5::write_to(t_, data.c_str(), data.size(), timedout );
    }
    bool socks5_serveradapter::write( const std::string& data, duration_t timedout ) {
        return op_done == socks5::write_to(t_, data.c_str(), data.size(), timedout );
    }
    bool socks5_serveradapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == socks5::write_to(t_, data, len, timedout);
    }
    bool socks5_serveradapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == socks5::write_to(t_, data.c_str(), data.size(), timedout );
    }
    bool socks5_serveradapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == socks5::write_to(t_, data.c_str(), data.size(), timedout );
    }
    bool socks5_serveradapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == socks5::write_to(t_, data, len, timedout);
    }

    // Wait for data
    iadapter::data_type socks5_serveradapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = socks5::read_from(t_, _d.second, timedout);
        return _d;
    }

    // Create a socks5 socket and bind to the adapter
    socks5sadapter::socks5sadapter( const peer_t& socks5 ) 
        : netadapter( socks5::create() ), socks5_addr(socks5),
        ssl_(std::bind(SSL_new, ssl::get_ctx< ssl::ctx_type_client >()), SSL_free) { 
        // Bind SSL Context
        ssl::set_ssl( this->t, ssl_.ptr_object );
    }
    socks5sadapter::~socks5sadapter() {
        if ( destory_flag_ == false ) {
            destory_flag_ = true;
            quit_task_ = this_task::get_task();
            this_task::holding();
        }
    }

    // Parse the host to connection's host address
    bool socks5sadapter::connect( const std::string& host, duration_t timedout ) {
        size_t _pp = host.find(":");
        if ( _pp == std::string::npos ) return false;
        std::string _addr = host.substr(0, _pp);
        uint16_t _port = (uint16_t)std::stoi(host.substr(_pp + 1));
        task_t _t = this_task::get_task();
        this->step([_addr, _port, this, _t, timedout]() {
            task_helper _ot(_t);
            socks5::connect_address _ca;
            _ca.proxy.addr = this->socks5_addr.ip.str();
            _ca.proxy.port = this->socks5_addr.port;
            _ca.target.addr = _addr;
            _ca.target.port = _port;
            if ( op_done != socks5::connect( _ca, timedout ) ) return;

            // Use the socks5 tunnel to build a ssl connection
            SOCKET_T _so = (SOCKET_T)pe::co::this_task::get_id();
            ssl::ssl_t _ssl = (ssl::ssl_t)this_task::get_arg();
            if ( ! ssl::connect_socket_to_ssl_env(_so, _ssl) ) return;

            _ot.job_done();
        });
        return this_task::holding();
    }

    // Write Data
    bool socks5sadapter::write( std::string&& data, duration_t timedout ) {
        return op_done == ssl::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool socks5sadapter::write( const std::string& data, duration_t timedout ) {
        return op_done == ssl::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool socks5sadapter::write( const char* data, size_t len, duration_t timedout) {
        return op_done == ssl::write_to(t, data, len, timedout);
    }
    bool socks5sadapter::direct_write( std::string&& data, duration_t timedout ) {
        return op_done == ssl::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool socks5sadapter::direct_write( const std::string& data, duration_t timedout ) {
        return op_done == ssl::write_to(t, data.c_str(), data.size(), timedout);
    }
    bool socks5sadapter::direct_write( const char* data, size_t len, duration_t timedout) {
        return op_done == ssl::write_to(t, data, len, timedout);
    }

    // Wait for data
    iadapter::data_type socks5sadapter::read( duration_t timedout ) {
        iadapter::data_type _d;
        _d.first = ssl::read_from(t, _d.second, timedout);
        return _d;
    }

}}}

// Push Chen
