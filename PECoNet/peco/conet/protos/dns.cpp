/*
    dns.cpp
    PECoNet
    2019-05-14
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */

#include <peco/conet/protos/dns.h>

#ifndef __APPLE__
#include <limits.h>
#include <linux/netfilter_ipv4.h>
#endif
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <peco/conet/protos/socks5.h>

#include <peco/peutils.h>
#include <peco/conet/conns/netadapter.h>

namespace pe { namespace co { namespace net { namespace proto { namespace dns {
   // Basic Domain Parser
    size_t __get_domain_basic( const char *data, std::string& domain ) {
        for ( ;; ) {
            uint8_t _l = data[0];
            if ( _l == 0 ) break;
            ++data;
            if ( domain.size() > 0 ) domain += '.';
            domain.append( data, _l );
            data += _l;
        }
        // All size is equal to the domain length + first _l + last \0
        return domain.size() + (size_t)2;
    }
    // Count the domain part size
    size_t __count_domain_part(const char *p) {
        size_t _rs = 0;
        for ( ;; ) {
            uint8_t _l = p[_rs];
            ++_rs;
            if ( (_l & 0xC0 ) == 0xC0 ) {
                // Offset
                _rs += 1;
                break;
            } else {
                if ( _l == 0 ) break;
                _rs += _l;
            }
        }
        return _rs;
    }
    std::string __get_domain( const char* p, const char* b ) {
        string _d;
        size_t _rs = 0;
        for ( ;; ) {
            uint8_t _l = p[_rs];
            ++_rs;
            if ( (_l & 0xC0 ) == 0xC0 ) {
                // Offset
                string _rest_d;
                uint16_t _offset = (_l & 0x3F);
                if ( _offset == 0 ) {
                    _offset = ntohs(*(uint16_t *)(p + _rs - 1));
                    _offset &= 0x3FFF;
                }
                __get_domain_basic(b + _offset, _rest_d);
                _d += _rest_d;
                break;
            } else {
                if ( _l == 0 ) break;
                if ( _d.size() > 0 ) _d += '.';
                _d.append(p + _rs, _l);
                _rs += _l;
            }
        }
        return _d;
    }
    size_t __set_domain( char* p, const std::string& domain ) {
        // Copy the domain
        char *_db = p;
        size_t _i = 0;
        do {
            char *_l = _db;
            ++_db;
            uint8_t _pl = 0;

            while ( _i < domain.size() && domain[_i] != '.' ) {
                *_db = domain[_i];
                // Increase everything
                ++_i;
                ++_pl;
                ++_db;
            }
            *((uint8_t *)_l) = _pl;
            ++_i;
            _pl = 0;
        } while ( _i < domain.size() );
        *_db = '\0';
        return domain.size() + 2;
    }

    // Init a dns packet's buffer
    void init_dns_packet( dns_packet* pkt ) {
        pkt->packet = (char *)malloc(sizeof(char) * 514);
        memset( pkt->packet, 0, sizeof(dns_packet_header) + sizeof(uint16_t) );
        pkt->length = sizeof(dns_packet_header);
        pkt->buflen = 514;
        pkt->header = (dns_packet_header *)(pkt->packet + sizeof(uint16_t));
    }
    void release_dns_packet( dns_packet* pkt ) {
        free(pkt->packet);
    }

    // Prepare for read or write
    void prepare_for_reading( dns_packet* pkt ) {
        pkt->header->trans_id = ntohs(pkt->header->trans_id);
        pkt->header->qdcount = ntohs(pkt->header->qdcount);
        pkt->header->ancount = ntohs(pkt->header->ancount);
        pkt->header->nscount = ntohs(pkt->header->nscount);
        pkt->header->arcount = ntohs(pkt->header->arcount);
    }
    void prepare_for_sending( dns_packet* pkt ) {
        *(uint16_t *)pkt->packet = htons(pkt->length);
        pkt->header->trans_id = htons(pkt->header->trans_id);
        pkt->header->qdcount = htons(pkt->header->qdcount);
        pkt->header->ancount = htons(pkt->header->ancount);
        pkt->header->nscount = htons(pkt->header->nscount);
        pkt->header->arcount = htons(pkt->header->arcount);
    }

