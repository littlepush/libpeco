/*
    http.cpp
    PECoNet
    2019-05-17
    Push Chen
    
    Copyright 2015-2019 MeetU Infomation and Technology Inc. All rights reserved.
 */
 
#include <peco/conet/protos/http.h>
#include <peco/conet/conns/tcp.h>
#include <peco/conet/protos/socks5.h>
#include <peco/conet/conns/netadapter.h>
#include <iomanip>

namespace pe { namespace co { namespace net { namespace proto { namespace http {
    namespace content_type {
        // Get the content type according to the extension of given filename
        const std::string& by_extension( const std::string& filename ) {
            static std::map<string, string> g_mime = {
                {"aac", "audio/aac"},
                {"abw", "application/x-abiword"},
                {"arc", "application/x-freearc"},
                {"avi", "video/x-msvideo"},
                {"azw", "application/vnd.amazon.ebook"},
                {"bin", "application/octet-stream"},
                {"bmp", "image/bmp"},
                {"bz", "application/x-bzip"},
                {"bz2", "application/x-bzip2"},
                {"csh", "application/x-csh"},
                {"css", "text/css"},
                {"csv", "text/csv"},
                {"doc", "application/msword"},
                {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
                {"eot", "application/vnd.ms-fontobject"},
                {"epub", "application/epub+zip"},
                {"gif", "image/gif"},
                {"htm", "text/html"},
                {"html", "text/html"},
                {"ico", "image/vnd.microsoft.icon"},
                {"ics", "text/calendar"},
                {"jar", "application/java-archive"},
                {"jpeg", "image/jpeg"},
                {"jpg", "image/jpeg"},
                {"js", "text/javascript"},
                {"json", "application/json"},
                {"mid", "audio/midi"},
                {"midi", "audio/x-midi"},
                {"mjs", "application/javascript"},
                {"mp3", "audio/mpeg"},
                {"mpeg", "video/mpeg"},
                {"mpkg", "application/vnd.apple.installer+xml"},
                {"odp", "application/vnd.oasis.opendocument.presentation"},
                {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
                {"odt", "application/vnd.oasis.opendocument.text"},
                {"oga", "audio/ogg"},
                {"ogv", "video/ogg"},
                {"ogx", "application/ogg"},
                {"otf", "font/otf"},
                {"png", "image/png"},
                {"pdf", "application/pdf"},
                {"ppt", "application/vnd.ms-powerpoint"},
                {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
                {"rar", "application/x-rar-compressed"},
                {"rtf", "application/rtf"},
                {"sh", "application/x-sh"},
                {"svg", "image/svg+xml"},
                {"swf", "application/x-shockwave-flash"},
                {"tar", "application/x-tar"},
                {"tif", "image/tiff"},
                {"tiff", "image/tiff"},
                {"ttf", "font/ttf"},
                {"txt", "text/plain"},
                {"vsd", "application/vnd.visio"},
                {"wav", "audio/wav"},
                {"weba", "audio/webm"},
                {"webm", "video/webm"},
                {"webp", "image/webp"},
                {"woff", "font/woff"},
                {"woff2", "font/woff2"},
                {"xhtml", "application/xhtml+xml"},
                {"xls", "application/vnd.ms-excel"},
                {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
                {"xml", "application/xml"},
                {"xul", "application/vnd.mozilla.xul+xml"},
                {"zip", "application/zip"},
                {"3gp", "video/3gpp"},
                {"3g2", "video/3gpp2"},
                {"7z", "application/x-7z-compressed"}
            };
            std::string _ext = pe::utils::string_tolower(
                *(pe::utils::split(filename, ".").rbegin())
            );
            auto _i = g_mime.find(_ext);
            if ( _i != g_mime.end() ) return _i->second;
            return g_mime["txt"];
        }
    }    
    // Utility Function From Hex to Decimal
    int __hex2decimal( const char* begin, size_t length ) {
        int __b = 1;
        int __r = 0;
        for ( int i = length - 1; i >= 0; --i ) {
            if ( begin[i] >= '0' && begin[i] <= '9' ) {
                __r += ( begin[i] - '0') * __b;
            } else if ( begin[i] >= 'A' && begin[i] <= 'F' ) {
                __r += ( begin[i] - 'A' + 10) * __b;
            } else if ( begin[i] >= 'a' && begin[i] <= 'f' ) {
                __r += ( begin[i] - 'a' + 10) * __b;
            }
            __b *= 16;
        }
        return __r;
    }
    string __decimal2hex( int d ) {
        string _r;
        do {
            int _i = d % 16;
            if ( _i < 10 ) _r += (_i + '0');
            else _r += (_i - 10 + 'A');
            d /= 16;
        } while ( d != 0 );
        std::reverse(_r.begin(), _r.end());
        return _r;
    }

    // BEGIN OF VERSION_T
    //
    version_t::version_t() : vn( v1_1 ) { }
    version_t::version_t( v_t v ) : vn(v) { }
    version_t::version_t( const version_t& v ) : vn(v.vn) { }
    version_t::version_t( version_t&& v ) : vn(v.vn) { }
    version_t::version_t( const std::string& vs ) { *this = vs; }

    version_t& version_t::operator = ( v_t v ) { vn = v; return *this; }
    version_t& version_t::operator = ( const version_t& v ) {
        if ( this == &v ) return *this; 
        vn = v.vn; return *this; 
    }
    version_t& version_t::operator = ( version_t&& v ) {
        if ( this == &v ) return *this; 
        vn = v.vn; return *this;
    }
    version_t& version_t::operator = ( const std::string& vs ) {
        if ( vs == "HTTP/1.0" ) vn = v1_0;
        else if ( vs == "HTTP/1.1" ) vn = v1_1;
        else vn = v2_0;
        return *this;
    }

    bool version_t::operator == ( v_t v ) const { return vn == v; }
    bool version_t::operator == ( const version_t& v ) { return vn == v.vn; }

    // Convert the version_t flag to string
    version_t::operator std::string() const {
        static const char * _v1_0 = "HTTP/1.0";
        static const char * _v1_1 = "HTTP/1.1";
        static const char * _v2_0 = "HTTP/2.0";
        return std::string( vn == v1_0 ? _v1_0 : (vn == v1_1 ? _v1_1 : _v2_0) );
    }

    std::string& version_t::operator >> (std::string& buffer) const {
        buffer += (std::string)*this; return buffer;
    }
    //
    // END OF VERSION_T

    // BEGIN OF PARAMETERS_T
    namespace url_params {
        // Check if contains a Key
        bool contains( const param_t& p, const std::string& k ) {
            return p.find( k ) != p.end();
        }

        // Parse from string buffer
        param_t parse( const char* b, uint32_t l ) {
            // Parse the params
            param_t _p;
            if ( b == NULL || l == 0 ) return _p;   // Empty input
            if ( b[0] == '?' ) ++b, --l;
            uint32_t _l = 0;
            while ( !std::isspace(b[_l]) && _l < l ) ++_l;
            std::string _fmt(b, _l);
            auto _kvs = pe::utils::split( _fmt, "&" );
            for ( const auto & _kvi : _kvs ) {
                auto _kv = pe::utils::split(_kvi, "=");
                if( _kv.size() == 1 ) _kv.push_back(std::string(""));
                _p[_kv[0]] = utils::url_decode(_kv[1]);
            }
            return _p;
        }

        // Format to buffer
        std::string& operator >> ( const param_t& p, std::string& buffer ) {
            if ( p.size() == 0 ) return buffer;
            std::list< std::string > _kvs;
            for ( const auto& _kv : p ) {
                _kvs.emplace_back( _kv.first + "=" + utils::url_encode(_kv.second) );
            }
            buffer += "?";
            buffer += pe::utils::join(_kvs.begin(), _kvs.end(), "&");
            return buffer;
        }
    }

    // BEGIN OF HEADER
    // 
    header_t::header_t( ) { }
    header_t::header_t( const header_t& h ) : hd_(h.hd_) { }
    header_t::header_t( header_t&& h ) : hd_(std::move(h.hd_)) { }
    header_t& header_t::operator = ( const header_t& h ) {
        if ( this == &h ) return *this; 
        hd_ = h.hd_; return *this;
    }
    header_t& header_t::operator = ( header_t&& h ) {
        if ( this == &h ) return *this; 
        hd_ = std::move(h.hd_); return *this;
    }
    // If the header contains key
    bool header_t::contains( const std::string& k ) {
        std::string _k(k);
        return hd_.find( pe::utils::string_capitalize(_k, "-_") ) != hd_.end();
    }

    std::string& header_t::operator [] ( const std::string& k ) {
        std::string _k(k);
        return hd_[pe::utils::string_capitalize(_k, "-_")];
    }
    std::string& header_t::operator [] ( std::string&& k ) {
        std::string _k(std::move(k));
        pe::utils::string_capitalize(_k, "-_");
        return hd_[std::move(_k)];
    }

    // Get header item size
    size_t header_t::size() const { return hd_.size(); }

    // erases elements
    void header_t::erase( header_t::iterator pos ) { hd_.erase(pos); }
    header_t::iterator header_t::erase( header_t::const_iterator pos ) { return hd_.erase(pos); }
    header_t::iterator header_t::erase( 
        header_t::const_iterator first, header_t::const_iterator last ) { 
        return hd_.erase(first, last); 
    }
    void header_t::erase( const std::string& k ) {
        std::string _k(k);
        hd_.erase(pe::utils::string_capitalize(_k, "-_"));
    }

    // Iterators
    header_t::iterator header_t::begin() noexcept { return hd_.begin(); }
    header_t::const_iterator header_t::begin() const noexcept { return hd_.begin(); }
    header_t::const_iterator header_t::cbegin() const noexcept { return hd_.cbegin(); }

    header_t::iterator header_t::end() noexcept { return hd_.end(); }
    header_t::const_iterator header_t::end() const noexcept { return hd_.end(); }
    header_t::const_iterator header_t::cend() const noexcept { return hd_.cend(); }

    header_t::reverse_iterator header_t::rbegin() noexcept { return hd_.rbegin(); }
    header_t::const_reverse_iterator header_t::rbegin() const noexcept { return hd_.rbegin(); }
    header_t::const_reverse_iterator header_t::crbegin() const noexcept { return hd_.crbegin(); }

    header_t::reverse_iterator header_t::rend() noexcept { return hd_.rend(); }
    header_t::const_reverse_iterator header_t::rend() const noexcept { return hd_.rend(); }
    header_t::const_reverse_iterator header_t::crend() const noexcept { return hd_.crend(); }

    // Format to buffer
    std::string& header_t::operator >> ( std::string& buffer ) const {
        for ( auto i = hd_.begin(); i != hd_.end(); ++i ) {
            if ( i->second.size() == 0 ) continue;
            buffer += i->first;
            buffer += ": ";
            buffer += i->second;
            buffer += "\r\n";
        }
        return buffer;
    }

    // Output
    ostream& operator << ( ostream& os, const header_t& h ) {
        for ( auto& i : h.hd_ ) {
            os << "> " << i.first << ": " << i.second << std::endl;
        }
        return os;
    }

    //
    // END OF HEADER

    // Cookie Definition
    // Init
    cookie_t::cookie_t() : 
        secure_only(false), http_only(false),
        expires(0), max_age(0), same_site(SameSite_Lax)
    { }
    cookie_t::cookie_t( const cookie_t& rhs ) : 
        secure_only(rhs.secure_only), http_only(rhs.http_only),
        expires(rhs.expires), max_age(rhs.max_age),
        domain(rhs.domain), path(rhs.path),
        same_site(rhs.same_site), key(rhs.key), value(rhs.value)
    { }
    cookie_t::cookie_t( cookie_t&& rhs ) :
        secure_only(rhs.secure_only), http_only(rhs.http_only),
        expires(rhs.expires), max_age(rhs.max_age),
        domain(std::move(rhs.domain)), path(std::move(rhs.path)),
        same_site(rhs.same_site), 
        key(std::move(rhs.key)), value(std::move(rhs.value))
    { }
    cookie_t & cookie_t::operator = ( const cookie_t& rhs ) {
        if ( this == &rhs ) return *this;
        secure_only = rhs.secure_only;
        http_only = rhs.http_only;
        expires = rhs.expires;
        max_age = rhs.max_age;
        domain = rhs.domain;
        path = rhs.path;
        same_site = rhs.same_site;
        key = rhs.key;
        value = rhs.value;
        return *this;
    }
    cookie_t & cookie_t::operator = ( cookie_t&& rhs ) {
        if ( this == &rhs ) return *this;
        secure_only = rhs.secure_only;
        http_only = rhs.http_only;
        expires = rhs.expires;
        max_age = rhs.max_age;
        domain = std::move(rhs.domain);
        path = std::move(rhs.path);
        same_site = rhs.same_site;
        key = std::move(rhs.key);
        value = std::move(rhs.value);
        return *this;
    }

    std::string __format_gmt_time(struct tm* ptm) {
        static std::string _sWeekday[] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
        };
        static std::string _sMonth[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        std::stringstream _ss;
        _ss << _sWeekday[ptm->tm_wday] << ", " << ptm->tm_mday
            << " " << _sMonth[ptm->tm_mon] << " " << (ptm->tm_year + 1900)
            << " " << std::setfill('0') << std::setw(2) << ptm->tm_hour << ":"
            << std::setfill('0') << std::setw(2) << ptm->tm_min << ":"
            << std::setfill('0') << std::setw(2) << ptm->tm_sec << " GMT";
        return _ss.str();
    }

    // Format to string
    std::string& cookie_t::operator >> ( std::string& buffer ) const {
        std::list< std::string > _data;
        _data.push_back(key + "=" + value);
        if ( secure_only ) _data.push_back("Secure");
        if ( http_only ) _data.push_back("HttpOnly");
        if ( expires > 0 ) {
            struct tm* _ltm = gmtime(&expires);
            _data.push_back("Expires=" + __format_gmt_time(_ltm));
        }
        if ( max_age > 0 ) _data.push_back("Max-Age=" + std::to_string(max_age));
        if ( domain.size() > 0 ) _data.push_back("Domain=" + domain);
        if ( path.size() > 0 ) _data.push_back("Path=" + path);
        if ( same_site != SameSite_Lax ) {
            if ( same_site == SameSite_Strict ) {
                _data.push_back("SameSite=Strict");
            } else {
                _data.push_back("SameSite=None");
            }
        }
        buffer += "Set-Cookie: ";
        buffer += utils::join(_data, "; ");
        buffer += "\r\n";
        return buffer;
    }

    // Create Cookie
    cookie_t cookie_create(const std::string& key, const std::string& value) {
        cookie_t _c;
        _c.key = key;
        _c.value = value;
        _c.path = "/";
        return _c;
    }
    // Create Secure Only Cookie
    cookie_t cookie_create_secure(const std::string& key, const std::string& value) {
        cookie_t _c;
        _c.key = key;
        _c.value = value;
        _c.path = "/";
        _c.secure_only = true;
        return _c;
    }
    // Create Http Only Cookie
    cookie_t cookie_create_ho(const std::string& key, const std::string& value) {
        cookie_t _c;
        _c.key = key;
        _c.value = value;
        _c.path = "/";
        _c.http_only = true;
        return _c;
    }

    // Get the code's message
    std::string code_message( CODE c ) {
        static std::map<CODE, const char *> _msg_map = {
            {CODE_000, "Internal Use: Fully charged by preprocessor"},
            {CODE_001, "Unknow Code"},
            {CODE_100, "Continue"},
            {CODE_101, "Switching Protocols"},
            {CODE_200, "OK"},
            {CODE_201, "Created"},
            {CODE_202, "Accepted"},
            {CODE_203, "Non-Authoritative Information"},
            {CODE_204, "No Content"},
            {CODE_205, "Reset Content"},
            {CODE_206, "Partial Content"},
            {CODE_300, "Multiple Choices"},
            {CODE_301, "Moved Permanently"},
            {CODE_302, "Found"},
            {CODE_303, "See Other"},
            {CODE_304, "Not Modified"},
            {CODE_305, "Use Proxy"},
            {CODE_307, "Temporary Redirect"},
            {CODE_400, "Bad Request"},
            {CODE_401, "Unauthorized"},
            {CODE_402, "Payment Required"},
            {CODE_403, "Forbidden"},
            {CODE_404, "Not Found"},
            {CODE_405, "Method Not Allowed"},
            {CODE_406, "Not Acceptable"},
            {CODE_407, "Proxy Authentication Required"},
            {CODE_408, "Request Time-out"},
            {CODE_409, "Conflict"},
            {CODE_410, "Gone"},
            {CODE_411, "Length Required"},
            {CODE_412, "Precondition Failed"},
            {CODE_413, "Request Entity Too Large"},
            {CODE_414, "Request-URI Too Large"},
            {CODE_415, "Unsupported Media Type"},
            {CODE_416, "Requested range not satisfiable"},
            {CODE_417, "Expectation Failed"},
            {CODE_500, "Internal Server Error"},
            {CODE_501, "Not Implemented"},
            {CODE_502, "Bad Gateway"},
            {CODE_503, "Service Unavailable"},
            {CODE_504, "Gateway Time-out"},
            {CODE_505, "HTTP Version not supported"},
        };

        auto _it = _msg_map.find(c);
        if ( _it == _msg_map.end() ) {
            return std::string(_msg_map[CODE_001]);
        }
        return std::string(_it->second);
    }

    // BEGIN OF BODY
    //
    // Create a normal body object
    body_t::body_t( bool chunked, bool gzipped ) : 
        is_chunked(chunked), is_gzipped(gzipped) 
    { }
    body_t::body_t( const body_t& b ) 
        : is_chunked(b.is_chunked), is_gzipped(b.is_gzipped), container(b.container) 
    { }
    body_t::body_t( body_t&& b )
        : is_chunked(b.is_chunked), is_gzipped(b.is_gzipped), container(std::move(b.container)) 
    { }

    body_t& body_t::operator = ( const body_t& b ) {
        if ( this == &b ) return *this;
        is_chunked = b.is_chunked; is_gzipped = b.is_gzipped;
        container = b.container;
        return *this;
    }
    body_t& body_t::operator = ( body_t&& b ) {
        if ( this == &b ) return *this;
        is_chunked = b.is_chunked; is_gzipped = b.is_gzipped;
        container = std::move(b.container);
        return *this;
    }

    void body_t::append( const std::string& data ) {
        if ( container.size() > 0 ) {
            auto _r = container.rbegin();
            if ( _r->ct.size() + data.size() < 1024 ) {
                _r->ct.append(data);
                return;
            }
        }
        container.emplace_back((body_t::body_part_t) {0, data});
    }
    void body_t::append( const char* data, uint32_t length ) {
        this->append(std::string(data, length));
    }
    void body_t::append( std::string&& data ) {
        body_t::body_part_t _bp{
            0, std::move(data)
        };
        container.emplace_back( std::move(_bp) );
    }
    bool body_t::load_file( const std::string& path ) {
        if ( !utils::is_file_existed(path) ) return false;
        body_t::body_part_t _bp{1, path};
        container.emplace_back( std::move(_bp) );
        return true;
    }

    // Tell the body's size, if is chunked, will return 0
    size_t body_t::calculate_size() {
        if ( is_chunked ) return 0u;
        size_t _c = 0;
        for ( auto i = container.begin(); i != container.end(); ++i ) {
            // If the body part is string, add size
            if ( i->f == 0 ) {
                _c += i->ct.size();
            }
            // If it is a file, calculate the file size
            if ( i->f == 1 ) {
                // We do have the file and already loaded it
                // into the memory, so we just need to query the reference
                // and get the size
                const std::string& _fp = utils::file_cache::load_file(i->ct);
                _c += _fp.size();
            }
        }
        return _c;
    }
    size_t body_t::size() { return this->calculate_size(); }

    // If the flag is already gzipped, do nothing
    // else, gzipped the data, and set the flag
    void body_t::gzip() {
        is_gzipped = true;
    }
    // unset the flag and unpack the body data
    void body_t::gunzip() {
        is_gzipped = false;
    }

    body_t::iterator body_t::begin() noexcept { return container.begin(); }
    body_t::const_iterator body_t::begin() const noexcept { return container.begin(); }
    body_t::const_iterator body_t::cbegin() const noexcept { return container.cbegin(); }

    body_t::iterator body_t::end() noexcept { return container.end(); }
    body_t::const_iterator body_t::end() const noexcept { return container.end(); }
    body_t::const_iterator body_t::cend() const noexcept { return container.cend(); }

    // Get the chunk size
    std::string body_t::chunk_size( body_t::iterator cit ) {
        if ( cit->f == 0 ) {
            return __decimal2hex( cit->ct.size() );
        }
        if ( cit->f == 1 ) {
            return __decimal2hex( utils::file_cache::load_file(cit->ct).size() );
        }
        return "0\r\n\r\n";
    }
    // Get all body data
    std::string body_t::raw() const {
        std::string _raw;
        for ( auto _it = this->begin(); _it != this->end(); ++_it ) {
            if ( _it->f == 0 ) {
                _raw += _it->ct;
            } else {
                _raw += utils::file_cache::load_file(_it->ct);
            }
        }
        return _raw;
    }

}}}}}

namespace pe { namespace co { namespace net {
    http_cookie::http_cookie() { }
    http_cookie::http_cookie(const http_cookie& rhs) : 
        cookies_(rhs.cookies_)
    { }
    http_cookie::http_cookie(http_cookie&& rhs) : 
        cookies_(std::move(rhs.cookies_))
    { }
    http_cookie& http_cookie::operator = (const http_cookie& rhs) {
        if ( this == &rhs ) return *this;
        cookies_ = rhs.cookies_;
        return *this;
    }
    http_cookie& http_cookie::operator = (http_cookie&& rhs) {
        if ( this == &rhs ) return *this;
        cookies_ = std::move(rhs.cookies_);
        return *this;
    }

