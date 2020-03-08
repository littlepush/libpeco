/*
    filemgr.cpp
    PECoUtils
    2019-10-30
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

#include <peco/utils/filemgr.h>
#include <zlib.h>
#include <fcntl.h>
#include <cstdio>
#include <dirent.h>
#include <stack>

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

namespace pe { namespace utils {

    // Check if folder existed
    bool is_folder_existed( const string& path ) {
        struct stat _dir_info;
        if ( stat(path.c_str(), &_dir_info) != 0 ) return false;
        return (bool)(_dir_info.st_mode & S_IFDIR);
    }

    // Check if file existed
    bool is_file_existed( const string& path ) {
        struct stat _f_info;
        if ( stat(path.c_str(), &_f_info) != 0 ) return false;
        return !(bool)(_f_info.st_mode & S_IFDIR);
    }

    // Create a folder at the path
    bool make_dir( const std::string& path ) {
        if ( mkdir(path.c_str(), ACCESSPERMS) == 0 ) return true;
        // If already existed, also success
        return errno == EEXIST;
    }

    // Create multiple levels of directories
    bool rek_make_dir( const std::string& path ) {
        auto _path = split( path, "/" );
        std::string _dpath;
        if ( path[0] == '/' ) _dpath = "/";
        while ( _path.size() > 0 ) {
            _dpath += (_path[0] + "/");
            if ( !make_dir(_dpath) ) return false;
            _path.erase(_path.begin());
        }
        return true;
    }

    /*
    Temp folder
    Create a temp folder under /dev/shm and remove it when destroy
    */
    temp_folder::temp_folder(const string& root_folder) {
        string __random__ = std::to_string((uint32_t)time(NULL));
        folder_path_ = root_folder + md5(__random__);
        mkdir(folder_path_.c_str(), ACCESSPERMS);
    }
    temp_folder::~temp_folder() {
        string _cmd = "rm -rf " + folder_path_;
        ignore_result(system(_cmd.c_str()));
    }

    // Get the temp folder path
    const std::string& temp_folder::path() const {
        return folder_path_;
    }

    // Copy File
    bool copy_file(const string& in_file, const string& out_file, int mod) {
        int _infile = open(in_file.c_str(), O_RDONLY, 0);
        if ( _infile == -1 ) return false;
        int _outfile = open(out_file.c_str(), O_WRONLY | O_CREAT, mod);
        if ( _outfile == -1 ) {
            close(_infile); return false;
        }

        char _buf[BUFSIZ]; size_t _size;
        while ( (_size = read(_infile, _buf, BUFSIZ)) > 0 ) {
            ignore_result(write(_outfile, _buf, _size));
        }

        close(_infile);
        close(_outfile);
        return true;
    }

    // Get dirname of a path
    std::string dirname( const std::string& path ) {
        size_t _l = path.rfind('/');
        if ( _l == std::string::npos ) return "./";
        return path.substr(0, _l + 1);  // Include '/'
    }

    // Get the filename of a path
    std::string filename( const std::string& path ) {
        std::string _ff(std::forward<std::string>(full_filename(path)));
        size_t _l = _ff.rfind('.');
        if ( _l == std::string::npos ) return _ff;
        return _ff.substr(0, _l);
    }

    // Get the full filename of a path
    std::string full_filename( const std::string& path ) {
        size_t _l = path.rfind('/');
        if ( _l == std::string::npos ) return path;
        return path.substr(_l + 1);
    }

    // Get the ext of a path
    std::string extension( const std::string& path ) {
        std::string _fpath = full_filename(path);
        size_t _l = _fpath.rfind('.');
        if ( _l == std::string::npos ) return std::string("");
        // Ignore hidden file's dot
        if ( _l == 0 ) return std::string("");
        return _fpath.substr(_l + 1);
    }

    // Get update time of a given file
    time_t file_update_time( const std::string& path ) {
        struct stat _finfo;
        if ( stat( path.c_str(), &_finfo) != 0 ) return (time_t)0;
#ifdef __APPLE__
        return _finfo.st_mtimespec.tv_sec;
#else
        return _finfo.st_mtime;
#endif
    }

    // Scan a given dir and get all files.
    void scan_dir( const std::string& path, dir_filter_t filter, bool ignore_hidden ) {
        std::string _currentpath = path;
        if ( *_currentpath.rbegin() != '/' ) _currentpath += '/';

        DIR *_dir = opendir(path.c_str());
        if ( _dir == NULL ) return;

        struct dirent* _node;
        while ( (_node = readdir(_dir)) != NULL ) {
            std::string _dpath(_node->d_name);
            if ( _dpath == "." || _dpath == ".." ) continue;
            // Check if ignore hidden file/folders
            if ( _dpath[0] == '.' && ignore_hidden ) continue;
            if ( !filter ) continue;    // do nothing
            filter(_currentpath + _dpath, (_node->d_type == DT_DIR));
        }
        closedir(_dir);
    }

    // Scan all levels
    void rek_scan_dir( const std::string& path, dir_filter_t filter, bool ignore_hidden ) {
        std::string _currentpath = path;
        if ( *_currentpath.rbegin() != '/' ) _currentpath += '/';

        DIR *_dir = opendir(path.c_str());
        if ( _dir == NULL ) return;

        struct dirent* _node;
        while ( (_node = readdir(_dir)) != NULL ) {
            // Ignore unknow node
            if ( _node->d_type == DT_UNKNOWN ) continue;
            
            std::string _dpath(_node->d_name);
            if ( _dpath == "." || _dpath == ".." ) continue;
            // Check if ignore hidden file/folders
            if ( _dpath[0] == '.' && ignore_hidden ) continue;
            if ( !filter ) continue;    // do nothing
            if ( filter(_currentpath + _dpath, (_node->d_type == DT_DIR)) ) {
                // go on scan if the filter return true
                if ( _node->d_type != DT_DIR ) continue;    // skip file
                rek_scan_dir(_currentpath + _dpath, filter, ignore_hidden);
            }
        }
        closedir(_dir);
    }
    // Remove a folder or file
    void fs_remove( const std::string& path ) {
        if ( is_file_existed(path) ) {
            // This is file
            ::remove(path.c_str());
        }
        if ( is_folder_existed(path) ) {
            // This is folder
            std::stack< std::pair< std::string, bool > > _cache;
            rek_scan_dir(path, [&_cache](const std::string& p, bool f) {
                _cache.emplace( std::make_pair(p, f) );
                return true;
            }, false);
            while ( _cache.size() != 0 ) {
                auto& _p = _cache.top();
                if ( _p.second == true ) {
                    ::rmdir(_p.first.c_str());
                } else {
                    ::remove(_p.first.c_str());
                }
                _cache.pop();
            }
        }
        // do nothing if not existed
    }

    // Singleton
    file_cache& file_cache::_() {static file_cache __; return __;}
    // Load file into memory
    // Each time will check the file's uptime
    // if greater than cached item, then reload
    const std::string& file_cache::load_file( const std::string& path ) {
        static std::string __empty__;
        time_t _ct = file_update_time( path );
        if ( _ct == 0 ) return __empty__;

        auto _i = _().cache_.find(path);
        if ( _i != _().cache_.end() ) {
            if ( _i->second.second == _ct ) {
                return _i->second.first;
            } else {
                // Remove the old cache
                _().cache_.erase(_i);
            }
        }
        std::ifstream _ifs(path);
        std::string _content(
            (std::istreambuf_iterator<char>(_ifs)), 
            (std::istreambuf_iterator<char>())
        );
        _ifs.close();
        _().cache_.emplace( 
            std::make_pair(
                std::move(path),
                std::make_pair(std::move(_content), _ct)
            )
        );
        return _().cache_[path].first;
    }

    // Remove the file cache from memory
    void file_cache::unload_file( const std::string& path ) {
        _().cache_.erase(path);
    }
}}

// Push Chen
