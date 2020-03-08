/*
    gzip.cpp
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

#include <peco/utils/gzip.h>

namespace pe { namespace utils {
    /*
        Create a Gzip data block
     */
    struct gzip_block_t* gzip_create_block()
    {
        struct gzip_block_t *_pblock = (struct gzip_block_t*)malloc(sizeof(struct gzip_block_t));
        _pblock->block_data = NULL;
        _pblock->block_length = 0;
        _pblock->data_length = 0;
        return _pblock;
    }

    /*
        Destroy the Gzip data block
    */
    void gzip_release_block(struct gzip_block_t* block)
    {
        if ( block == NULL ) return;
        if ( block->block_data != NULL ) {
            free(block->block_data);
        #ifdef DEBUG
            block->block_data = NULL;
            block->block_length = 0;
            block->data_length = 0;
        #endif
        }
        free(block);
    }

    int gzip( const char* input_str, uint32_t length, struct gzip_block_t* pblock ) {
        static const int _chunkSize = 16384;
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = (uint)length;
        stream.next_in = (Bytef *)input_str;
        stream.total_out = 0;
        stream.avail_out = 0;

        int compression = Z_DEFAULT_COMPRESSION;

        if (deflateInit2(&stream, compression, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) == Z_OK)
        {
            pblock->block_data = (char *)malloc(_chunkSize);
            int _bufLen = _chunkSize;
            while (stream.avail_out == 0)
            {
                if (stream.total_out >= (unsigned int)_bufLen)
                {
                    //data.length += ChunkSize;
                    _bufLen += _chunkSize;
                    pblock->block_data = (char *)realloc(pblock->block_data, _bufLen);
                }
                stream.next_out = (uint8_t *)pblock->block_data + stream.total_out;
                stream.avail_out = (unsigned int)(_bufLen - stream.total_out);
                deflate(&stream, Z_FINISH);
            }
            deflateEnd(&stream);
            pblock->data_length = stream.total_out;
            pblock->block_length = _bufLen;
            return stream.total_out;
        }
        return -1;
    }

    int gunzip( const char* input_str, uint32_t length, struct gzip_block_t* pblock )
    {
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.avail_in = length;
        stream.next_in = (Bytef *)input_str;
        stream.total_out = 0;
        stream.avail_out = 0;
            
        if ( inflateInit2(&stream, 47) == Z_OK )
        {
            int _buflength = length * 1.5;
            char *_buffer = (char *)malloc(_buflength);
            int status = Z_OK;
            while ( status == Z_OK ) {
                if ( stream.total_out >= (unsigned int)_buflength ) {
                    _buflength += length / 2;
                    _buffer = (char *)realloc( _buffer, _buflength );
                }
                stream.next_out = (uint8_t *)(_buffer + stream.total_out);
                stream.avail_out = (unsigned)(_buflength - stream.total_out);
                status = inflate( &stream, Z_SYNC_FLUSH );
            }

            if ( inflateEnd(&stream) == Z_OK ) {
                if ( status == Z_STREAM_END ) {
                    pblock->block_data = _buffer;
                    pblock->data_length = stream.total_out;
                    pblock->block_length = _buflength;
                    return stream.total_out;
                }
            }
            free( _buffer );
        }
        return -1;
    }

    // Gzip Data
    std::string gzip_data( const char* input_str, uint32_t length ) {
        std::string _rstr;
        gzip_block_t* _pb = gzip_create_block();
        int _rcode = gzip( input_str, length, _pb );
        if ( _rcode > 0 ) {
            _rstr.append(_pb->block_data, _pb->data_length);
        }
        gzip_release_block(_pb);
        return _rstr;
    }
    std::string gzip_data( const std::string& input_str ) {
        std::string _rstr;
        gzip_block_t* _pb = gzip_create_block();
        int _rcode = gzip( input_str.c_str(), input_str.size(), _pb );
        if ( _rcode > 0 ) {
            _rstr.append(_pb->block_data, _pb->data_length);
        }
        gzip_release_block(_pb);
        return _rstr;
    }

    // GUnzip Data
    std::string gunzip_data( const char* input_str, uint32_t length ) {
        std::string _rstr;
        gzip_block_t* _pb = gzip_create_block();
        int _rcode = gunzip( input_str, length, _pb );
        if ( _rcode > 0 ) {
            _rstr.append(_pb->block_data, _pb->data_length);
        }
        gzip_release_block(_pb);
        return _rstr;
    }
    std::string gunzip_data( const std::string& input_str ) {
        std::string _rstr;
        gzip_block_t* _pb = gzip_create_block();
        int _rcode = gunzip( input_str.c_str(), input_str.size(), _pb );
        if ( _rcode > 0 ) {
            _rstr.append(_pb->block_data, _pb->data_length);
        }
        gzip_release_block(_pb);
        return _rstr;
    }
}}

// Push Chen