    void http_cookie::set_cookie(const std::string& key, const std::string& value) {
        cookies_[key] = cookie_create(key, value);
    }
    void http_cookie::set_cookie(const cookie_t& c) {
        cookies_[c.key] = c;
    }
    void http_cookie::set_cookie(cookie_t&& c) {
        cookies_.emplace( std::make_pair(c.key, std::move(c)) );
    }

    // If has cookie
    bool http_cookie::contains(const std::string& key) const {
        return cookies_.find(key) != cookies_.end();
    }

    std::string& http_cookie::get_cookie_value(const std::string& key) {
        return cookies_[key].value;
    }
    cookie_t& http_cookie::get_cookie(const std::string& key) {
        return cookies_[key];
    }
    
    http_cookie::iterator http_cookie::begin() noexcept { return cookies_.begin(); }
    http_cookie::const_iterator http_cookie::begin() const noexcept { return cookies_.begin(); }
    http_cookie::const_iterator http_cookie::cbegin() const noexcept { return cookies_.cbegin(); }

    http_cookie::iterator http_cookie::end() noexcept { return cookies_.end(); }
    http_cookie::const_iterator http_cookie::end() const noexcept { return cookies_.end(); }
    http_cookie::const_iterator http_cookie::cend() const noexcept { return cookies_.cend(); }

