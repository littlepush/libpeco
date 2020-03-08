#include <peco/conet/conns/tcp.h>
#include <peco/conet/conns/udp.h>
#include <peco/conet/conns/uds.h>
#include <peco/conet/conns/ssl.h>
#include <peco/conet/protos/dns.h>
#include <peco/conet/protos/http.h>
#include <peco/conet/protos/redis.h>
#include <peco/conet/conns/netadapter.h>
#include <peco/peutils.h>

using namespace pe::co;

int main( int argc, char * argv[] ) {

    ON_DEBUG(
        pe::co::net::enable_conet_trace();
    )
    // uint32_t i = 12345;
    // std::cout << "i: " << i << std::endl;
    // std::cout << "htonl: " << htonl(i) << std::endl;
    // std::cout << "h2n: " << net::h2n(i) << std::endl;

    // uint32_t ni = htonl(12345);
    // std::cout << "ni: " << ni << std::endl;
    // std::cout << "ntohl: " << ntohl(ni) << std::endl;
    // std::cout << "n2h: " << net::n2h(ni) << std::endl;

    // this_loop.do_job([]() {
    //     net::tcp_factory::client += []( net::tcp_item&& client ) {
    //         parent_task::helper _ph;
    //         client.connect( net::peer_t("127.0.0.1:20001") );
    //         _ph.can_go_on();

    //         client.out << "hello";
    //     };
    //     if ( this_task::holding() ) {
    //         std::cout << "success" << std::endl;
    //     } else {
    //         std::cout << "failed success" << std::endl;
    //     }
    // });

    // net::tcp_factory::server & net::peer_t("0.0.0.0:12391") 
    // += []( net::tcp_item&& client ) {
    //     task_debug_info(client.conn_task);
    // };

    // net::SOCKET_T _so = net::tcp::create();
    // this_loop.do_job( _so, []() {
    //     std::cout << "in socket's task job" << std::endl;
    //     auto _ops = net::tcp::connect(net::peer_t("127.0.0.1:9900"));
    //     if ( _ops != net::op_done ) {
    //         std::cerr << "failed to connect!" << std::endl;
    //         return;
    //     } else {
    //         std::cout << "Connected!" << std::endl;
    //     }
    //     net::tcp::write("ls\n", 3);
    //     std::cout << "after write" << endl;
    //     string _buffer;
    //     _ops = net::tcp::read(_buffer);
    //     if ( _ops == net::op_failed ) {
    //         std::cout << "read failed" << std::endl;
    //     } else if ( _ops == net::op_timedout ) {
    //         std::cout << "read timedout" << std::endl;
    //     } else {
    //         std::cout << "received: " << _buffer << std::endl;
    //     }
    //     return;
    // });

    // net::uds_factory::server & "./test.socket"
    // += []( net::uds_item&& client ) {
    //     task_debug_info(client.conn_task);
    //     client.out << "Hello";
    //     this_task::sleep(std::chrono::seconds(1));
    // };

    // this_loop.do_delay([]{
    //     net::uds_factory::client += [](net::uds_item&& client) {
    //         client.connect("./test.socket");
    //         std::string _buf;
    //         client.in >> _buf;
    //         std::cout << _buf << std::endl;
    //     };
    // }, std::chrono::seconds(1));

    // auto _sslsvr = (net::ssl_factory::server & net::peer_t("0.0.0.0:12312"));
    // // net::ssl::set_ctx(_sslsvr.conn_task, net::ssl::get_ctx<...>
    // _sslsvr += []( net::ssl_item&& client ) {
    //     task_debug_info(client.conn_task);
    // };

    // net::ssl_factory::client += [](net::ssl_item&& client) {
    //     net::ssl::set_ssl(client.conn_task, ...);
        
    // }


    // net::SOCKET_T _lso = net::tcp::create(net::peer_t("0.0.0.0:9901"));
    // if ( SOCKET_NOT_VALIDATE(_lso) ) {
    //     std::cerr << "failed to create a listen socket bind to the port" << std::endl;
    //     return 1;
    // }
    // this_loop.do_job( _lso, []() {
    //     std::cout << "in listen task" << std::endl;
    //     net::tcp::listen([]() {
    //         string _buffer;
    //         net::tcp::read(_buffer);
    //         std::cout << "received: " << _buffer << std::endl;
    //         net::tcp::write(std::forward<string>(_buffer));
    //     });
    // });

    this_loop.do_job( [](){
        this_task::begin_tick();
        std::cout << pe::co::net::get_hostname("www.baidu.com") << std::endl;
        std::cout << "used: " << this_task::tick() << "ms" << std::endl;
        std::cout << pe::co::net::get_hostname("www.baidu.com") << std::endl;
        std::cout << "used: " << this_task::tick() << "ms" << std::endl;

        pe::co::net::set_query_server(
            "*.google*", 
            pe::co::net::peer_t("8.8.8.8:53"), 
            pe::co::net::peer_t("127.0.0.1:2046"));
        std::cout << pe::co::net::get_hostname("www.google.com") << std::endl;
        std::cout << pe::co::net::get_hostname("mail.google.com") << std::endl;
    });

    // this_loop.do_job([]() {
    //     this_task::begin_tick();
    //     net::http_request _req = net::http_request::fromURL("https://www.baidu.com");
    //     auto _resp = net::http_connection::send_request(_req);
    //     cout << "response from www.baidu.com is: " << _resp.status_code << std::endl;
    //     cout << _resp.header << std::endl;
    //     std::cout << "Baidu's content length is: " << 
    //         _resp.body.size() << std::endl;
    //     std::cout << "query www.baidu.com used: " << this_task::tick() << "ms" << std::endl;
    // });

    // this_loop.do_job([]() {
    //     this_task::begin_tick();
    //     net::http_request _req = net::http_request::fromURL("https://www.google.com");
    //     auto _resp = net::http_connection::send_request( 
    //         pe::co::net::peer_t("180.167.200.246:4001"), _req);
    //     cout << "response from www.google.com is: " << _resp.status_code << std::endl;
    //     cout << _resp.header << std::endl;
    //     std::cout << "query www.google.com used: " << this_task::tick() << "ms" << std::endl;
    //     for ( auto bit = _resp.body.begin(); bit != _resp.body.end(); ++bit ) {
    //         std::cout << *bit;
    //     }
    //     std::cout << std::endl;
    // });

    // net::SOCKET_T _httpSo = net::tcp::create(net::peer_t("0.0.0.0:8880"));
    // this_loop.do_job(_httpSo, []() {
    //     net::http_server::listen([]( net::http_request& req) {
    //         net::http_response _resp;
    //         _resp.load_file("./test/test_cast1.cpp");
    //         net::http_server::send_response(_resp);
    //     });
    //     std::cout << "stop to listen on 8880" << std::endl;
    // });

    // auto _g = std::make_shared< net::redis::group >(
    //     net::peer_t("192.168.71.130:6379"), "locald1@redis", 2);
    // this_loop.do_loop([_g]() {
    //     std::cout << _g->query("GET", "MyKey") << std::endl;
    //     // this_task::debug_info();
    // }, std::chrono::milliseconds(100));
	
	this_loop.do_job([]() {
        net::redis::group _rg("redis://dd56ba84fb2924ccac24206eb083f8bb@10.10.64.1");
        
		// net::redis::group _rg(net::peer_t("10.10.64.1:6379"), "dd56ba84fb2924ccac24206eb083f8bb", 1);
		auto _r = _rg.query("HSCAN", "proxy_cache", 0);
		std::cout << _r << std::endl;
	});

    // net::SOCKET_T _rSo = net::tcp::create(net::peer_t("0.0.0.0:8890"));
    // this_loop.do_job(_rSo, []() {
    //     net::tcp::listen([]() {
    //         net::SOCKET_T _pSo = net::tcp::create();
    //         task* _ptask = NULL;
    //         this_loop.do_job(_pSo, [&_ptask]() {
    //             do {
    //                 parent_task::guard _g;
    //                 if ( net::op_done != net::tcp::connect(net::peer_t("10.15.11.1:38422")) )
    //                     return;
    //                 _g.job_done();
    //                 _ptask = this_task::get_task();
    //             } while ( false );
    //            net::tcp::redirect_data(parent_task::get_task());
    //         });
    //         if ( !this_task::holding() ) return;    // Failed to connect to target
    //         net::tcp::redirect_data(_ptask);
    //     });
    // });


    this_loop.run();

    return 0;
}
