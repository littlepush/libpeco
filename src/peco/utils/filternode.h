/*
    filter_node.h
    PECoUtils
    2019-05-22
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

#ifndef PE_CO_UTILS_FILTER_NODE_H__
#define PE_CO_UTILS_FILTER_NODE_H__

#include <peco/utils/mustd.hpp>

namespace pe { namespace utils {
    template < typename match_type_t, typename value_type_t >
    class filter_node
    {
    public: 
        typedef match_type_t                            key_type;
        typedef value_type_t                            value_type;

        typedef filter_node<match_type_t, value_type_t> node_t;
        typedef std::shared_ptr< node_t >               shared_node_t;
        typedef std::map< match_type_t, shared_node_t > filter_tree_t;
    protected:
        bool                                is_root_;
        match_type_t                        match_obj_;
        value_type_t                        value_obj_;
        bool                                tflag_;
        filter_tree_t                       sub_filter_tree_;
    protected: 

        // Internal C'str
        filter_node<match_type_t, value_type_t>()
            : is_root_(true), tflag_(false) { }
        filter_node<match_type_t, value_type_t>( const match_type_t& m ) 
            : is_root_(false), match_obj_(m), tflag_(false) { }
        filter_node<match_type_t, value_type_t>( const match_type_t& m, const value_type_t& v )
            : is_root_(false), match_obj_(m), value_obj_(v), tflag_(false) { }

        // No Copy
        filter_node<match_type_t, value_type_t>( const node_t& ) = delete;
        filter_node<match_type_t, value_type_t>( node_t&& ) = delete;
        node_t& operator = ( const node_t& ) = delete;
        node_t& operator = ( node_t&& ) = delete;

        template< typename match_iterator_t >
        static list< shared_node_t > is_match_( 
            shared_node_t root_node, 
            match_iterator_t begin, 
            match_iterator_t end
        ) {
            list< shared_node_t > _stack;
            if ( root_node->is_root_node() == false ) return _stack;
            shared_node_t _ln = nullptr, _lln = root_node;
            for ( auto i = begin; i != end; ++i ) {
                _ln = _lln->matching_sub_node(*i);
                if ( _ln == nullptr ) {
                    _stack.clear(); return _stack;
                }
                _stack.push_back(_ln);
                if ( _ln->is_terminated() ) return _stack;
                _lln = _ln;
            }
            _stack.clear();
            return _stack;
        }
        shared_node_t _create_node(const match_type_t& m) { return shared_node_t( new node_t(m) ); }
    public: 

        // Create a new filter node
        static shared_node_t create_root_node() { return shared_node_t(new node_t); }
        static shared_node_t create_node(const match_type_t& m, const value_type_t& v ) { 
            return std::make_shared< node_t >(m, v);
        }

        // Compare Operations
        bool operator == ( const node_t& ref ) { 
            if ( this == &ref ) return true; 
            return ( is_root_ ? false : (match_obj_ == ref.match_obj_) ); 
        }
        bool operator != ( const node_t& ref ) { 
            return ( is_root_ ? false : (!(*this == ref)) ); 
        }
        bool operator < ( const node_t& ref ) { 
            return ( is_root_ ? false : (match_obj_ < ref.match_obj_) ); 
        }
        // compare with only match object
        bool operator == ( const match_type_t& mt ) { 
            return ( is_root_ ? false : (match_obj_ == mt) ); 
        }
        bool operator != ( const match_type_t& mt ) { 
            return ( is_root_ ? false : (match_obj_ != mt) ); 
        }
        bool operator < ( const match_type_t& mt ) { 
            return ( is_root_ ? false : (match_obj_ < mt) ); 
        }

        // Properties
        bool is_root_node() const { return is_root_; }
        const match_type_t& match_obj() const { return match_obj_; }
        value_type_t& value_obj() { return value_obj_; }
        const value_type_t& value_obj() const { return value_obj_; }
        void set_value_obj(const value_type_t& v) { value_obj_ = v; }
        void emplace_value_obj(value_type_t&& v) { value_obj_ = std::move(v); }

        // Sub Nodes
        shared_node_t add_sub_node( const match_type_t& mt ) {
            auto _i = sub_filter_tree_.find(mt);
            if ( _i == sub_filter_tree_.end() ) {
                // New sub node
                shared_node_t _n = this->_create_node(mt);
                sub_filter_tree_[mt] = _n;
                return _n;
            } else {
                return _i->second;
            }
        }

        // Terminate
        void set_terminate_flag() { tflag_ = true; }
        void clear_terminate_flag() { tflag_ = false; }
        bool is_terminated() const { return tflag_; }
        bool contains_sub_fitler_tree() const {
            return sub_filter_tree_.size() > 0;
        }

        // Search a match subnode
        shared_node_t matching_sub_node(const match_type_t& m) const {
            auto _i = sub_filter_tree_.find(m);
            if ( _i == sub_filter_tree_.end() ) return nullptr;
            return _i->second;
        }

        // Operator for root
        template< typename match_iterator_t >
        static shared_node_t is_match( 
            shared_node_t root_node, 
            match_iterator_t begin, 
            match_iterator_t end 
        ) {
            auto _stack = is_match_(root_node, begin, end);
            if ( _stack.size() == 0 ) return nullptr;
            return _stack.back();
        }

        template< typename filter_iterator_t >
        static shared_node_t create_filter_tree( 
            shared_node_t root_node, 
            filter_iterator_t begin, 
            filter_iterator_t end,
            const value_type_t& v
        ) {
            if ( root_node->is_root_node() == false ) return nullptr;
            shared_node_t _ln = nullptr, _lln = root_node;
            for ( auto i = begin; i != end; ++i ) {
                _ln = _lln->add_sub_node(*i);
                _lln = _ln;
            }
            _lln->set_terminate_flag();
            _lln->set_value_obj(v);
            return _lln;
        }

        string to_string() const {
            string _value = (is_root_ ? "(R:<" : "(N:<");
            _value += std::to_string(match_obj_);
            _value += ">:";
            _value += (tflag_ ? "T:" : "-:");
            _value += std::to_string(sub_filter_tree_.size());
            _value += ")";
            return _value;
        }

        template< typename filter_iterator_t >
        static void remove_filter_tree( 
            shared_node_t root_node, 
            filter_iterator_t begin, 
            filter_iterator_t end 
        ) {
            if ( begin == end ) return; 
            auto _stack = node_t::is_match_(root_node, begin, end);
            if ( _stack.size() == 0 ) return;

            if ( _stack.back()->contains_sub_fitler_tree() ) {
                _stack.back()->clear_terminate_flag(); return;
            }
            for ( auto i = (++_stack.rbegin()), l = _stack.rbegin(); i != _stack.rend(); ++i, ++l ) {
                if ( ((*i)->sub_filter_tree_.size() > 1) || (*i)->is_terminated() ) return;
                (*i)->sub_filter_tree_.erase((*l)->match_obj_);
            }
            if ( (*_stack.begin())->sub_filter_tree_.size() > 1 || (*_stack.begin())->is_terminated() ) return;
            root_node->sub_filter_tree_.erase((*_stack.begin())->match_obj_);
        }
    };
}}

#endif 

// Push Chen