    void http_cookie::dump_as_request(std::string& buffer) {
        if ( cookies_.size() == 0 ) return;
        buffer += "Cookie: ";
        std::list< std::string > _cookie_list;
        for ( auto& kv : cookies_ ) {
            _cookie_list.emplace_back( kv.first + "=" + kv.second.value );
        }
        buffer += utils::join(_cookie_list, "; ");
        buffer += "\r\n";
    }
    void http_cookie::dump_as_response(std::string& buffer) {
        if ( cookies_.size() == 0 ) return;
        for ( auto& kv : cookies_ ) {
            kv.second >> buffer;
        }
    }

    // public: 
    http_request::http_request() : 
        ssl_scheme_(false), method_("GET"), version_(v1_1), path_("/"),
        version(version_), header(header_), body(body_), params(params_), 
        cookie(cookie_)
    { header_["Host"] = "127.0.0.1"; }
    http_request::http_request( const http_request & r ) : 
        ssl_scheme_(r.ssl_scheme_),
        method_(r.method_),
        version_(r.version_),
        path_(r.path_),
        params_(r.params_),
        header_(r.header_),
        body_(r.body_),
        cookie_(r.cookie_),
        version(version_),
        header(header_),
        body(body_),
        params(params_),
        cookie(cookie_)
    { }
    http_request::http_request( http_request&& r ) : 
        ssl_scheme_(std::move(r.ssl_scheme_)),
        method_(std::move(r.method_)),
        version_(std::move(r.version_)),
        path_(std::move(r.path_)),
        params_(std::move(r.params_)),
        header_(std::move(r.header_)),
        body_(std::move(r.body_)),
        cookie_(std::move(r.cookie_)),
        version(version_),
        header(header_),
        body(body_),
        params(params_),
        cookie(cookie_)
    { }

