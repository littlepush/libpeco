/*
    gzip.h
    PECoUtils
    2019-07-26
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

#ifndef PE_CO_UTILS_GZIP_H__
#define PE_CO_UTILS_GZIP_H__

#include <peco/utils/mustd.hpp>
#include <zlib.h>

namespace pe { namespace utils {
    struct gzip_block_t {
        char            *block_data;
        uint32_t        block_length;
        uint32_t        data_length;
    };

    /*
        Create a Gzip data block
     */
    struct gzip_block_t* gzip_create_block();
    /*
        Destroy the Gzip data block
    */
    void gzip_release_block(struct gzip_block_t* block);

    /*
        Compress the input string with gzip
        Return the length of the output string
    */
    int gzip( const char* input_str, uint32_t length, struct gzip_block_t* pblock );

    /*
        Decompress the input string with gzip
        Return the length of the output string
    */
    int gunzip( const char* input_str, uint32_t length, struct gzip_block_t* pblock );

    // Gzip Data
    std::string gzip_data( const char* input_str, uint32_t length );
    std::string gzip_data( const std::string& input_str );

    // GUnzip Data
    std::string gunzip_data( const char* input_str, uint32_t length );
    std::string gunzip_data( const std::string& input_str );
}}

#endif

// Push Chen