    // get the data buffer's beginning
    char *pkt_begin( dns_packet* pkt ) {
        return (char *)pkt->header;
    }

    // append a records
    void set_ips( dns_packet* pkt, const std::list<ip_t>& ips ) {
        if ( pkt->dsize == 0 ) return;
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
        for ( auto& ip : ips ) {
            dns_arecord_map * _amap = (dns_arecord_map *)_pbuf;
            _amap->name = _offset;
            _amap->qtype = htons((uint16_t)dns_qtype_host);
            _amap->qclass = htons((uint16_t)dns_qclass_in);
            _amap->ttl = htonl(30 * 60);    // 30 mins
            _amap->rlen = htons(sizeof(uint32_t));
            _amap->rdata = (uint32_t)ip;
            _pbuf += sizeof(dns_arecord_map);
        }
        pkt->length += (ips.size() * sizeof(dns_arecord_map));
    }
    std::list<dns_a_record> ips( dns_packet* pkt ) {
        // Try to check the domain size
        if ( pkt->dsize == 0 ) {
            string _qd = domain( pkt );
        }
        std::list< dns_a_record > _result;

        // Begin + Domain + QType + QClass
        const char *_pbuf = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;
        uint16_t _c = (pkt->header->ancount + pkt->header->nscount);
        for ( uint16_t i = 0; i < _c; ++i ) {
            // Skip the domain part
            size_t _ds = __count_domain_part(_pbuf);
            _pbuf += _ds;

            // Map the basic part
            dns_map_basic * _mb = (dns_map_basic *)_pbuf;
            bool _is_a = ((dns_qtype)ntohs(_mb->qtype) == dns_qtype_host);
            _pbuf += sizeof(dns_map_basic);

            // If this is an ARecord
            if ( _is_a ) {
                dns_a_record _ar = {
                    ip_t( *(uint32_t*)_pbuf ),
                    (dns_qclass)ntohs(_mb->qclass),
                    time(NULL) + ntohl(_mb->ttl)
                };
                _result.emplace_back(_ar);
            }
            _pbuf += (size_t)ntohs(_mb->rlen);
        }
        return _result;
    }

    // process cname records
    void set_cname( dns_packet* pkt, const std::string& cname ) {
        if ( pkt->dsize == 0 ) return;
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
        dns_record_map_basic * _bmap = (dns_record_map_basic *)_pbuf;
        _bmap->name = _offset;
        _bmap->qtype = htons((uint16_t)dns_qtype_cname);
        _bmap->qclass = htons((uint16_t)dns_qclass_in);
        _bmap->ttl = htonl(30 * 60);    // 30 mins
        _bmap->rlen = htons(cname.size() + 2);
        // _bmap->rdata = (uint32_t)ip;
        char *_rdata = (char *)(_bmap + 1);
        __set_domain(_rdata, cname);
        pkt->length += (sizeof(dns_record_map_basic) + cname.size() + 2);

    }
    dns_cname_record cname( dns_packet* pkt ) {
        // Try to check the domain size
        if ( pkt->dsize == 0 ) {
            string _qd = domain( pkt );
        }
        dns_cname_record _cr;
        const char *_pbuf = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize + 2 + 2;
        uint16_t _c = (pkt->header->ancount + pkt->header->nscount);
        for ( uint16_t i = 0; i < _c; ++i ) {
            // Skip the domain part
            size_t _ds = __count_domain_part(_pbuf);
            _pbuf += _ds;

            // Map the basic part
            dns_map_basic * _mb = (dns_map_basic *)_pbuf;
            bool _is_c = ((dns_qtype)ntohs(_mb->qtype) == dns_qtype_cname);
            _pbuf += sizeof(dns_map_basic);

            // If this is a CName record
            if ( _is_c ) {
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
    void set_domain( dns_packet* pkt, const std::string& domain ) {
        pkt->header->qrcode = 0;    // this is a request
        pkt->header->is_recursive_desired = 1;
        pkt->header->qdcount = 1;   // query one domain

        // Copy the domain
        __set_domain( pkt_begin(pkt) + pkt->length, domain );
        pkt->length += (domain.size() + 2);
        pkt->dsize = (uint16_t)(domain.size() + 2);
    }
    std::string domain( dns_packet* pkt ) {
        std::string _d;
        if ( pkt->length > (sizeof(dns_packet_header) + 2 + 2) ) {
            __get_domain_basic(pkt_begin(pkt) + sizeof(dns_packet_header), _d);
        }
        pkt->dsize = (uint16_t)(_d.size() + 2);
        return _d;
    }
    // Get qtype
    dns_qtype qtype( dns_packet* pkt ) {
        char * _e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
        uint16_t * _f = (uint16_t *)_e;
        return (dns_qtype)ntohs(_f[0]);
    }
    void set_qtype( dns_packet* pkt, dns_qtype qtype ) {
        char * _e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
        uint16_t * _f = (uint16_t *)_e;
        pkt->length += sizeof(uint16_t);
        _f[0] = htons(qtype);
    }
    // Get qclass
    dns_qclass qclass( dns_packet* pkt ) {
        char * _e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
        uint16_t * _f = (uint16_t *)_e;
        return (dns_qclass)ntohs(_f[1]);
    }
    void set_qclass( dns_packet* pkt, dns_qclass qclass ) {
        char * _e = pkt_begin(pkt) + sizeof(dns_packet_header) + pkt->dsize;
        uint16_t * _f = (uint16_t *)_e;
        pkt->length += sizeof(uint16_t);
        _f[1] = htons(qclass);
    }

}}}}}

namespace pe { namespace co { namespace net {

