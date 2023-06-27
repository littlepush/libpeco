/*
    dns.cpp
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

#include "extensions/dns.h"

namespace peco {

namespace dns_utils {

// Basic Domain Parser
size_t __get_domain_basic(const char *data, std::string &domain) {
  for (;;) {
    uint8_t _l = data[0];
    if (_l == 0)
      break;
    ++data;
    if (domain.size() > 0)
      domain += '.';
    domain.append(data, _l);
    data += _l;
  }
  // All size is equal to the domain length + first _l + last \0
  return domain.size() + (size_t)2;
}
// Count the domain part size
size_t __count_domain_part(const char *p) {
  size_t _rs = 0;
  for (;;) {
    uint8_t _l = p[_rs];
    ++_rs;
    if ((_l & 0xC0) == 0xC0) {
      // Offset
      _rs += 1;
      break;
    } else {
      if (_l == 0)
        break;
      _rs += _l;
    }
  }
  return _rs;
}
std::string __get_domain(const char *p, const char *b) {
  std::string _d;
  size_t _rs = 0;
  for (;;) {
    uint8_t _l = p[_rs];
    ++_rs;
    if ((_l & 0xC0) == 0xC0) {
      // Offset
      std::string _rest_d;
      uint16_t _offset = (_l & 0x3F);
      if (_offset == 0) {
        _offset = ntohs(*(uint16_t *)(p + _rs - 1));
        _offset &= 0x3FFF;
      }
      __get_domain_basic(b + _offset, _rest_d);
      _d += _rest_d;
      break;
    } else {
      if (_l == 0)
        break;
      if (_d.size() > 0)
        _d += '.';
      _d.append(p + _rs, _l);
      _rs += _l;
    }
  }
  return _d;
}
size_t __set_domain(char *p, const std::string &domain) {
  // Copy the domain
  char *_db = p;
  size_t _i = 0;
  do {
    char *_l = _db;
    ++_db;
    uint8_t _pl = 0;

    while (_i < domain.size() && domain[_i] != '.') {
      *_db = domain[_i];
      // Increase everything
      ++_i;
      ++_pl;
      ++_db;
    }
    *((uint8_t *)_l) = _pl;
    ++_i;
    _pl = 0;
  } while (_i < domain.size());
  *_db = '\0';
  return domain.size() + 2;
}

// Init a dns packet's buffer
void init_dns_packet(dns_packet *pkt) {
  pkt->packet = (char *)malloc(sizeof(char) * 514);
  memset(pkt->packet, 0, sizeof(dns_packet_header) + sizeof(uint16_t));
  pkt->length = sizeof(dns_packet_header);
  pkt->buflen = 514;
  pkt->header = (dns_packet_header *)(pkt->packet + sizeof(uint16_t));
}
void release_dns_packet(dns_packet *pkt) { free(pkt->packet); }

// Prepare for read or write
void prepare_for_reading(dns_packet *pkt) {
  pkt->header->trans_id = ntohs(pkt->header->trans_id);
  pkt->header->qdcount = ntohs(pkt->header->qdcount);
  pkt->header->ancount = ntohs(pkt->header->ancount);
  pkt->header->nscount = ntohs(pkt->header->nscount);
  pkt->header->arcount = ntohs(pkt->header->arcount);
}
void prepare_for_sending(dns_packet *pkt) {
  *(uint16_t *)pkt->packet = htons(pkt->length);
  pkt->header->trans_id = htons(pkt->header->trans_id);
  pkt->header->qdcount = htons(pkt->header->qdcount);
  pkt->header->ancount = htons(pkt->header->ancount);
  pkt->header->nscount = htons(pkt->header->nscount);
  pkt->header->arcount = htons(pkt->header->arcount);
}

// get the data buffer's beginning
char *pkt_begin(dns_packet *pkt) { return (char *)pkt->header; }

// append a records
void set_ips(dns_packet *pkt, const std::list<ip_t> &ips) {
  if (pkt->dsize == 0)
    return;
  // Set for response
  pkt->header->qrcode = 0;
  // no error
  pkt->header->respcode = (uint16_t)dns_rcode_noerr;
  // set an count
  pkt->header->ancount = (uint16_t)ips.size();

  // Begin + Domain + QType + QClass
  char *_pbuf = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;

  // Offset
  uint16_t _offset = htons(sizeof(dns_packet_header) | 0xC000);

  // Append all data
  for (auto &ip : ips) {
    dns_arecord_map *_amap = (dns_arecord_map *)_pbuf;
    _amap->name = _offset;
    _amap->qtype = htons((uint16_t)dns_qtype_host);
    _amap->qclass = htons((uint16_t)dns_qclass_in);
    _amap->ttl = htonl(30 * 60); // 30 mins
    _amap->rlen = htons(sizeof(uint32_t));
    _amap->rdata = (uint32_t)ip;
    _pbuf += sizeof(dns_arecord_map);
  }
  pkt->length += (ips.size() * sizeof(dns_arecord_map));
}
std::list<dns_a_record> ips(dns_packet *pkt) {
  // Try to check the domain size
  if (pkt->dsize == 0) {
    std::string _qd = domain(pkt);
  }
  std::list<dns_a_record> _result;

  // Begin + Domain + QType + QClass
  const char *_pbuf =
    pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;
  uint16_t _c = (pkt->header->ancount + pkt->header->nscount);
  for (uint16_t i = 0; i < _c; ++i) {
    // Skip the domain part
    size_t _ds = __count_domain_part(_pbuf);
    _pbuf += _ds;

    // Map the basic part
    dns_map_basic *_mb = (dns_map_basic *)_pbuf;
    bool _is_a = ((dns_qtype)ntohs(_mb->qtype) == dns_qtype_host);
    _pbuf += sizeof(dns_map_basic);

    // If this is an ARecord
    if (_is_a) {
      dns_a_record _ar = {ip_t(*(uint32_t *)_pbuf),
                          (dns_qclass)ntohs(_mb->qclass),
                          time(NULL) + ntohl(_mb->ttl)};
      _result.emplace_back(_ar);
    }
    _pbuf += (size_t)ntohs(_mb->rlen);
  }
  return _result;
}

// process cname records
void set_cname(dns_packet *pkt, const std::string &cname) {
  if (pkt->dsize == 0)
    return;
  // Set for response
  pkt->header->qrcode = 0;
  // no error
  pkt->header->respcode = (uint16_t)dns_rcode_noerr;
  // set an count
  pkt->header->ancount = 1;

  // Begin + Domain + QType + QClass
  char *_pbuf = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;

  // Offset
  uint16_t _offset = htons(sizeof(dns_packet_header) | 0xC000);

  // Append all data
  dns_record_map_basic *_bmap = (dns_record_map_basic *)_pbuf;
  _bmap->name = _offset;
  _bmap->qtype = htons((uint16_t)dns_qtype_cname);
  _bmap->qclass = htons((uint16_t)dns_qclass_in);
  _bmap->ttl = htonl(30 * 60); // 30 mins
  _bmap->rlen = htons(cname.size() + 2);
  // _bmap->rdata = (uint32_t)ip;
  char *_rdata = (char *)(_bmap + 1);
  __set_domain(_rdata, cname);
  pkt->length += (sizeof(dns_record_map_basic) + cname.size() + 2);
}
dns_cname_record cname(dns_packet *pkt) {
  // Try to check the domain size
  if (pkt->dsize == 0) {
    std::string _qd = domain(pkt);
  }
  dns_cname_record _cr;
  const char *_pbuf =
    pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;
  uint16_t _c = (pkt->header->ancount + pkt->header->nscount);
  for (uint16_t i = 0; i < _c; ++i) {
    // Skip the domain part
    size_t _ds = __count_domain_part(_pbuf);
    _pbuf += _ds;

    // Map the basic part
    dns_map_basic *_mb = (dns_map_basic *)_pbuf;
    bool _is_c = ((dns_qtype)ntohs(_mb->qtype) == dns_qtype_cname);
    _pbuf += sizeof(dns_map_basic);

    // If this is a CName record
    if (_is_c) {
      // Get the domain
      _cr.domain = __get_domain(_pbuf, pkt_begin(pkt));
      // Get other info
      _cr.qclass = (dns_qclass)ntohs(_mb->qclass);
      _cr.ttl = (uint32_t)ntohl(_mb->ttl);
      return _cr;
    }
    _pbuf += (size_t)ntohs(_mb->rlen);
  }
  return _cr;
}

// query domain
void set_domain(dns_packet *pkt, const std::string &domain) {
  pkt->header->qrcode = 0; // this is a request
  pkt->header->is_recursive_desired = 1;
  pkt->header->qdcount = 1; // query one domain

  // Copy the domain
  __set_domain(pkt_begin(pkt) + pkt->length, domain);
  pkt->length += (domain.size() + 2);
  pkt->dsize = (uint16_t)(domain.size() + 2);
}
std::string domain(dns_packet *pkt) {
  std::string _d;
  if (pkt->length > (sizeof(dns_packet_header) + 2 + 2)) {
    __get_domain_basic(pkt_begin(pkt) + sizeof(dns_packet_header), _d);
  }
  pkt->dsize = (uint16_t)(_d.size() + 2);
  return _d;
}
// Get qtype
dns_qtype qtype(dns_packet *pkt) {
  char *_e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
  uint16_t *_f = (uint16_t *)_e;
  return (dns_qtype)ntohs(_f[0]);
}
void set_qtype(dns_packet *pkt, dns_qtype qtype) {
  char *_e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
  uint16_t *_f = (uint16_t *)_e;
  pkt->length += sizeof(uint16_t);
  _f[0] = htons(qtype);
}
// Get qclass
dns_qclass qclass(dns_packet *pkt) {
  char *_e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
  uint16_t *_f = (uint16_t *)_e;
  return (dns_qclass)ntohs(_f[1]);
}
void set_qclass(dns_packet *pkt, dns_qclass qclass) {
  char *_e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
  uint16_t *_f = (uint16_t *)_e;
  pkt->length += sizeof(uint16_t);
  _f[1] = htons(qclass);
}

} // namespace dns_utils

// internal dns packet wrapper
class __dns_pkt_guard {
  dns_packet          pkt;
public:
  __dns_pkt_guard( ) { dns_utils::init_dns_packet(&pkt); }
  ~__dns_pkt_guard( ) { dns_utils::release_dns_packet(&pkt); }
  dns_packet * get() { return &pkt; }
};

/**
 * @brief DNS Methods
*/
std::list<dns_a_record> resolv_by_udp(const peer_t& dns_server, const std::string& domain);
std::list<dns_a_record> resolv_by_tcp(const peer_t& dns_server, const std::string& domain);
std::list<dns_a_record> parse_dns_packet(const peer_t& dns_server, dns_packet* pkt);

