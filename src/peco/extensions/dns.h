/*
    dns.h
    libpeco
    2022-02-25
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

#pragma once

#ifndef PECO_EXTENSIONS_DNS_H__
#define PECO_EXTENSIONS_DNS_H__

#include "peco/pecostd.h"
#include "peco/net.h"

namespace peco {

// DNS Question Type
typedef enum {
  dns_qtype_host              = 0x01,     // Host(A) record
  dns_qtype_ns                = 0x02,     // Name server (NS) record
  dns_qtype_cname             = 0x05,     // Alias(CName) record
  dns_qtype_ptr               = 0x0C,     // Reverse-lookup(PTR) record
  dns_qtype_mx                = 0x0F,     // Mail exchange(MX) record
  dns_qtype_srv               = 0x21,     // Service(SRV) record
  dns_qtype_ixfr              = 0xFB,     // Incremental zone transfer(IXFR) record
  dns_qtype_axfr              = 0xFC,     // Standard zone transfer(AXFR) record
  dns_qtype_all               = 0xFF      // All records
} dns_qtype;

// DNS Question Class
typedef enum {
  dns_qclass_in               = 0x0001,   // Represents the IN(internet) 
                                          // question and is normally set to 0x0001
  dns_qclass_ch               = 0x0003,   // the CHAOS class
  dns_qclass_hs               = 0x0004    // Hesiod   
} dns_qclass;

typedef enum {
  dns_opcode_standard         = 0,
  dns_opcode_inverse          = 1,
  dns_opcode_status           = 2,
  dns_opcode_reserved_3       = 3,        // not use
  dns_opcode_notify           = 4,        // in RFC 1996
  dns_opcode_update           = 5         // in RFC 2136
} dns_opcode;

typedef enum {
  dns_rcode_noerr                 = 0,
  dns_rcode_format_error          = 1,
  dns_rcode_server_failure        = 2,
  dns_rcode_name_error            = 3,
  dns_rcode_not_impl              = 4,
  dns_rcode_refuse                = 5,
  dns_rcode_yxdomain              = 6,
  dns_rcode_yxrrset               = 7,
  dns_rcode_nxrrset               = 8,
  dns_rcode_notauth               = 9,
  dns_rcode_notzone               = 10,
  dns_rcode_badvers               = 16,
  dns_rcode_badsig                = 16,
  dns_rcode_badkey                = 17,
  dns_rcode_badtime               = 18,
  dns_rcode_badmode               = 19,
  dns_rcode_badname               = 20,
  dns_rcode_badalg                = 21
} dns_rcode;


#pragma pack( push, 1 )
// DNS Package
struct dns_packet_header {
  // 1
  uint16_t    trans_id;
  // 2
  uint8_t     is_recursive_desired:   1;
  uint8_t     is_truncation:          1;
  uint8_t     is_authoritative:       1;
  uint8_t     opcode:                 4;
  uint8_t     qrcode:                 1;

  uint8_t     respcode:               4;
  uint8_t     checking_disabled:      1;
  uint8_t     authenticated:          1;
  uint8_t     reserved:               1;
  uint8_t     is_recursive_available: 1;
  // 3
  uint16_t    qdcount;
  // 4
  uint16_t    ancount;
  // 5
  uint16_t    nscount;
  // 6
  uint16_t    arcount;
};

// DNS Packet
struct dns_packet {
  char *              packet;
  uint16_t            length;
  uint16_t            buflen;
  dns_packet_header   *header;
  uint16_t            dsize;
};

// A Record
struct dns_a_record {
  ip_t                ip;
  dns_qclass          qclass;
  time_t              ttl;
};
// CName Record
struct dns_cname_record {
  std::string         domain;
  dns_qclass          qclass;
  uint32_t            ttl;
};

// Record basic part
struct dns_map_basic {
  uint16_t            qtype;
  uint16_t            qclass;
  uint32_t            ttl;
  uint16_t            rlen;
};

// Basic Record Map
struct dns_record_map_basic {
  uint16_t            name;
  uint16_t            qtype;
  uint16_t            qclass;
  uint32_t            ttl;
  uint16_t            rlen;
};

// A Record Map 
struct dns_arecord_map : public dns_record_map_basic {
  uint32_t            rdata;
};

#pragma pack(pop)

namespace dns_utils {

// Init a dns packet's buffer
void init_dns_packet( dns_packet* pkt );
void release_dns_packet( dns_packet* pkt );

// Prepare for read or write
void prepare_for_reading( dns_packet* pkt );
void prepare_for_sending( dns_packet* pkt );

// get the data buffer's beginning
char *pkt_begin( dns_packet* pkt );

// append a records
void set_ips( dns_packet* pkt, const std::list<ip_t>& ips );
std::list<dns_a_record> ips( dns_packet* pkt );

// process cname records
void set_cname( dns_packet* pkt, const std::string& cname );
dns_cname_record cname( dns_packet* pkt );

// query domain
void set_domain( dns_packet* pkt, const std::string& domain );
std::string domain( dns_packet* pkt );

// Get qtype
dns_qtype qtype( dns_packet* pkt );
void set_qtype( dns_packet* pkt, dns_qtype qtype = dns_qtype_host );
// Get qclass
dns_qclass qclass( dns_packet* pkt );
void set_qclass( dns_packet* pkt, dns_qclass qclass = dns_qclass_in );

} // namespace dns_utils

/**
 * @brief dns resolv
*/
class dns_resolver {
public:
  /**
   * @brief Default c'stor, with no dns server, will not resolve any domain
  */
  dns_resolver();
  /**
   * @brief Create the dns resolver with given dns server
  */
  dns_resolver(const peer_t& dns_server);
  dns_resolver(const std::string& dns_server);

  /**
   * @brief Copy & Move
  */
  dns_resolver(const dns_resolver& other);
  dns_resolver(dns_resolver&& other);
  dns_resolver& operator=(const dns_resolver& other);
  dns_resolver& operator=(dns_resolver&& other);

  /**
   * @brief Change the dns server
  */
  void set_dns_server(const peer_t& dns_server);
  void set_dns_server(const std::string& dns_server);

  /**
   * @brief Do the resolv action
  */
  std::list<dns_a_record> resolv(const std::string& domain);
  std::list<dns_a_record> operator()(const std::string& domain);
  
protected:
  peer_t dns_server_;
};

} // namespace peco

#endif

// Push Chen