	typedef std::pair< peer_t, peer_t >                               DNSValueType;
    typedef utils::filter_node< char, DNSValueType >                  DNSFilter;
    typedef DNSFilter::shared_node_t                                  pDNSFilter;

    // DNS Result Cache
    typedef proto::dns::dns_a_record        a_record_t;
    typedef proto::dns::dns_cname_record    c_record_t;

    // internal dns packet wrapper
    class __dns_pkt_guard {
        proto::dns::dns_packet          pkt;
    public:
        __dns_pkt_guard( ) { proto::dns::init_dns_packet(&pkt); }
        ~__dns_pkt_guard( ) { proto::dns::release_dns_packet(&pkt); }
        proto::dns::dns_packet * get() { return &pkt; }
    };

	class __dns_cache_manager {
	protected:
		// Filters
		pDNSFilter                                                    flt_begin;
		pDNSFilter                                                    flt_end;
		pDNSFilter                                                    flt_contain;
		std::unordered_map< std::string, DNSValueType >*              flt_equal;

		// Caches
		std::unordered_map< std::string, std::list< a_record_t > >*   dns_a_cache;
		std::unordered_map< std::string, c_record_t >*                dns_c_cache;

		// Resolv
		std::vector< peer_t >*                                        resolv_list;
        time_t                                                        host_up_time;

        // TransId
        uint16_t                                                      saved_transid;

    protected:

        bool _parse_response( proto::dns::dns_packet* pkt, const std::string& domain ) {
            // parse the response and update the cache
            proto::dns::prepare_for_reading( pkt );
            auto _ips = proto::dns::ips( pkt );
            bool _has_any_result = false;
            if ( _ips.size() > 0 ) {
                dns_a_cache->emplace( std::make_pair( domain, std::move(_ips) ) );
                _has_any_result = true;
            }
            auto _cname = proto::dns::cname( pkt );
            if ( _cname.domain.size() > 0 ) {
                dns_c_cache->emplace( std::make_pair( domain, std::move(_cname) ) );
                _has_any_result = true;
            }
            return _has_any_result;
        }

        ip_t _do_query( const std::string& domain ) {
            // Create a dns query packet
            __dns_pkt_guard _pg;
            proto::dns::dns_packet* _pkt = _pg.get();
            _pkt->header->trans_id = ++saved_transid;
            proto::dns::set_domain( _pkt, domain );
            proto::dns::set_qtype( _pkt );
            proto::dns::set_qclass( _pkt );
            proto::dns::prepare_for_sending( _pkt );

            auto _sinfo = match_any_query(domain);
            if ( !_sinfo.first ) {
                // Not match anything
                return _do_query_according_to_resolv( _pkt, domain );
            }

            if ( _sinfo.second ) {
                // Has a socks5
                return _do_query_by_tcp_server(_pkt, domain, _sinfo.first, _sinfo.second);
            }
            // Normal UDP
            return _do_query_by_udp_server(_pkt, domain, _sinfo.first);
        }