    http_request& http_request::operator = ( const http_request& r ) {
        if ( this == &r ) return *this;
        ssl_scheme_ = r.ssl_scheme_;
        method_ = r.method_;
        version_ = r.version_;
        path_ = r.path_;
        params_ = r.params_;
        header_ = r.header_;
        body_ = r.body_;
        cookie_ = r.cookie_;
        return *this;
    }
    http_request& http_request::operator = ( http_request&& r ) {
        if ( this == &r ) return *this;
        ssl_scheme_ = std::move(r.ssl_scheme_);
        method_ = std::move(r.method_);
        version_ = std::move(r.version_);
        path_ = std::move(r.path_);
        params_ = std::move(r.params_);
        header_ = std::move(r.header_);
        body_ = std::move(r.body_);
        cookie_ = std::move(r.cookie_);
        return *this;
    }

    // Get and set the HTTP Method
    std::string& http_request::method() noexcept { return this->method_; }
    const std::string& http_request::method() const noexcept { return this->method_; }

    // Get and set the path of the url
    std::string& http_request::path() noexcept { return this->path_; }
    const std::string& http_request::path() const noexcept { return this->path_; }

    // Get the last path component
    std::string http_request::last_component() noexcept {
        size_t _pos = path_.rfind("/");
        if ( _pos == std::string::npos ) _pos = 0;
        return path_.substr(_pos);
    }

