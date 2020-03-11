/*
    process.cpp
    PECoTask
    2019-10-31
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/process.h>
#include <unistd.h>
#include <peco/peutils.h>

#if PZC_TARGET_LINUX
#include <sys/types.h>
#include <sys/wait.h>
#endif

extern char** environ;

namespace pe {
    namespace co {
#define __READ_FD   0
#define __WRITE_FD  1

        // Check if a pipe has data to read
        bool __pipe_pending( long p ) {
            fd_set _fs;
            FD_ZERO( &_fs );
            FD_SET( p, &_fs );

            int _ret = 0; struct timeval _tv = {0, 0};

            do {
                _ret = ::select( p + 1, &_fs, NULL, NULL, &_tv );
            } while ( _ret < 0 && errno == EINTR );
            return (_ret > 0);
        }

        std::string __pipe_read( long p ) {
            static size_t max_size = 4096;  // 4K for max reading buffer
            size_t buf_size = 512;
            std::string _buffer(buf_size, '\0');
            size_t _received = 0;
            size_t _leftspace = buf_size;
            do {
                int _retcode = ::read(p, &_buffer[0] + _received, _leftspace);
                if ( _retcode < 0 ) {
                    if ( errno == EINTR ) continue;     // signal 7, retry
                    if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                        // no more data on a non-blocking socket
                        _buffer.resize(_received);
                        break;
                    }
                    // Other error
                    _buffer.resize(0); break;
                } else if ( _retcode == 0 ) {
                    // Closed
                    _buffer.resize(0); break;
                } else {
                    _received += _retcode;
                    _leftspace -= _retcode;
                    if ( _leftspace > 0 ) {
                        // unfull
                        _buffer.resize(_received); break;
                    } else {
                        if ( buf_size * 2 <= max_size ) {
                            buf_size *= 2;
                        } else if ( buf_size < max_size ) {
                            buf_size = _buffer.max_size();
                        } else {
                            break;  // no more space for current reading
                        }

                        // Resize the buffer and try to read again
                        _leftspace = buf_size - _received;
                        _buffer.resize(buf_size);
                    }
                }
            } while ( _leftspace > 0 );
            return _buffer;
        }

        process::process() : 
        c2p_out_{-1, -1}, c2p_err_{-1, -1}, c2p_in_{-1, -1}
        { }

        process::process( const std::string& cmd ) :
        c2p_out_{-1, -1}, c2p_err_{-1, -1}, c2p_in_{-1, -1}
        {  
            cmd_ = utils::split(cmd, " ");
        }

        process::~process( ) { }

        void process::append( const std::string& arg ) {
            cmd_.push_back(arg);
        }

        process& process::operator << (const std::string& arg) {
            cmd_.push_back(arg);
            return *this;
        }

        // Write input to the process
        void process::input( std::string&& data ) {
            if ( c2p_in_[__WRITE_FD] == -1 ) return;
            ignore_result(::write(c2p_in_[__WRITE_FD], data.c_str(), data.size()));
        }

        // Send EOF
        void process::send_eof() {
            if ( c2p_in_[__WRITE_FD] == -1 ) return;
            ::close(c2p_in_[__WRITE_FD]);
            c2p_in_[__WRITE_FD] = -1;
        }

        // Tell if current process is still running
        bool process::running() const {
            return c2p_out_[__READ_FD] != -1;
        }

        int process::run( ) {
            // Create the pipe just at the beginning of run
            ignore_result(pipe(c2p_out_)); 
            ignore_result(pipe(c2p_err_)); 
            ignore_result(pipe(c2p_in_));

            pid_t _p = fork();
            if ( _p < 0 ) {
                ::close(c2p_out_[0]); ::close(c2p_out_[1]);
                ::close(c2p_err_[0]); ::close(c2p_err_[1]);
                ::close(c2p_in_[0]); ::close(c2p_in_[1]);
                return -99;
            }
            if ( _p == 0 ) {
                // In Child Process, execlp the command

                // Redirect Child's stdout and stderr to Parent
                dup2(c2p_out_[__WRITE_FD], STDOUT_FILENO);
                dup2(c2p_err_[__WRITE_FD], STDERR_FILENO);
                // Redirect Parent's stdin to child
                dup2(c2p_in_[__READ_FD], STDIN_FILENO);

                // Close unused pipe
                close(c2p_out_[__READ_FD]); c2p_out_[__READ_FD] = -1;
                close(c2p_out_[__WRITE_FD]); c2p_out_[__WRITE_FD] = -1;

                close(c2p_err_[__READ_FD]); c2p_err_[__READ_FD] = -1;
                close(c2p_err_[__WRITE_FD]); c2p_err_[__WRITE_FD] = -1;

                close(c2p_in_[__WRITE_FD]); c2p_in_[__WRITE_FD] = -1;
                close(c2p_in_[__READ_FD]); c2p_in_[__READ_FD] = -1;

                // std::cout << utils::join(cmd_.begin(), cmd_.end(), " ") << std::endl;

                // Try to search the binary file
                std::string _cmd = cmd_[0];
                if ( !(_cmd[0] == '/' || _cmd[0] == '.') ) {
                    std::string _e_path = getenv("PATH");
                    ON_DEBUG_COTASK(
                        std::cout << "system path: " << _e_path << std::endl;
                    )
                    auto _syspaths = utils::split(_e_path, ":");
                    for ( const auto& p : _syspaths ) {
                        std::string _cmdfile = p + "/" + _cmd;
                        ON_DEBUG_COTASK(
                            std::cout << "search path: " << _cmdfile << std::endl;
                        )
                        if ( utils::is_file_existed(_cmdfile) ) {
                            cmd_[0] = _cmdfile;
                            break;
                        }
                    }
                }

                char **_args = (char **)calloc(cmd_.size() + 1, sizeof(char *));
                for ( size_t i = 0; i < cmd_.size(); ++i ) {
                    _args[i] = &cmd_[i][0];
                }
                _args[cmd_.size()] = NULL;
                execve(cmd_[0].c_str(), _args, environ);
                std::string _errmsg(strerror(errno));
                _errmsg += "\n";
                _errmsg = "errno: " + std::to_string(errno) + ", " + _errmsg;
                ignore_result(::write(c2p_err_[__WRITE_FD], _errmsg.c_str(), _errmsg.size()));
                exit(errno);
                // The running image will be replaced by the command
            } else {
                // In Parent Process, create new job to read the output from child

                // Close unused pipe
                close(c2p_out_[__WRITE_FD]); c2p_out_[__WRITE_FD] = -1;
                close(c2p_err_[__WRITE_FD]); c2p_err_[__WRITE_FD] = -1;
                close(c2p_in_[__READ_FD]); c2p_in_[__READ_FD] = -1;

                loop::main.do_job(c2p_out_[__READ_FD], [this]() {
                    parent_task::guard _pg;
                    while ( true ) {
                        long _pid = (long)this_task::get_id();
                        if ( !__pipe_pending(_pid) ) {
                            auto _sig = this_task::wait_for_event(
                                event_read, std::chrono::milliseconds(1000));
                            if ( _sig == no_signal ) continue;
                            if ( _sig == bad_signal ) break;
                        }

                        std::string _buf = std::forward< std::string >(__pipe_read(_pid));
                        // Pipe closed
                        if ( _buf.size() == 0 ) break;
                        if ( this->stdout ) this->stdout( std::move(_buf) );
                    }
                });

                loop::main.do_job(c2p_err_[__READ_FD], [this]() {
                    parent_task::guard _pg;
                    while ( true ) {
                        long _pid = (long)this_task::get_id();
                        if ( !__pipe_pending(_pid) ) {
                            auto _sig = this_task::wait_for_event(
                                event_read, std::chrono::milliseconds(1000));
                            if ( _sig == no_signal ) continue;
                            if ( _sig == bad_signal ) break;
                        }

                        std::string _buf = std::forward< std::string >(__pipe_read(_pid));
                        // Pipe closed
                        if ( _buf.size() == 0 ) break;
                        if ( this->stderr ) this->stderr( std::move(_buf) );
                    }
                });

                for ( int i = 0; i < 2; ++i ) {
                    this_task::holding();
                }
                
                // Close input stream first
                if ( c2p_in_[__WRITE_FD] != -1 ) {
                    ::close(c2p_in_[__WRITE_FD]);
                    c2p_in_[__WRITE_FD] = -1;
                }
                c2p_out_[__READ_FD] = -1;
                c2p_err_[__READ_FD] = -1;

                int _retcode = -10000;
                pid_t _w = waitpid(_p, &_retcode, 0);
                if ( _w == -1 ) {
                    std::cerr << "waitpid error: " << errno << std::endl;
                }

                ON_DEBUG_COTASK(
                    std::cout << "run command `" << utils::join(cmd_.begin(), cmd_.end(), " ") 
                        << "` finished with ret code: " << _retcode << std::endl;
                )
                return _retcode;
            }
        }

        void parent_pipe_for_read( int p[2] ) {
            ::close(p[PIPE_WRITE_FD]);
        }
        void parent_pipe_for_write( int p[2] ) {
            ::close(p[PIPE_READ_FD]);
        }
        void child_dup_for_read( int p[2], int fno ) {
            dup2(p[PIPE_WRITE_FD], fno);
            ::close(p[0]); ::close(p[1]);
        }
        void child_dup_for_write( int p[2], int fno ) {
            dup2(p[PIPE_READ_FD], fno);
            ::close(p[0]); ::close(p[1]);            
        }

        bool pipe_is_pending( long p ) {
            return __pipe_pending(p);
        }
        std::string pipe_read( long p ) {
            return __pipe_read(p);
        }

    }
}

// Push Chen