        ip_t _do_query_according_to_resolv( proto::dns::dns_packet* pkt, const std::string& domain ) {
            if ( resolv_list->size() == 0 ) return ip_t("255.255.255.255");
            for ( size_t i = 0; i < resolv_list->size(); ++i ) {
                ip_t _r(_do_query_by_udp_server(pkt, domain, (*resolv_list)[i]));
                if ( _r.is_valid() ) return _r;
            }
            return ip_t("255.255.255.255");
        }

        ip_t _do_query_by_tcp_server(
            proto::dns::dns_packet* pkt, 
            const std::string& domain,
            const peer_t& server,
            const peer_t& socks5 = peer_t::nan
        ) {
            netadapter *_qadapter = NULL;
            if ( socks5 ) {
                _qadapter = new socks5adapter(socks5);
            } else {
                _qadapter = new tcpadapter;
            }
            std::shared_ptr< netadapter > _padapter(_qadapter);
            if ( ! _padapter->connect(server.str()) ) return ip_t((uint32_t)-1);
            if ( !_padapter->write(std::string(pkt->packet, pkt->length + sizeof(uint16_t))) )
                return ip_t((uint32_t)-1);
            auto _r = _padapter->read(std::chrono::seconds(3));
            if ( _r.first != op_done ) return ip_t((uint32_t)-1);

            proto::dns::dns_packet _ipkt;
            _ipkt.packet = &_r.second[0];
            _ipkt.length = _r.second.size() - sizeof(uint16_t);
            _ipkt.buflen = 0;
            _ipkt.header = (proto::dns::dns_packet_header *)(_ipkt.packet + sizeof(uint16_t));
            _ipkt.dsize = 0;

            if ( !_parse_response(&_ipkt, domain) ) return ip_t((uint32_t)-1);
            return make_query(domain);
        }

        ip_t _do_query_by_udp_server(
            proto::dns::dns_packet* pkt,
            const std::string& domain,
            const peer_t& server
        ) {
            udpadapter _uadapter;
            _uadapter.connect(server.str());
            if ( !_uadapter.write(std::string(pkt_begin(pkt), pkt->length)) )
                return ip_t((uint32_t)-1);

            auto _r = _uadapter.read(std::chrono::seconds(3));
            if ( _r.first != op_done ) return ip_t((uint32_t)-1);

            proto::dns::dns_packet _ipkt;
            _ipkt.packet = &_r.second[0];
            _ipkt.length = _r.second.size();
            _ipkt.buflen = 0;
            _ipkt.header = (proto::dns::dns_packet_header *)(&_r.second[0]);
            _ipkt.dsize = 0;
            if ( _ipkt.header->is_truncation ) {
                return _do_query_by_tcp_server(pkt, domain, server);
            }
            if ( !_parse_response(&_ipkt, domain) ) return ip_t((uint32_t)-1);
            return make_query(domain);
        }

    public: 