    // Get all components of path, each component is start with '/'
    std::vector< std::string > http_request::path_components() const noexcept {
        auto _c = pe::utils::split( path_, "/" );
        for ( auto& c : _c ) {
            c.insert(0, 1, '/');
        }
        _c.insert(_c.begin(), "/");
        return _c;
    }
    // Get or set the host in header
    std::string& http_request::host() noexcept { return header_["Host"]; }

    bool http_request::contains_param( const std::string& key ) { 
        return url_params::contains(this->params_, key);
    }

    // If is using SSL
    bool http_request::is_security() { return ssl_scheme_; }
    void http_request::set_secutiry( bool is_ssl ) { ssl_scheme_ = is_ssl; }

    http_request http_request::fromURL( const std::string& url ) {
        // SCHEME://HOST/PATH?PARAMS
        size_t _scheme_pos = url.find("://");
        bool _ssl_scheme = false;
        if ( _scheme_pos == std::string::npos ) {
            // Default is http
            _scheme_pos = 0;
        } else {
            std::string _scheme = url.substr(0, _scheme_pos);
            _scheme_pos += 3;   // Skip `://`
            pe::utils::string_tolower(_scheme);
            if ( _scheme == "http" ) {
                _ssl_scheme = false;
            } else if ( _scheme == "https" ) {
                _ssl_scheme = true;
            } else {
                throw( std::string("unsupported scheme for http request") );
            }
        }

        http_request _req;
        _req.set_secutiry(_ssl_scheme);

        // Get the host part
        size_t _host_pos = url.find("/", _scheme_pos);
        if ( _host_pos == std::string::npos ) {
            // No path
            _req.host() = url.substr(_scheme_pos);
            return _req;
        } else {
            // HOST/PATH
            _req.host() = url.substr(_scheme_pos, _host_pos - _scheme_pos);
        }

        // Check if has params
        size_t _param_pos = url.find("?", _host_pos);
        if ( _param_pos == std::string::npos ) {
            // No Param
            _req.path() = url.substr(_host_pos);
        } else {
            // PATH?PARAM
            _req.path() = url.substr(_host_pos, _param_pos - _host_pos);
            _req.params_ = url_params::parse( 
                url.c_str() + _param_pos + 1, 
                url.size() - _param_pos
            );
        }
        return _req;
    }