/**
 * @brief Default c'stor, with no dns server, will not resolve any domain
*/
dns_resolver::dns_resolver() {}
/**
 * @brief Create the dns resolver with given dns server
*/
dns_resolver::dns_resolver(const peer_t& dns_server)
  : dns_server_(dns_server) {}
dns_resolver::dns_resolver(const std::string& dns_server)
  : dns_server_(peer_t(dns_server)) {}

/**
 * @brief Copy & Move
*/
dns_resolver::dns_resolver(const dns_resolver& other)
  : dns_server_(other.dns_server_) { }
dns_resolver::dns_resolver(dns_resolver&& other)
  : dns_server_(std::move(other.dns_server_)) {}
dns_resolver& dns_resolver::operator=(const dns_resolver& other) {
  if (this == &other) return *this;
  dns_server_ = other.dns_server_;
  return *this;
}
dns_resolver& dns_resolver::operator=(dns_resolver&& other) {
  if (this == &other) return *this;
  dns_server_ = std::move(other.dns_server_);
  return *this;
}

/**
 * @brief Change the dns server
*/
void dns_resolver::set_dns_server(const peer_t& dns_server) {
  dns_server_ = dns_server;
}
void dns_resolver::set_dns_server(const std::string& dns_server) {
  dns_server_ = dns_server;
}