        __dns_cache_manager() : 
            flt_equal(new std::unordered_map< std::string, DNSValueType >),
            dns_a_cache(new std::unordered_map< std::string, std::list< a_record_t > >),
            dns_c_cache(new std::unordered_map< std::string, c_record_t >),
            resolv_list(new std::vector< peer_t >),
            host_up_time(0),
            saved_transid(0)
        { 
            flt_begin = DNSFilter::create_root_node();
            flt_end = DNSFilter::create_root_node();
            flt_contain = DNSFilter::create_root_node();

            // Begin Host file check loop
            this_loop.do_loop([this]() {
                // If i am the last task, quit
                if ( this_loop.task_count() == 1 ) {
                    this_task::cancel_loop();
                    return;
                }

                time_t _lu = utils::file_update_time("/etc/hosts");
                if ( this->host_up_time == _lu ) return;
                this->host_up_time = _lu;

                // Remove all old record
                auto i = std::begin(*this->dns_a_cache);
                while ( i != std::end(*this->dns_a_cache) ) {
                    if ( i->second.begin()->ttl == 0 )  {
                        i = this->dns_a_cache->erase(i);
                    } else {
                        ++i;
                    }
                }
                auto j = std::begin(*this->dns_c_cache);
                while ( j != std::end(*this->dns_c_cache) ) {
                    if ( j->second.ttl == 0 ) {
                        j = this->dns_c_cache->erase(j);
                    } else {
                        ++j;
                    }
                }
                
                // Read hosts file and update the cache
                std::ifstream _hf("/etc/hosts");
                // Read line
                std::string _line;
                while ( std::getline(_hf, _line) ) {
                    utils::trim(_line);
                    if ( _line.size() == 0 ) continue;  // empty line
                    if ( _line[0] == '#' ) continue;    // comment
                    if ( _line[0] == ':' ) continue;    // ipv6, not now
                    auto _p = utils::split(_line, " \t");
                    if ( _p.size() == 1 ) continue;
                    ip_t _ip(_p[0]);
                    if ( _ip.is_valid() ) {
                        for ( size_t i = 1; i < _p.size(); ++i ) {
                            std::list< a_record_t > _l;
                            _l.emplace_back((a_record_t) {
                                _ip,
                                proto::dns::dns_qclass_in,
                                0
                            });
                            (*this->dns_a_cache)[_p[i]] = _l;
                        }
                    } else {
                        for ( size_t i = 1; i < _p.size(); ++i ) {
                            (*this->dns_c_cache)[_p[i]] = (c_record_t){
                                _p[0],
                                proto::dns::dns_qclass_in,
                                0
                            };
                        }
                    }
                }
            }, std::chrono::seconds(1));

            res_init();
            for ( int i = 0; i < _res.nscount; ++i ) {
                peer_t _pi(
                    ip_t(_res.nsaddr_list[i].sin_addr.s_addr), 
                    ntohs(_res.nsaddr_list[i].sin_port)
                    );
                resolv_list->emplace_back(_pi);
            }
        }
        ~__dns_cache_manager() {
            delete flt_equal;
            delete dns_a_cache;
            delete dns_c_cache;
            delete resolv_list;
        }

        // Sigleton
        static __dns_cache_manager& m() {
            static __dns_cache_manager g_m;
            return g_m;
        }

        // Add Host => DNS Server Relation
        void set_query_server(const std::string& host, const peer_t& server, const peer_t& socks5) {
            auto _ss = std::make_pair( server, socks5 );
            bool _is_begin = (*host.rbegin() == '*');
            bool _is_end = (host[0] == '*');
            bool _is_contain = (_is_begin && _is_end);
            bool _is_equal = (!_is_begin) && (!_is_end);

            if ( _is_equal ) {
                ON_DEBUG_CONET(
                    std::cout << host << " is equal filter" << std::endl;
                )
                (*flt_equal)[host] = _ss;
            } else if ( _is_contain ) {
                std::string _chost = host.substr(1, host.size() - 2);
                ON_DEBUG_CONET(
                    std::cout << _chost << " is contain filter" << std::endl;
                )
                DNSFilter::create_filter_tree(flt_contain, _chost.begin(), _chost.end(), _ss);
            } else if ( _is_begin ) {
                std::string _bhost = host.substr(0, host.size() - 1);
                ON_DEBUG_CONET(
                    std::cout << _bhost << " is begin filter" << std::endl;
                )
                DNSFilter::create_filter_tree(flt_begin, _bhost.begin(), _bhost.end(), _ss);
            } else {
                std::string _ehost = host.substr(1);
                ON_DEBUG_CONET(
                    std::cout << _ehost << " is end filter" << std::endl;
                )
                DNSFilter::create_filter_tree(flt_end, _ehost.rbegin(), _ehost.rend(), _ss);
            }
        }

