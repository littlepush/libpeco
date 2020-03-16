/*
    http.h
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#pragma once

#ifndef PE_CO_NET_HTTP_H__
#define PE_CO_NET_HTTP_H__

#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/utils/obj.h>
#include <peco/peutils.h>

namespace pe { namespace co { namespace net { namespace proto { namespace http {
    namespace content_type {
        // Get the content type according to the extension of given filename
        const std::string& by_extension( const std::string& filename );
    }
    
    enum v_t {
        v1_0    = 0x01,
        v1_1    = 0x02,
        v2_0    = 0x03
    };
    // HTTP Code
    enum CODE {
        CODE_000            = 000,      // Internal Use: Fully charged by preprocessor
        CODE_001            = 001,      // Unknow
        CODE_100            = 100,      // Continue
        CODE_101            = 101,      // Switching Protocols
        CODE_200            = 200,      // OK
        CODE_201            = 201,      // Created
        CODE_202            = 202,      // Accepted
        CODE_203            = 203,      // Non-Authoritative Information
        CODE_204            = 204,      // No Content
        CODE_205            = 205,      // Reset Content
        CODE_206            = 206,      // Partial Content
        CODE_300            = 300,      // Multiple Choices
        CODE_301            = 301,      // Moved Permanently
        CODE_302            = 302,      // Found
        CODE_303            = 303,      // See Other
        CODE_304            = 304,      // Not Modified
        CODE_305            = 305,      // Use Proxy
        CODE_307            = 307,      // Temporary Redirect
        CODE_400            = 400,      // Bad Request
        CODE_401            = 401,      // Unauthorized
        CODE_402            = 402,      // Payment Required
        CODE_403            = 403,      // Forbidden
        CODE_404            = 404,      // Not Found
        CODE_405            = 405,      // Method Not Allowed
        CODE_406            = 406,      // Not Acceptable
        CODE_407            = 407,      // Proxy Authentication Required
        CODE_408            = 408,      // Request Time-out
        CODE_409            = 409,      // Conflict
        CODE_410            = 410,      // Gone
        CODE_411            = 411,      // Length Required
        CODE_412            = 412,      // Precondition Failed
        CODE_413            = 413,      // Request Entity Too Large
        CODE_414            = 414,      // Request-URI Too Large
        CODE_415            = 415,      // Unsupported Media Type
        CODE_416            = 416,      // Requested range not satisfiable
        CODE_417            = 417,      // Expectation Failed
        CODE_500            = 500,      // Internal Server Error
        CODE_501            = 501,      // Not Implemented
        CODE_502            = 502,      // Bad Gateway
        CODE_503            = 503,      // Service Unavailable
        CODE_504            = 504,      // Gateway Time-out
        CODE_505            = 505       // HTTP Version not supported
    };

    // Get the message of a code
    std::string code_message( CODE c );

    // Httpversion_t 
    struct version_t {
        //version_t 
        v_t         vn;

        // Convert the version_t string to the flag
        version_t();
        version_t( v_t v );
        version_t( const version_t& v );
        version_t( version_t&& v );
        version_t( const std::string& vs );

        version_t& operator = ( v_t v );
        version_t& operator = ( const version_t& v );
        version_t& operator = ( version_t&& v );
        version_t& operator = ( const std::string& vs );

        bool operator == ( v_t v ) const;
        bool operator == ( const version_t& v );

        // Convert the version_t flag to string
        operator std::string() const;

        // Copy to buffer
        std::string& operator >> (std::string& buffer) const;
    };

    // Parameters
    namespace url_params {
        typedef std::map< std::string, std::string >    param_t;

        // Check if contains a Key
        bool contains( const param_t& p, const std::string& k );

        // Parse from string buffer
        param_t parse( const char* b, uint32_t l );

        // Format to buffer
        std::string& operator >> ( const param_t& p, std::string& buffer );
    };

    struct header_t {
        typedef std::map< std::string, std::string >    hd_data_t;
        typedef hd_data_t::iterator                     iterator;
        typedef hd_data_t::const_iterator               const_iterator;
        typedef hd_data_t::reverse_iterator             reverse_iterator;
        typedef hd_data_t::const_reverse_iterator       const_reverse_iterator;

        hd_data_t       hd_;

        header_t( );
        template < typename lineIterator >
        header_t( lineIterator begin, lineIterator end ) {
            for ( lineIterator i = begin; i != end; ++i ) {
                size_t _ = i->find(':');
                std::string _k = (*i).substr(0, _);
                std::string _v = (*i).substr(_ + 1);
                (*this)[_k] = pe::utils::trim(_v);
            }
        }
        header_t( const header_t& r );
        header_t( header_t&& r );
        header_t& operator = ( const header_t& h );
        header_t& operator = ( header_t&& h );
        
        // If the header contains key
        bool contains( const std::string& k );

        std::string& operator [] ( const std::string& k );
        std::string& operator [] ( std::string&& k );

        // Get header item size
        size_t size() const;

        // erases elements
        void erase( iterator pos );
        iterator erase( const_iterator pos );
        iterator erase( const_iterator first, const_iterator last );
        void erase( const std::string& k );

        // Iterators
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        reverse_iterator rbegin() noexcept;
        const_reverse_iterator rbegin() const noexcept;
        const_reverse_iterator crbegin() const noexcept;

        reverse_iterator rend() noexcept;
        const_reverse_iterator rend() const noexcept;
        const_reverse_iterator crend() const noexcept;

        // Format to buffer
        std::string& operator >> ( std::string& buffer ) const;
    };

    typedef enum {
        SameSite_Strict,
        SameSite_Lax,
        SameSite_None
    } cookieSameSiteOption;

    // Cookie Definition
    struct cookie_t {
        bool                    secure_only;
        bool                    http_only;
        time_t                  expires;
        uint32_t                max_age;
        std::string             domain;
        std::string             path;
        cookieSameSiteOption    same_site;
        std::string             key;
        std::string             value;

        // Init
        cookie_t();
        cookie_t( const cookie_t& rhs );
        cookie_t( cookie_t&& rhs );
        cookie_t & operator = ( const cookie_t& rhs );
        cookie_t & operator = ( cookie_t&& rhs );

        // Format to string
        std::string& operator >> ( std::string& buffer ) const;
    };

    // Create Cookie
    cookie_t cookie_create(const std::string& key, const std::string& value);
    // Create Secure Only Cookie
    cookie_t cookie_create_secure(const std::string& key, const std::string& value);
    // Create Http Only Cookie
    cookie_t cookie_create_ho(const std::string& key, const std::string& value);

    // Output
    ostream& operator << ( ostream& os, const header_t& h );
  
    struct body_t {

        typedef struct {
            int             f;
            const char      *data;
            size_t          len;
            std::string     ct;
        } body_part_t;

        typedef std::list< body_part_t >            body_container_t;
        typedef body_container_t::iterator          iterator;
        typedef body_container_t::const_iterator    const_iterator;

        bool                is_chunked;
        bool                is_gzipped;

        // If the body is chuncked, the container will contain all the parts
        // otherwise, the container will only contains one body buffer.
        body_container_t    container;

        // Create a normal body object
        body_t( bool chunked = false, bool gzipped = false );
        body_t( const body_t& b );
        body_t( body_t&& b );

        body_t& operator = ( const body_t& b );
        body_t& operator = ( body_t&& b );

        void append( const std::string& data );
        void append( const char* data, uint32_t length );
        void append( std::string&& data );
        bool load_file( const std::string& path );

        // Tell the body's size, if is chunked, will return 0
        size_t calculate_size();
        size_t size();

        // If the flag is already gzipped, do nothing
        // else, gzipped the data, and set the flag
        void gzip();
        // unset the flag and unpack the body data
        void gunzip();

        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        // Get the chunk size
        std::string chunk_size( body_t::iterator cit );

        // Get all body data
        std::string raw() const;
    };

}}}}}

namespace pe { namespace co { namespace net {
    using namespace pe::co::net::proto::http;

    class http_cookie {
    public:
        typedef std::map< std::string, cookie_t >   cookie_container_t;
        typedef cookie_container_t::iterator        iterator;
        typedef cookie_container_t::const_iterator  const_iterator;

    protected:
        cookie_container_t              cookies_;
    public:
        http_cookie();
        http_cookie(const http_cookie& rhs);
        http_cookie(http_cookie&& rhs);
        http_cookie& operator = (const http_cookie& rhs);
        http_cookie& operator = (http_cookie&& rhs);

        void set_cookie(const std::string& key, const std::string& value);
        void set_cookie(const cookie_t& c);
        void set_cookie(cookie_t&& c);

        // If has cookie
        bool contains(const std::string& key) const;

        std::string& get_cookie_value(const std::string& key);
        cookie_t& get_cookie(const std::string& key);

        // Iterator
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;


        void dump_as_request(std::string& buffer);
        void dump_as_response(std::string& buffer);
    };

    class http_request {
        bool                                ssl_scheme_;
        std::string                         method_;
        version_t                           version_;
        std::string                         path_;
        url_params::param_t                 params_;
        header_t                            header_;
        body_t                              body_;
        http_cookie                         cookie_;

    public: 
        http_request();
        http_request( const http_request & r );
        http_request( http_request&& r );

        http_request& operator = ( const http_request& r );
        http_request& operator = ( http_request&& r );

        // Get and set the HTTP Method
        std::string& method() noexcept;
        const std::string& method() const noexcept;

        // Get and set the path of the url
        std::string& path() noexcept;
        const std::string& path() const noexcept;

        // Get the last path component
        std::string last_component() noexcept;

        // Get all components of path, each component is start with '/'
        std::vector< std::string > path_components() const noexcept;

        // Get or set the host in header
        std::string& host() noexcept;

        bool contains_param( const std::string& key );

        // Header and body reference
        version_t &             version;
        header_t &              header;
        body_t &                body;
        url_params::param_t &   params;
        http_cookie&            cookie;

        // If is using SSL
        bool is_security();
        void set_secutiry( bool is_ssl = true );

    public: 
        static http_request fromURL( const std::string& url );
    };

    class http_response {
        version_t                           version_;
        CODE                                status_code_;
        std::string                         message_;
        header_t                            header_;
        body_t                              body_;
        http_cookie                         cookie_;
    public: 
        http_response();
        http_response( const http_response& r );
        http_response( http_response&& r );

        http_response& operator = ( const http_response& r );
        http_response& operator = ( http_response&& r );

        // Load file to the body
        void load_file(const std::string& path);

        // append new data to the response's body
        void write( std::string&& data );
        void write( const std::string& data );
        void write( const char* data );
        void write( const char* data, size_t len );

        // Header and body reference
        version_t &         version;
        header_t &          header;
        body_t &            body;
        CODE &              status_code;
        std::string&        message;
        http_cookie&        cookie;
    };

    // Connection Methods
    namespace http_connection {
        // Send the request to peer server and get the response
        http_response send_request( http_request& req, duration_t timeout = NET_DEFAULT_TIMEOUT );

        // Send the request to some other server(not the resolv of host in req)
        http_response send_request( http_request& req, const peer_t& host, duration_t timeout = NET_DEFAULT_TIMEOUT );
       
        // Use a socks5 to send the request 
        http_response send_request( const peer_t& socks5, http_request& req, duration_t timeout = NET_DEFAULT_TIMEOUT );

        // Use a sock5 to send the request and specified the target address
        http_response send_request( 
            const peer_t& socks5, 
            const http_request& req, 
            const std::string& host, 
            uint16_t port, 
            duration_t timeout = NET_DEFAULT_TIMEOUT 
        );
    }

    // Create a simple server
    namespace http_server {
        // Listen on a socket, the socket can be tcp, ssl,
        // the socket must be bind to the port first
        void listen( std::function< void (http_request&) > cb );

        // Send the response
        void send_response( http_response& resp );
    }
}}}

#endif

// Push Chen
//