/**
 * @brief Do the resolv action
*/
std::list<dns_a_record> dns_resolver::resolv(const std::string& domain) {
  if (!dns_server_) return {};
  return resolv_by_udp(dns_server_, domain);
}
std::list<dns_a_record> dns_resolver::operator()(const std::string& domain) {
  return this->resolv(domain);
}

std::list<dns_a_record> resolv_by_udp(const peer_t& dns_server, const std::string& domain) {
  __dns_pkt_guard pg;
  dns_packet* pkt = pg.get();
  pkt->header->trans_id = (uint16_t)TASK_TIME_NOW().time_since_epoch().count();
  dns_utils::set_domain(pkt, domain);
  dns_utils::set_qtype(pkt);
  dns_utils::set_qclass(pkt);
  dns_utils::prepare_for_sending(pkt);

  auto uc = udp_connector::create();
  uc->connect(dns_server);  // should always success
  if (!uc->write(dns_utils::pkt_begin(pkt), pkt->length)) {
    return {};
  }
  auto r = uc->read();
  dns_packet rpkt;
  rpkt.packet = &r.data[0];
  rpkt.length = r.data.size();
  rpkt.buflen = 0;
  rpkt.header = (dns_packet_header *)rpkt.packet;
  rpkt.dsize = 0;
  if (rpkt.header->is_truncation) {
    return resolv_by_tcp(dns_server, domain);
  }
  return parse_dns_packet(dns_server, &rpkt);
}

std::list<dns_a_record> resolv_by_tcp(const peer_t& dns_server, const std::string& domain) {
  __dns_pkt_guard pg;
  dns_packet* pkt = pg.get();
  pkt->header->trans_id = (uint16_t)TASK_TIME_NOW().time_since_epoch().count();
  dns_utils::set_domain(pkt, domain);
  dns_utils::set_qtype(pkt);
  dns_utils::set_qclass(pkt);
  dns_utils::prepare_for_sending(pkt);

  auto tc = tcp_connector::create();
  if (!tc->connect(dns_server)) {
    return {};
  }
  if (!tc->write(pkt->packet, pkt->length + sizeof(uint16_t))) {
    return {};
  }
  auto r = tc->read();
  dns_packet rpkt;
  rpkt.packet = &r.data[0];
  rpkt.length = r.data.size();
  rpkt.buflen = 0;
  rpkt.header = (dns_packet_header *)(rpkt.packet + sizeof(uint16_t));
  rpkt.dsize = 0;
  return parse_dns_packet(dns_server, &rpkt);
}

std::list<dns_a_record> parse_dns_packet(const peer_t& dns_server, dns_packet* pkt) {
  dns_utils::prepare_for_reading(pkt);
  auto ips = dns_utils::ips(pkt);
  if (ips.size() > 0) {
    return ips;
  }
  auto cname = dns_utils::cname(pkt);
  if (cname.domain.size() > 0) {
    return resolv_by_udp(dns_server, cname.domain);
  }
  // no result
  return {};
}

} // namespace peco

// Push Chen
