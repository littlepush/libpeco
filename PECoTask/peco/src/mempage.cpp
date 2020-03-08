/*
    mempage.cpp
    PZCLibMemory
    2019-05-04
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#include <peco/cotask/mempage.h>

namespace mu {
    namespace mem {

        // The global static main page item.
        page page::main;

        void page::__request_page_from_system() {
            __page_info * _pi = (__page_info *)malloc(sizeof(__page_info) * 1);
            _pi->p = (page_t)malloc(page_size);
            if ( _pi->p == NULL ) {
                __throw_bad_alloc();
            }
            _pi->n = NULL;
            for ( size_t i = 0; i < piece_count; ++i ) {
                fpi_.emplace( std::forward<piece_t>(
                    (piece_t)((char *)_pi->p + (i * piece_size))) );
            }
            if ( pgi_header_ == NULL ) pgi_header_ = _pi;
            if ( pgi_tail_ == NULL ) pgi_tail_ = _pi;
            if ( pgi_tail_ != _pi ) pgi_tail_->n = _pi;
        }

        page::page() : pgi_header_(NULL), pgi_tail_(NULL) { }
        page::~page() {
            while ( pgi_header_ != NULL ) {
                free(pgi_header_->p);
                __page_info *_t = pgi_header_->n;
                free(pgi_header_);
                pgi_header_ = _t;
            }
        }

        // Get a piece from the page
        page::piece_type page::get_piece() {
            if ( this->fpi_.size() == 0 ) {
                this->__request_page_from_system();
            }
            piece_t _p = 0;
            this->fpi_.pop_and_fetch(_p);
            return (piece_type)_p;
        }

        // Release a page
        void page::release_piece(page::piece_type p) {
            this->fpi_.emplace(std::forward<piece_t>((piece_t)p));
        }
    }
}

// Push Chen
