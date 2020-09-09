/*
    obj.h
    PECoNet
    2019-05-12
    Push Chen

    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#pragma once

#ifndef PE_CO_NET_OBJ_H__
#define PE_CO_NET_OBJ_H__

#include <peco/conet/utils/basic.h>

namespace pe { namespace co { namespace net {
    /*!
    The IP object, compatible with std::string and uint32_t
    This is a ipv4 ip address class.
    */
    class ip_t {
        std::string          ip_;

        // Internal Conversition Methods
        static uint32_t __ipstring2inaddr(const std::string& ipstring);
        static void __inaddr2ipstring(uint32_t inaddr, std::string& ipstring);
    public:
        // Basic Empty Constructure
        ip_t();

        // Copy & Move Operators
        ip_t(const ip_t& rhs);
        ip_t(ip_t&& lhs);
        ip_t& operator = (const ip_t& rhs);
        ip_t& operator = (ip_t&& lhs);

        // Conversition
        explicit ip_t(const std::string &ipaddr);
        explicit ip_t(uint32_t ipaddr);
        ip_t& operator = (const std::string &ipaddr);
        ip_t& operator = (uint32_t ipaddr);

        // Return if the IP is validate
        explicit operator bool const();
        bool is_valid() const;

        // Conversition operators
        explicit operator uint32_t() const;
        const string& str() const;
        const char *c_str() const;

        // string length of the ip address
        size_t size() const;

        // Operators
        bool operator == (const ip_t& rhs) const;
        bool operator != (const ip_t& rhs) const;
        bool operator <(const ip_t& rhs) const;
        bool operator >(const ip_t& rhs) const;
        bool operator <=(const ip_t& rhs) const;
        bool operator >=(const ip_t& rhs) const;
    };

    // Output
    std::ostream & operator << (std::ostream &os, const ip_t & ip);

    /*!
    IP Range and IP Mask
    support format:
        x.x.x.x
        x.x.x.x/24
    mask should not bigger than 32
    */
    class iprange_t {
    private:
        ip_t            low_;
        ip_t            high_;
        uint32_t        mask_;

        void __parse_range(const std::string &fmt);
    public:
        iprange_t();

        // Copye
        iprange_t(const iprange_t& rhs);
        iprange_t(iprange_t&& lhs);
        iprange_t& operator =(const iprange_t& rhs);
        iprange_t& operator =(iprange_t&& lhs);

        // Format
        explicit iprange_t(const std::string & fmt);
        iprange_t(const ip_t& from, const ip_t& to);

        // Validation
        explicit operator bool() const;

        // Output string
        explicit operator const std::string() const;
        string str() const;

        // Get the mask
        uint32_t mask() const;
        // Get Origin
        const ip_t& origin() const;

        // Check if an ip is in the range
        bool isIpInRange(const ip_t& ip) const;
    };

    // Output the ip range
    std::ostream & operator << (std::ostream &os, const iprange_t &range);

    /*!
    Peer Info, contains an IP address and a port number.
    should be output in the following format: 0.0.0.0:0
    */
    class peer_t {
    public:
        typedef std::pair<ip_t, uint16_t>   __inner_peer_t;
    private:
        ip_t            ip_;
        uint16_t        port_;
        std::string     format_;

        // Parse the peer string in certen format
        void __parse_peer(const std::string& fmt);
    public:
        const ip_t &        ip;
        const uint16_t &    port;

        // Constructure
        peer_t();

        // Copy
        peer_t(const peer_t& rhs);
        peer_t(peer_t&& lhs);
        peer_t& operator = (const peer_t& rhs);
        peer_t& operator = (peer_t&& lhs);

        // Setting
        peer_t(const ip_t& ip, uint16_t p);
        explicit peer_t(const __inner_peer_t& ipt);
        explicit peer_t(const std::string& fmt);
        explicit peer_t(const struct sockaddr_in addr);

        peer_t& operator = (const __inner_peer_t& ipt);
        peer_t& operator = (const std::string& fmt);
        peer_t& operator = (const struct sockaddr_in addr);

        // Operators
        operator bool() const;

        // Conversitions
        operator struct sockaddr_in() const;
        const string& str() const;
        const char *c_str() const;
        size_t size() const;

        // Get an empty peer info
        static const peer_t nan;
    };

    // Output the peer info
    std::ostream & operator << (std::ostream &os, const peer_t &peer);
}}}

#endif

// Push Chen
//
