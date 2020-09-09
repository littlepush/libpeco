/*
    filemgr.h
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

#pragma once

#ifndef PE_CO_UTILS_FILEMGR_H__
#define PE_CO_UTILS_FILEMGR_H__

#include <peco/utils/stringutil.h>

namespace pe { namespace utils {
    // Check if folder existed
    bool is_folder_existed( const string& path );

    // Check if file existed
    bool is_file_existed( const string& path );

    // Create a folder at the path
    bool make_dir( const std::string& path );

    // Create multiple levels of directories
    bool rek_make_dir( const std::string& path );

    /*
    Temp folder
    Create a temp folder under /dev/shm and remove it when destroy
    */
    class temp_folder
    {
    protected:
        std::string             folder_path_;
    public:
        temp_folder(const string& root_folder = "/dev/shm/");
        ~temp_folder();

        // Get the temp folder path
        const std::string& path() const;
    };

    // Copy File
    bool copy_file(const std::string& in_file, const std::string& out_file, int mod = 0644);

    // Get dirname of a path
    std::string dirname( const std::string& path );

    // Get the filename of a path
    std::string filename( const std::string& path );

    // Get the full filename of a path
    std::string full_filename( const std::string& path );

    // Get the ext of a path
    std::string extension( const std::string& path );

    // Get update time of a given file
    time_t file_update_time( const std::string& path );

    // Filter in dir scan, return true to continue scan sub dir
    typedef std::function< bool ( const std::string&, bool ) > dir_filter_t;

    // Scan a given dir and get all files.
    void scan_dir( const std::string& path, dir_filter_t filter, bool ignore_hidden = true );

    // Scan all levels
    void rek_scan_dir( const std::string& path, dir_filter_t filter, bool ignore_hidden = true );

    // Remove a folder or file
    void fs_remove( const std::string& path );

    // File Cache
    class file_cache {
        typedef std::pair< std::string, time_t >        file_node_t;
        typedef std::map< std::string, file_node_t >    file_cache_t;

        file_cache_t        cache_;
        // Singleton
        static file_cache& _();
    public: 
        // Load file into memory
        // Each time will check the file's uptime
        // if greater than cached item, then reload
        static const std::string& load_file( const std::string& path );

        // Remove the file cache from memory
        static void unload_file( const std::string& path );
    };
}}

#endif
// Push Chen