        void clear_query_server( const std::string& host ) {
            bool _is_begin = (*host.rbegin() == '*');
            bool _is_end = (host[0] == '*');
            bool _is_contain = (_is_begin && _is_end);
            bool _is_equal = (!_is_begin) && (!_is_end);

            if ( _is_equal ) {
                flt_equal->erase(host);
            } else if ( _is_contain ) {
                std::string _chost = host.substr(1, host.size() - 2);
                DNSFilter::remove_filter_tree(flt_contain, _chost.begin(), _chost.end());
            } else if ( _is_begin ) {
                std::string _bhost = host.substr(0, host.size() - 1);
                DNSFilter::remove_filter_tree(flt_begin, _bhost.begin(), _bhost.end());
            } else {
                std::string _ehost = host.substr(1);
                DNSFilter::remove_filter_tree(flt_end, _ehost.rbegin(), _ehost.rend());
            }
        }

        DNSValueType match_any_query( const std::string& host ) {
            peer_t _dns_server = peer_t::nan;
            peer_t _socks5 = peer_t::nan;
            do {
                auto _eit = flt_equal->find( host );
                if ( _eit != flt_equal->end() ) {
                    _dns_server = _eit->second.first;
                    _socks5 = _eit->second.first;
                    break;
                }
                pDNSFilter _pf = DNSFilter::is_match(flt_end, host.rbegin(), host.rend());
                if ( _pf != nullptr ) {
                    ON_DEBUG_CONET(
                        std::cout << "flt_end match: " << host << std::endl;
                    )
                    auto & _v = _pf->value_obj();
                    _dns_server = _v.first;
                    _socks5 = _v.second;
                    break;
                }
                _pf = DNSFilter::is_match(flt_begin, host.begin(), host.end());
                if ( _pf != nullptr ) {
                    auto& _v = _pf->value_obj();
                    _dns_server = _v.first;
                    _socks5 = _v.second;
                    break;
                }
                for ( auto i = host.begin(); i != host.end(); ++i ) {
                    _pf = DNSFilter::is_match(flt_contain, i, host.end());
                    if ( _pf != nullptr ) {
                        auto& _v = _pf->value_obj();
                        _dns_server = _v.first;
                        _socks5 = _v.second;
                        break;
                    }
                }
            } while ( false );

            return std::make_pair(_dns_server, _socks5);
        }

        ip_t make_query( const std::string& host ) {
            auto _rit = dns_a_cache->find(host);
            if ( _rit != dns_a_cache->end() ) {
                time_t _ttl = _rit->second.begin()->ttl;
                if ( _ttl != 0 && _ttl < time(NULL) ) {
                    dns_a_cache->erase(_rit);
                } else {
                    ip_t _ip = _rit->second.begin()->ip;
                    if ( _rit->second.size() > 1 ) {
                        // Re-order if has more than 1 result
                        _rit->second.emplace_back(std::move(*_rit->second.begin()));
                        _rit->second.pop_front();
                    }
                    // Return from Cache
                    return _ip;
                }
            }
            auto _cit = dns_c_cache->find(host);
            if ( _cit != dns_c_cache->end() ) {  // check cname cache
                time_t _ttl = _cit->second.ttl;
                if ( _ttl != 0 && _ttl < time(NULL) ) {
                    dns_c_cache->erase(_cit);    // timedout
                } else {
                    // Query the cname domain
                    return make_query(_cit->second.domain);
                }
            }
            return _do_query( host );
        }
	};

    #define DNS_MGR         __dns_cache_manager::m()
    // Set host => dns server relation
    void set_query_server( 
        const std::string& host, 
        const peer_t& server, 
        const peer_t& socks5
    ) {
        DNS_MGR.set_query_server(host, server, socks5);
    }
    void clear_query_server( const std::string& host ) {
        DNS_MGR.clear_query_server(host);
    }
    // Tell if the host match any set query server
    std::pair< peer_t, peer_t > match_query_server( const std::string& host ) {
        return DNS_MGR.match_any_query(host);
    }

    // Query Domain
    ip_t get_hostname( const std::string& host ) {
        ip_t _r(host);
        if ( _r ) return _r;    // The host is just an IP
        return DNS_MGR.make_query( host );
    }
    // Force to stop hosts update
    void stop_hosts_update() {
        // pe::co::task_exit(g_host_update_task);
    }
}}}

// Push Chen
//

