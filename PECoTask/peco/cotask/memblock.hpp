/*
    memblock.hpp
    PZCLibMemory
    2019-05-04
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PZC_LIB_MEMORY_MEMBLOCK_HPP__
#define PZC_LIB_MEMORY_MEMBLOCK_HPP__

#include <peco/utils/mustd.hpp>

namespace mu {
    namespace mem {

        /*
        A Item Block Node
        We use two point to manage all items in the block.
        +--------------------------+
        |...|top|.<data>..|tail|...|
        +--------------------------+
        item in the block will always be put at the position
        of tail, and be fetched at top.
        */
        template < class item_t, size_t Capacity = 256UL >
        struct item_block__ {
            item_t                              *block;
            item_t                              *top, *tail;
            size_t                              count;
            item_block__<item_t, Capacity>      *next_ptr;
        };

        /*
        Block Manager.
        In the manager, we use to list of item_block__, one is 
        current data list, another is a free block list.
        The block acts just like a queue.
        New item will be append to the last data block's tail position.
        If current data block is full, we will ask to fetch a new
        item_block__ node. A new node can be fetched from the top of
        free node list, or a new one if free node list is empty too.
        If a block is empty, which means all item has been popped out,
        the block will be put at the top of free node list.
        */
        template < 
            class item_t, 
            size_t Capacity = 256UL
        >
        class block {
            // Self type redefined, for more simple code.
            typedef item_block__<item_t, Capacity> item_block_t;

            // Free List Top Node, the Free list is like a stack
            item_block_t         *free_head_;
            // Data Block Pointer, we always fetch item from data_head_
            // and append item to data_tail_
            item_block_t         *data_head_;
            item_block_t         *data_tail_;
            // All item count
            size_t               count_;

            // Create a new item_block__ node
            item_block_t * __new_block() {
                // Create the node struct
                item_block_t *_ptr = (item_block_t *)malloc(sizeof(item_block_t));
                // Init the node's block data memory
                _ptr->block = (item_t*)malloc(sizeof(item_t) * Capacity);
                // The top and tail pointer will be inited to the beginning
                // of the block.
                _ptr->top = _ptr->block;
                _ptr->tail = _ptr->block;
                _ptr->count = 0;
                _ptr->next_ptr = NULL;
                return _ptr;
            }
            // Free a item_block__ node, the node is no longer used in the manager item.
            void __delete_block(item_block_t * ptr) {
                free(ptr->block);
                free(ptr);
            }
            // Try to check the free node list, if it's not empty, return the 
            // top node in the free node list. otherwise, create a new one
            item_block_t * __fetch_block() {
                if ( free_head_ != NULL ) {
                    item_block_t *_ptr = free_head_;
                    // Reset the top of the free node list.
                    free_head_ = free_head_->next_ptr;
                    _ptr->top = _ptr->block;
                    _ptr->tail = _ptr->block;
                    _ptr->count = 0;
                    _ptr->next_ptr = NULL;
                    return _ptr;
                }
                return __new_block();
            }
            // A node block is empty, we put it back to the free node list.
            inline void __hold_block(item_block_t* ptr) {
                ptr->next_ptr = free_head_;
                free_head_ = ptr;
            }
            // if the item count is very small and cannot fill even one block
            // and when after some operations, the only data block become empty,
            // we use this method to rest the status of the header data block
            void __reset_block(item_block_t* ptr) {
                ptr->top = ptr->block;
                ptr->tail = ptr->block;
                ptr->count = 0;
            }
        public:
            // Default C'str, create a new data block
            block() : free_head_(NULL), count_(0) {
                data_head_ = __new_block();
                data_tail_ = data_head_;
            }
            // D'str, release all blocks, both data and free node list.
            ~block() {
                while ( data_head_ != NULL ) {
                    item_block_t * _ptr = data_head_;
                    data_head_ = data_head_->next_ptr;
                    __delete_block(_ptr);
                }
                while ( free_head_ != NULL ) {
                    item_block_t * _ptr = free_head_;
                    free_head_ = free_head_->next_ptr;
                    __delete_block(_ptr);
                }
            }

            // Properties: size
            size_t size() const { return count_; }
            // Properties: tell if current block manager has no data.
            bool empty() const { return count_ == 0; }

            // Append item to the tail block's tail position
            void emplace( item_t&& t ) {
                // Last block is full
                if ( data_tail_->tail == (data_tail_->block + Capacity) ) {
                    item_block_t *_ptr = __fetch_block();
                    data_tail_->next_ptr = _ptr;
                    data_tail_ = _ptr;
                }
                // Set the item to the tail
                new ((void *)data_tail_->tail)item_t(std::forward<item_t>(t));
                ++data_tail_->tail;
                ++data_tail_->count;
                ++count_;
            }
            // Fetch the top item and pop from the block manager
            void pop_and_fetch(item_t& c) {
                assert(count_ != 0);
                c = std::move(*data_head_->top);
                // Destory the cached item
                (*data_head_->top).~item_t();

                ++data_head_->top;
                --data_head_->count;
                --count_;

                // Check if the top block is the only block and is empty
                if ( data_head_->count == 0 ) {
                    if ( data_head_ == data_tail_ ) {
                        __reset_block(data_head_);
                    } else {
                        item_block_t *_ptr = data_head_;
                        data_head_ = data_head_->next_ptr;
                        __hold_block(_ptr);
                    }
                }
            }
        };
    }
}

#endif

// Push Chen
