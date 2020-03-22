#include <peco/peutils.h>
#include <peco/cotask.h>
using namespace pe;
using namespace pe::co;

int g_test = 0;

int main( int argc, char* argv[] ) {

    ON_DEBUG(
        pe::co::enable_cotask_trace();
    )

    this_loop.do_job([]() {

        for ( int i = 0; i < 5; ++i ) {
            this_loop.do_job([]() {
                parent_task::guard _pg;
                this_task::sleep( std::chrono::seconds(1) );
                ++g_test;
                std::cout << "in task: " << this_task::get_id() << ", g_test is: " << g_test << std::endl;
            });
        }
        for ( int i = 0; i < 5; ++i ) {
            ignore_result( this_task::holding() );
        }

        std::cout << "after case done, g_test is: " << g_test << std::endl;
    });

    // for ( int i = 0; i < 2; ++i ) {
    //     this_loop.do_job([]() {
    //         ++g_test;
    //         std::cout << "in task " << this_task::get_task()->id << ", g_test is: " << g_test << std::endl;
    //     });
    // }

    pe::utils::sys::update_sysinfo();
    std::cout << "CPU Count: " << pe::utils::sys::cpu_count() << std::endl;
    std::cout << "Total Memory: " << pe::utils::sys::total_memory() << std::endl;

    this_loop.do_loop([]() {
        pe::utils::sys::update_sysinfo();
        std::cout << "CPU Loads: " << pe::utils::join(
            pe::utils::sys::cpu_loads(), " ") << std::endl;
        std::cout << "CPU Usage: " << pe::utils::sys::cpu_usage() << std::endl;
        std::cout << "Memory Usage: " << pe::utils::sys::memory_usage() << std::endl;
    }, std::chrono::seconds(1));
    this_loop.do_loop([]() {
        std::cout << time(NULL) << std::endl;
    }, std::chrono::milliseconds(1000));

    // this_loop.do_job([]() {
    //     while ( true ) {
    //         pe::co::this_task::sleep(std::chrono::seconds(3));
    //         std::cout << "job at: " << time(NULL) << std::endl;
    //     }
    // });

    this_loop.do_job([]() {
        std::cout << "hold jot at: " << time(NULL) << std::endl;
        this_loop.do_delay([]() {
            parent_task::guard _pg;
            std::cout << "delay job done" << std::endl;
        }, std::chrono::seconds(5));
        pe::co::this_task::holding();
        std::cout << "job goon at: " << time(NULL) << std::endl;
    });

    // this_loop.do_job([]() {

    //     this_loop.do_job([]() {
    //         pe::co::parent_task::guard _g;
    //         pe::co::parent_task::go_on();
    //         pe::co::this_task::sleep(std::chrono::seconds(1));
    //         pe::co::parent_task::go_on();
    //         this_task::debug_info();
    //     });

    //     while ( pe::co::this_task::holding() ) {
    //         std::cout << "while holding true" << std::endl;
    //     }
    //     std::cout << "out of while holding" << std::endl;
    //     pe::co::this_task::debug_info();
    // });

    // this_loop.do_delay([]() {
    //     int a = 100;
    //     int b = 0;
    //     if ( b == 0 ) throw "b is 0";
    //     std::cout << a / b << std::endl;
    // }, std::chrono::seconds(2));

    // this_loop.do_delay([]() {
    //     pe::co::taskadapter _ta;
    //     _ta.step([]() {
    //         std::cout << "this is in task adapter" << std::endl;
    //         pe::co::this_task::sleep(std::chrono::seconds(1));
    //         std::cout << "still in the first step" << std::endl;
    //     });

    //     _ta.step([]() {
    //         std::cout << "after one second, do step again" << std::endl;
    //         pe::co::this_task::sleep(std::chrono::seconds(3));
    //         std::cout << "xxx step2 after 3 seconds" << std::endl;
    //     });

    //     pe::co::this_task::sleep(std::chrono::seconds(2));
    //     std::cout << "[x] now we are going to destory the task adapter" << std::endl;
    // }, std::chrono::seconds(3));

    // this_loop.do_job([]() {
    //     // In this case we will try to start run process
    //     pe::co::process _p("ls -lha");
    //     _p.std_out = []( std::string&& o ) {
    //         std::cout << o;
    //     };
    //     _p.std_err = []( std::string&& o ) {
    //         std::cerr << o;
    //     };
    //     int _ret = _p.run();
    //     std::cout << "end of make, return " << _ret << std::endl;
    // });

    // this_loop.do_job([]() {
    //     pe::co::process _p("bash -l");
    //     _p.std_out = []( std::string&& o ) { std::cout << o; };
    //     _p.std_err = []( std::string&& o ) { std::cerr << o; };
    //     pe::co::task* _tinput = this_loop.do_job([&_p]() {
    //         pe::co::waiting_signals _sig = pe::co::no_signal;
    //         fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK );
    //         do {
    //             _sig = pe::co::this_task::wait_fd_for_event(
    //                 STDIN_FILENO, pe::co::event_read, std::chrono::seconds(1)
    //             );
    //             if ( _sig == pe::co::bad_signal ) break;
    //             if ( _sig == pe::co::receive_signal ) {
    //                 std::string _cmdline;
    //                 std::getline(std::cin, _cmdline);
    //                 _cmdline += "\n";
    //                 _p.input(std::move(_cmdline));
    //             }
    //         } while ( true );
    //     });
    //     _p.run();
    //     task_exit(_tinput);
    // });

    this_loop.run();

    return 0;
}