    // HTTP RESPONSE
    http_response::http_response( ) : 
        version_(v1_1), status_code_(CODE_200), message_("OK"),
        version(version_), header(header_), body(body_),
        status_code(status_code_), message(message_), cookie(cookie_)
    { }
    http_response::http_response( const http_response& r ) : 
        version_(r.version_), status_code_(r.status_code_), message_(r.message_), 
        header_(r.header_), body_(r.body_), cookie_(r.cookie_),
        version(version_), header(header_), body(body_), 
        status_code(status_code_), message(message_), cookie(cookie_)
    { }
    http_response::http_response( http_response&& r ) : 
        version_(std::move(r.version_)), status_code_(r.status_code_), message_(r.message_),
        header_(std::move(r.header_)), body_(std::move(r.body_)), cookie_(std::move(r.cookie_)),
        version(version_), header(header_), body(body_), 
        status_code(status_code_), message(message_), cookie(cookie_)
    { }
    void http_response::write( std::string&& data ) {
        this->body_.append(std::move(data));
        this_task::yield();
    }
    void http_response::write( const std::string& data ) {
        this->body_.append(data);
        this_task::yield();
    }
    void http_response::write( const char* data ) {
        this->body_.append(data, strlen(data));
        this_task::yield();
    }
    void http_response::write( const char* data, size_t len ) {
        this->body_.append(data, len);
        this_task::yield();
    }

    http_response& http_response::operator = ( const http_response& r ) {
        if ( this == &r ) return *this;

        version_ = r.version_;
        status_code_ = r.status_code_;
        message_ = r.message_;
        header_ = r.header_;
        body_ = r.body_;
        cookie_ = r.cookie_;
        return *this;
    }

    http_response& http_response::operator = ( http_response&& r ) {
        if ( this == &r ) return *this;

        version_ = std::move(r.version_);
        status_code_ = std::move(r.status_code_);
        message_ = std::move(r.message_);
        header_ = std::move(r.header_);
        body_ = std::move(r.body_);
        cookie_ = std::move(r.cookie_);
        return *this;
    }

    void http_response::load_file( const std::string& path ) {
        if ( ! this->body_.load_file(path) ) {
            this->status_code_ = CODE_404; return;
        }
        this->header["Content-Type"] = content_type::by_extension( path );
    }

    // Connection Methods
    namespace http_connection {
        void __send_http_package( 
            iadapter* adapter, 
            std::string& buf, header_t& h, body_t& b 
        ) {
            // Calculate the body's size
            std::string _temp;
            if ( b.is_chunked ) {
                h["Transfer-Encoding"] = "chunked";
            }
            if ( b.is_gzipped ) {
                h["Content-Encoding"] = "gzip";
                _temp = b.raw();
                _temp = utils::gzip_data( _temp );
            }
            if ( !b.is_chunked ) {
                if ( b.is_gzipped ) {
                    h["Content-Length"] = std::to_string(_temp.size());
                } else {
                    h["Content-Length"] = std::to_string(b.calculate_size());
                }
            }

            h >> buf;
            buf += "\r\n";

            ON_DEBUG_CONET(
                std::cout << "will send: \n" << buf << std::endl;
            )
            // Write the header
            adapter->write( buf );

            if ( !b.is_chunked && b.is_gzipped ) {
                // All data is in _temp
                adapter->write(std::move(_temp));
                return;
            }

            // Write the body according to the content length or chunk parts
            if ( ! b.is_gzipped ) {
                for ( auto ib = b.begin(); ib != b.end(); ++ib ) {
                    if ( ib->ct.size() == 0 ) continue;
                    const std::string& _piece = (ib->f == 0 ? ib->ct : utils::file_cache::load_file(ib->ct));

                    if ( b.is_chunked ) {
                        adapter->write( __decimal2hex(_piece.size()) + "\r\n" );
                    }
                    adapter->write(_piece);
                    if ( b.is_chunked ) {
                        adapter->write("\r\n");
                    }
                    this_task::yield();
                }
            } else {
                size_t _sent = 0;
                for ( ; _sent + 4096 < _temp.size(); _sent += 4096 ) {
                    adapter->write( __decimal2hex(4096) + "\r\n" );
                    adapter->write( _temp.c_str() + _sent, 4096 );
                    adapter->write( "\r\n" );
                }
                adapter->write( __decimal2hex(_temp.size() - _sent) + "\r\n" );
                adapter->write( _temp.c_str() + _sent, _temp.size() - _sent );
                adapter->write( "\r\n" );
            }
            if ( b.is_chunked ) {
                adapter->write("0\r\n\r\n");   // END of CHUNK
            }
        }

