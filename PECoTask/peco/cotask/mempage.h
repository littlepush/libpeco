/*
    mempage.h
    PZCLibMemory
    2019-05-04
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PZC_LIB_MEMORY_MEMPAGE_H__
#define PZC_LIB_MEMORY_MEMPAGE_H__

#include <peco/utils/mustd.hpp>
#include <peco/cotask/memblock.hpp>

#ifndef _MEM_PAGE_SIZE
#define _MEM_PAGE_SIZE      8388608UL   // 8MB
#endif

#ifndef _MEM_PIECE_SIZE
#define _MEM_PIECE_SIZE     524288UL    // 512KB
#endif

namespace mu {
    namespace mem {

        /*
        This is a memory page management class.
        We alloc a big page(4MB) when the page item has been created.
        And seperate the page into 32 pieces, each piece is 128KB
        We will never release the page until exit the application if you
        choose to use the default main page item.
        */
        class page {
        public: 
            // The page size enum, to change the default value, use
            // -D in the compile time.
            enum {
                page_size               = _MEM_PAGE_SIZE,
                piece_size              = _MEM_PIECE_SIZE,
                piece_count             = _MEM_PAGE_SIZE / _MEM_PIECE_SIZE
            };

            typedef intptr_t            piece_t;
            typedef void*               piece_type;
            typedef void*               page_t;
            typedef block< piece_t >    piece_info_t;

            // Page Info node. Act as a list node
            struct __page_info{
                page_t                  p;  // Current page
                __page_info*            n;  // Next page
            };
        private: 
            // Use block to manage all free pieces
            piece_info_t                fpi_;       // Free Piece Info
            // The list node
            __page_info                 *pgi_header_, *pgi_tail_;

            // Create a new page, alloc the memory
            void __request_page_from_system();

        public: 

            // C'str and D'str
            page(); 
            ~page();

            // Get a piece from the page
            piece_type get_piece();

            // Release a page
            void release_piece(piece_type p);

            // Global main page item
            static page main;
        };
    }
}

#endif

// Push Chen
