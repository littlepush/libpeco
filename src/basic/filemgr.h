/*
    filemgr.h
    libpeco
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

#ifndef PECO_FILEMGR_H__
#define PECO_FILEMGR_H__

#include "pecostd.h"

namespace peco {
    
// Check if folder existed
bool is_folder_existed( const std::string& path );

// Check if file existed
bool is_file_existed( const std::string& path );

// Create a folder at the path
bool make_dir( const std::string& path );

// Create multiple levels of directories
bool rek_make_dir( const std::string& path );

// Get dirname of a path
std::string dir_name( const std::string& path );

// Get the filename of a path
std::string file_name( const std::string& path );

// Get the full filename of a path
std::string full_file_name( const std::string& path );

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

} // namespace peco

#endif
// Push Chen