        // Parse a http package's head and body part
        bool __read_http_package( 
            iadapter *adapter,
            header_t& h,
            http_cookie& c,
            body_t& b,
            duration_t timedout
        ) {
            do {
                auto _hline = adapter->read_crlf( timedout );
                if ( _hline.first != op_done ) return false;
                if ( _hline.second.size() == 0 ) break; // End of header
                ON_DEBUG_CONET(
                    std::cout << "fetch header line: <" << _hline.second << ">" << std::endl;
                )
                size_t _ = _hline.second.find(':');
                std::string _k = _hline.second.substr(0, _);
                std::string _v = _hline.second.substr(_ + 1);
                utils::trim(_v);
                _k = utils::string_capitalize(_k, " -_");
                if ( _k == "Set-Cookie") {
                    auto _cparts = utils::split(_v, std::vector<std::string>{"; "});
                    if ( _cparts.size() == 0 ) continue;
                    auto _ckv = utils::split(_cparts[0], "=");
                    if ( _ckv.size() == 1 ) _ckv.push_back(std::string(""));
                    cookie_t _c = cookie_create(_ckv[0], _ckv[1]);
                    for ( size_t i = 1; i < _cparts.size(); ++i ) {
                        auto _cargs = utils::split(_cparts[i], "=");
                        if ( _cargs[0] == "Expires" ) {
                            std::tm _tm = {};
                            strptime(_cargs[1].c_str(), "%a, %d %b %Y %H:%M:%S %Z", &_tm);
                            _c.expires = std::mktime(&_tm);
                        }
                        else if ( _cargs[0] == "Max-Age" ) {
                            _c.max_age = std::stoi(_cargs[1]);
                        }
                        else if ( _cargs[0] == "Domain" ) {
                            _c.domain = _cargs[1];
                        }
                        else if ( _cargs[0] == "Path" ) {
                            _c.path = _cargs[1];
                        }
                        else if ( _cargs[0] == "Secure" ) {
                            _c.secure_only = true;
                        }
                        else if ( _cargs[0] == "HttpOnly" ) {
                            _c.http_only = true;
                        }
                        else if ( _cargs[0] == "SameSite" ) {
                            if ( _cargs[1] == "Strict" ) _c.same_site = SameSite_Strict;
                            else if ( _cargs[1] == "Lax" ) _c.same_site = SameSite_Lax;
                            else if ( _cargs[1] == "None" ) _c.same_site = SameSite_None;
                        }
                    }
                    c.set_cookie(std::move(_c));
                } else if ( _k == "Cookie" ) {
                    auto _cvalues = utils::split(_v, "; ");
                    for ( auto& _ckv : _cvalues ) {
                        auto _c = utils::split(_ckv, "=");
                        if ( _c.size() == 1 ) _c.push_back(std::string(""));
                        c.set_cookie(_c[0], _c[1]);
                    }
                }
                h[_k] = pe::utils::trim(_v);
            } while ( true );

            if ( h.contains("Content-Encoding") ) {
                std::string _ce = h["Content-Encoding"];
                auto _cp = pe::utils::split(_ce, std::vector<std::string>({",", " "}));
                for ( const auto& _c : _cp ) {
                    if ( _c == "chunked" ) b.is_chunked = true;
                    if ( _c == "gzip" ) b.is_gzipped = true;
                }
            }
            if ( h.contains("Transfer-Encoding") ) {
                std::string _te = h["Transfer-Encoding"];
                auto _cp = pe::utils::split(_te, std::vector<std::string>({",", " "}));
                for ( const auto& _c : _cp ) {
                    if ( _c == "chunked" ) b.is_chunked = true;
                    if ( _c == "gzip" ) b.is_gzipped = true;
                }
                ON_DEBUG_CONET(
                std::cout << "response header contains transfer-encoding, the body has flag: "
                    << "chunked(" << (b.is_chunked ? "true" : "false") << "), "
                    << "gzipped(" << (b.is_gzipped ? "true" : "false") << ")"
                    << std::endl;
                )
            }

            if ( b.is_chunked == false ) {
                // Check the content length
                if ( !h.contains("Content-Length") ) {
                    // No body
                    return true;
                }
                std::string _cl = h["Content-Length"];
                ON_DEBUG_CONET(
                    std::cout << "response content length: " << std::stoi(_cl) << std::endl;
                )

                auto _r = adapter->read_enough( (size_t)std::stoi(_cl), timedout );
                if ( _r.first != op_done ) return false;
                ON_DEBUG_CONET(
                    std::cout << "read enough up to " << _r.second.size() << std::endl;
                )
                b.append( std::move(_r.second) );
                // b.container.emplace_back( std::move(_r.second) );
            } else {
                do {
                    auto _hex = adapter->read_crlf( timedout );
                    if ( _hex.first != op_done ) return false;
                    size_t _hlen = (size_t)__hex2decimal( _hex.second.c_str(), _hex.second.size() );
                    ON_DEBUG_CONET(
                        std::cout << "response get chunked hex: " << 
                            _hex.second << "(" << _hlen << ")" << std::endl;
                    )
                    auto _part = adapter->read_enough( _hlen + 2, timedout );
                    if ( _part.first != op_done ) return false;
                    if ( _hlen == 0 ) break;
                    _part.second.erase( _part.second.size() - 2 );
                    b.append( std::move(_part.second) );
                    this_task::yield();
                } while ( true );
                ON_DEBUG_CONET(
                    std::cout << "after fetch all chunked body" << std::endl;
                )
            } 
            return true;
        }

