/*
    obj.cpp
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/conet/utils/obj.h>

namespace pe { namespace co { namespace net {
    // Internal Conversition Methods
    uint32_t ip_t::__ipstring2inaddr(const std::string& ipstring) {
        return inet_addr(ipstring.c_str());
    }

    void ip_t::__inaddr2ipstring(uint32_t inaddr, std::string& ipstring) {
        ipstring.resize(16);
        int _l = sprintf( &ipstring[0], "%u.%u.%u.%u",
            (inaddr >> (0 * 8)) & 0x00FF,
            (inaddr >> (1 * 8)) & 0x00FF,
            (inaddr >> (2 * 8)) & 0x00FF,
            (inaddr >> (3 * 8)) & 0x00FF 
        );
        ipstring.resize(_l);
    }

    // Basic Empty Constructure
    ip_t::ip_t() : ip_("255.255.255.255") {}

    // Copy & Move Operators
    ip_t::ip_t(const ip_t& rhs) : ip_(rhs.ip_) { }
    ip_t::ip_t(ip_t&& lhs) : ip_(move(lhs.ip_)) { }
    ip_t& ip_t::operator = (const ip_t& rhs) {
        if ( this == &rhs ) return *this;
        ip_ = rhs.ip_;
        return *this;
    }
    ip_t& ip_t::operator = (ip_t&& lhs) {
        if ( this == &lhs ) return *this;
        ip_ = move(lhs.ip_);
        return *this;
    }

    // Conversition
    ip_t::ip_t(const std::string &ipaddr) { 
        ip_t::__inaddr2ipstring(
            ip_t::__ipstring2inaddr(ipaddr), ip_);
    }
    ip_t::ip_t(uint32_t ipaddr) {
        ip_t::__inaddr2ipstring(ipaddr, ip_);
    }
    ip_t& ip_t::operator = (const std::string &ipaddr) {
        ip_t::__inaddr2ipstring(
            ip_t::__ipstring2inaddr(ipaddr), ip_);
        return *this;
    }
    ip_t& ip_t::operator = (uint32_t ipaddr) {
        ip_t::__inaddr2ipstring(ipaddr, ip_);
        return *this;
    }

    ip_t::operator bool const() {
        return this->is_valid();
    }
    bool ip_t::is_valid() const {
        return (uint32_t)-1 != this->operator uint32_t();
    }
    // Conversition operators
    ip_t::operator uint32_t() const { return ip_t::__ipstring2inaddr(ip_); }
    const string& ip_t::str() const { return ip_; }
    const char* ip_t::c_str() const { return ip_.c_str(); }

    // string length of the ip address
    size_t ip_t::size() const { return ip_.size(); }

    // Operators
    bool ip_t::operator == (const ip_t& rhs) const { return ip_ == rhs.ip_; }
    bool ip_t::operator != (const ip_t& rhs) const { return ip_ != rhs.ip_; }
    bool ip_t::operator <(const ip_t& rhs) const { 
        return ntohl((uint32_t)(*this)) < ntohl((uint32_t)rhs); 
    }
    bool ip_t::operator >(const ip_t& rhs) const { 
        return ntohl((uint32_t)(*this)) > ntohl((uint32_t)rhs); 
    }
    bool ip_t::operator <=(const ip_t& rhs) const { 
        return ntohl((uint32_t)(*this)) <= ntohl((uint32_t)rhs); 
    }
    bool ip_t::operator >=(const ip_t& rhs) const { 
        return ntohl((uint32_t)(*this)) >= ntohl((uint32_t)rhs); 
    }

    // Output
    std::ostream & operator << (std::ostream &os, const ip_t & ip) {
        os << ip.str(); return os;
    }

    void iprange_t::__parse_range(const std::string &fmt) {
        // This is invalidate range
        size_t _slash_pos = fmt.find('/');
        std::string _ipstr, _maskstr;
        if ( _slash_pos == std::string::npos ) {
            _ipstr = fmt;
            _maskstr = "32";
        } else {
            _ipstr = fmt.substr(0, _slash_pos);
            _maskstr = fmt.substr(_slash_pos + 1);
        }

        ip_t _lowip(_ipstr);
        if ( (uint32_t)_lowip == 0 ) return;  // Invalidate 

        // Mask part
        if ( _maskstr.size() > 2 ) {
            ip_t _highip(_maskstr);
            if ( (uint32_t)_highip == 0 ) return;     // Invalidate Second Part
            if ( _highip < _lowip ) return; // Invalidate order of the range
            low_ = move(_lowip);
            high_ = move(_highip);

            // Try to calculate the mask_
            uint32_t _d = (uint32_t)high_ - (uint32_t)low_;
            uint32_t _bc = 0;
            while ( _d != 0 ) {
                _bc += 1; _d >>= 1;
            }
            mask_ = 32 - _bc;
        } else {
            uint32_t _mask = stoi(_maskstr, nullptr, 10);
            if ( _mask > 32 ) return;
            uint32_t _fmask = 0xFFFFFFFF;
            _fmask <<= (32 - _mask);

            uint32_t _low = ntohl((uint32_t)_lowip) & _fmask;
            uint32_t _high = _low | (~_fmask);
            low_ = htonl(_low);
            high_ = htonl(_high);

            mask_ = _mask;
        }
    }

    iprange_t::iprange_t() : low_((uint32_t)0), high_((uint32_t)0), mask_(32u) { }

    // Copy
    iprange_t::iprange_t(const iprange_t& rhs) : 
        low_(rhs.low_), high_(rhs.high_), mask_(rhs.mask_) { }
    iprange_t::iprange_t(iprange_t&& lhs) : 
        low_(move(lhs.low_)), high_(move(lhs.high_)), mask_(move(lhs.mask_)) { }
    iprange_t& iprange_t::operator =(const iprange_t& rhs) {
        if ( this == &rhs ) return *this;
        low_ = rhs.low_; high_ = rhs.high_; mask_ = rhs.mask_;
        return *this;
    }
    iprange_t& iprange_t::operator =(iprange_t&& lhs) {
        if ( this == &lhs ) return *this;
        low_ = move(lhs.low_); high_ = move(lhs.high_); mask_ = move(lhs.mask_);
        return *this;
    }

    // Format
    iprange_t::iprange_t(const std::string & fmt) {
        this->__parse_range(fmt);
    }
    iprange_t::iprange_t(const ip_t& from, const ip_t& to) {
        if ( from < to ) { low_ = from; high_ = to; }
        else { low_ = to; high_ = from; }
        // Try to calculate the mask_
        uint32_t _d = ntohl((uint32_t)high_) - ntohl((uint32_t)low_);
        uint32_t _bc = 0;
        while ( _d != 0 ) {
            _bc += 1; _d >>= 1;
        }
        mask_ = 32 - _bc;
    }

    // Validation
    iprange_t::operator bool() const {
        return (uint32_t)low_ != (uint32_t)0 && (uint32_t)high_ != (uint32_t)0;
    }

    // Get the mask
    uint32_t iprange_t::mask() const {
        return mask_;
    }
    // Get Origin
    const ip_t& iprange_t::origin() const {
        return low_;
    }

    // Output string
    iprange_t::operator const std::string() const {
        return low_.str() + "/" + to_string(mask_);
        // return low_.str() + " - " + high_.str();
    }

    string iprange_t::str() const {
        return low_.str() + "/" + to_string(mask_);
        // return low_.str() + " - " + high_.str();
    }
    // Check if an ip is in the range
    bool iprange_t::isIpInRange(const ip_t& ip) const {
        return low_ <= ip && ip <= high_;
    }

    // Output the ip range
    std::ostream & operator << (std::ostream &os, const iprange_t &range) {
        os << range.str();
        return os;
    }


    // Parse the peer string in certen format
    void peer_t::__parse_peer(const std::string& fmt) {
        size_t _pos = fmt.rfind(":");
        if ( _pos == std::string::npos ) {
            ip_ = "0.0.0.0";
            port_ = stoi(fmt, nullptr, 10);
            format_ = "0.0.0.0:" + fmt;
        } else {
            ip_ = fmt.substr(0, _pos);
            port_ = stoi(fmt.substr(_pos + 1), nullptr, 10);
            format_ = fmt;
        }
    }

    // Constructure
    peer_t::peer_t() : ip_(0), port_(0), format_("0.0.0.0:0"), ip(ip_), port(port_) { }

    // Copy
    peer_t::peer_t(const peer_t& rhs) : 
        ip_(rhs.ip_), port_(rhs.port_), format_(rhs.format_),
        ip(ip_), port(port_) { }
    peer_t::peer_t(peer_t&& lhs) : 
        ip_(move(lhs.ip_)), port_(move(lhs.port_)), format_(move(lhs.format_)),
        ip(ip_), port(port_) { }
    peer_t& peer_t::operator = (const peer_t& rhs) {
        if ( this == &rhs ) return *this;
        ip_ = rhs.ip_;
        port_ = rhs.port_;
        format_ = rhs.format_;
        return *this;
    }
    peer_t& peer_t::operator = (peer_t&& lhs) {
        if ( this == &lhs ) return *this;
        ip_ = move(lhs.ip_);
        port_ = move(lhs.port_);
        format_ = move(lhs.format_);
        return *this;
    }

    // Setting
    peer_t::peer_t(const ip_t& ip, uint16_t p) : 
        ip_(ip), port_(p), ip(ip_), port(port_) {
        format_ = (const std::string&)ip_;
        format_ += ":";
        format_ += to_string(port_);
    }
    peer_t::peer_t(const __inner_peer_t& ipt) : 
        ip_(ipt.first), port_(ipt.second), ip(ip_), port(port_) {
        format_ = (const std::string&)ip_;
        format_ += ":";
        format_ += to_string(port_);
    }

    peer_t::peer_t(const std::string& fmt) : ip(ip_), port(port_) {
        this->__parse_peer(fmt);
    }
    peer_t::peer_t(const struct sockaddr_in addr) : 
        ip_(addr.sin_addr.s_addr), port_(ntohs(addr.sin_port)), 
        ip(ip_), port(port_) {
        format_ = (const std::string&)ip_;
        format_ += ":";
        format_ += to_string(port_);
    }

    peer_t& peer_t::operator = (const __inner_peer_t& ipt) {
        ip_ = ipt.first;
        port_ = ipt.second;
        format_ = (const std::string&)ip_;
        format_ += ":";
        format_ += to_string(port_);
        return *this;
    }
    peer_t& peer_t::operator = (const std::string& fmt) {
        this->__parse_peer(fmt);
        return *this;
    }
    peer_t& peer_t::operator = (const struct sockaddr_in addr) {
        ip_ = addr.sin_addr.s_addr;
        port_ = ntohs(addr.sin_port);
        format_ = (const std::string&)ip_;
        format_ += ":";
        format_ += to_string(port_);
        return *this;
    }

    // Operators
    peer_t::operator bool() const { return port_ > 0 && port_ <= 65535; }

    // Conversitions
    peer_t::operator struct sockaddr_in() const {
        struct sockaddr_in _addr;
        _addr.sin_family = AF_INET;
        _addr.sin_addr.s_addr = (uint32_t)ip_;
        _addr.sin_port = htons(port_);
        return _addr;
    }
    const string& peer_t::str() const { return format_; }
    const char* peer_t::c_str() const { return format_.c_str(); }
    size_t peer_t::size() const { return format_.size(); }

    // Get an empty peer info
    const peer_t peer_t::nan;

    // Output the peer info
    std::ostream & operator << (std::ostream &os, const peer_t &peer) {
        os << peer.str();
        return os;
    }        
}}}

// Push Chen
//