        void __read_response_package(
            iadapter * adapter, 
            http_response & resp, 
            duration_t timedout
        ) {
            std::string _cache;
            auto _top_line = adapter->read_crlf( timedout );
            if ( _top_line.first != op_done ) {
                resp.status_code = CODE_001; return;
            }
            auto _pparts = pe::utils::split(_top_line.second, " ");
            if ( _pparts.size() < 3 ) {
                resp.status_code = CODE_400; return;
            }
            resp.version = _pparts[0];
            resp.status_code = (CODE)std::stoi(_pparts[1]);
            resp.message = pe::utils::join(_pparts.begin() + 2, _pparts.end(), " ");

            if ( !__read_http_package(adapter, resp.header, resp.cookie, resp.body, timedout) ) {
                resp.status_code = CODE_001;
            }
        }

        // Use a sock5 to send the request and specified the target address
        http_response send_request( 
            const peer_t& socks5, 
            http_request& req, 
            const std::string& host, 
            uint16_t port, 
            duration_t timeout 
        ) {
            // Create the adapter according to the socks5 and ssl status
            netadapter *_padapter = NULL;
            if ( socks5 ) {
                if ( req.is_security() ) {
                    _padapter = new socks5sadapter( socks5 );
                } else {
                    _padapter = new socks5adapter( socks5 );
                }
            } else {
                if ( req.is_security() ) {
                    _padapter = new ssladapter;
                } else {
                    _padapter = new tcpadapter;
                }
            }
            std::shared_ptr< netadapter > _adapter(_padapter);

            // Format the host
            std::string _host = host + ":" + std::to_string(port);

            // Create the response object
            http_response _resp;

            // Connect
            if ( !_adapter->connect( _host ) ) {
                _resp.status_code = CODE_001; return _resp;
            }

            // Create the sending buffer
            std::string _buf;

            // Protocol Line
            _buf += req.method() + " " + req.path();
            url_params::operator >>( req.params, _buf);
            _buf += " ";
            req.version >> _buf;
            _buf += "\r\n";

            // Send the request
            req.cookie.dump_as_request(_buf);
            __send_http_package( _adapter.get(), _buf, req.header, req.body );
            // Wait for resposne
            __read_response_package( _adapter.get(), _resp, timeout );
            return _resp;
        }

        // Send the request to peer server and get the response
        http_response send_request( http_request& req, duration_t timeout ) {
            auto _hp = pe::utils::split( req.host(), ":" );
            if ( _hp.size() == 1 ) _hp.push_back(req.is_security() ? "443" : "80" );
            return send_request( peer_t::nan, req, _hp[0], std::stoi(_hp[1]), timeout );
        }

        // Send the request to some other server(not the resolv of host in req)
        http_response send_request( http_request& req, const peer_t& host, duration_t timeout ) {
            return send_request( peer_t::nan, req, host.ip.str(), host.port, timeout );
        }
       
        // Use a socks5 to send the request 
        http_response send_request( const peer_t& socks5, http_request& req, duration_t timeout ) {
            auto _hp = pe::utils::split( req.host(), ":" );
            if ( _hp.size() == 1 ) _hp.push_back(req.is_security() ? "443" : "80" );
            return send_request( socks5, req, _hp[0], std::stoi(_hp[1]), timeout );
        }
    }

    // Create a simple server
    namespace http_server {
        // Listen on a socket, the socket can be tcp, ssl,
        // the socket must be bind to the port first
        void listen( std::function< void (http_request&) > cb ) {
            tcp::listen([cb]() {
                tcp_serveradapter _tsa;
                // 1 second linger time
                // rawf::lingertime(this_task::get_id(), true, 1);
                while ( true ) {
                    auto _pline = _tsa.read_crlf( std::chrono::minutes(10) );
                    if ( _pline.first != op_done ) break;
                    auto _pparts = pe::utils::split(_pline.second, " ");
                    if ( _pparts.size() != 3 ) break;   // Invalidate http request protocol, drop

                    http_request _req;

                    // Method
                    _req.method() = pe::utils::string_toupper(_pparts[0]);

                    // Path and parameters
                    size_t _ppos = _pparts[1].find("?");
                    if ( _ppos == std::string::npos ) {
                        _req.path() = _pparts[1];
                    } else {
                        _req.path() = _pparts[1].substr(0, _ppos);
                        _req.params = url_params::parse(
                            _pparts[1].c_str() + _ppos, 
                            _pparts[1].size() - _ppos
                        );
                    }
                    // Version
                    _req.version = _pparts[2];

                    // Read rest parts
                    if ( ! http_connection::__read_http_package( 
                        &_tsa, _req.header, _req.cookie, _req.body, std::chrono::minutes(1)) 
                    ) break;

                    // this_task::yield();

                    // Invoke the worker
                    cb(_req);

                    if ( _req.header.contains("Connection") ) {
                        auto _c = _req.header["Connection"];
                        if ( utils::string_tolower(_c) != "keep-alive" ) break;
                    }
                }
            });
        }
        // Send the response
        void send_response( http_response& resp ) {
            static const std::string _h_server = "PECoNet/0.1";
            if ( !resp.header.contains("Server") ) {
                resp.header["Server"] = _h_server;
            }

            std::string _buf;
            _buf += (std::string)resp.version;
            _buf += " ";
            _buf += std::to_string(resp.status_code);
            _buf += " ";
            _buf += code_message(resp.status_code);
            _buf += "\r\n";

            tcp_serveradapter _tsa;

            resp.cookie.dump_as_response(_buf);
            http_connection::__send_http_package( &_tsa, _buf, resp.header, resp.body );
        }

    }
}}}

// Push Chen
//

