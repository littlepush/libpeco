/*
    eventtrigger.h
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

#ifndef PE_CO_UTILS_EVENT_TRIGGER_H__
#define PE_CO_UTILS_EVENT_TRIGGER_H__

#include <peco/utils/mustd.hpp>

namespace pe { namespace utils {

    template < typename storage_t > 
    class eventtrigger {
        // Typename rewrite
        typedef storage_t                           storage_type;
        typedef typename storage_t::key_type        key_type;
        typedef typename storage_t::value_type      value_type;
        typedef typename storage_t::iterator        iterator;
        typedef typename storage_t::const_iterator  const_iterator;

        typedef std::pair< bool, value_type >       value_change_status_t;
        // Self Type
        typedef eventtrigger< storage_t >           self_t;

        // The basic handler
        typedef std::function< 
            void ( const value_type&, const value_type&) 
        > trigger_handler_t;

        // Trigger Events
        typedef enum {
            ET_TRIGGER_CHANGED,
            ET_TRIGGER_CHANGED_TO,
            ET_TRIGGER_CHANGED_FROM,
            ET_TRIGGER_CHANGED_EXCEPT,
            ET_TRIGGER_CHANGED_SPECIAL
        } ET_TRIGGER_TYPE;

        // Signel trigger operator object
        typedef struct {
            ET_TRIGGER_TYPE             type;
            std::vector< value_type >   values;
            trigger_handler_t           handler;
        } trigger_operator_t;

        typedef std::list< trigger_operator_t >             trigger_op_list_t;
        typedef typename trigger_op_list_t::iterator        trigger_iterator;
        typedef std::map< key_type, trigger_op_list_t >     trigger_infomap_t;

        typedef std::map< key_type, value_type >            kv_map_t;

        // Storage Operators
        typedef std::function< void ( storage_t& ) >        storage_init_t;
        typedef std::function< void ( storage_t& ) >        storage_close_t;
        typedef std::function< bool ( const_iterator&, const key_type&) >   search_pred_t;
    protected: 
        storage_t                       info_data_;
        trigger_infomap_t               trigger_map_;
        storage_close_t                 info_close_;
    public: 
        // Create a event trigger with specifial storage
        eventtrigger<storage_t>( storage_init_t si, storage_close_t sc )
            : info_close_(sc) { si(info_data_); }
        
        // NO Copy & Move
        eventtrigger<storage_t>( const self_t& ) = delete;
        eventtrigger<storage_t>( self_t&& ) = delete;
        self_t& operator = (const self_t&) = delete;
        self_t& operator = (self_t&&) = delete;

        // Set And Get Value
        void set_value(const key_type& k, const value_type& v ) {
            value_change_status_t _cs = info_data_.set(k, v);
            // _cs.first means if the given value is equal to the old value
            // if the value not change, do nothing
            if ( _cs.first == false ) return;

            auto _opit = trigger_map_.find(k);
            // No trigger registed on this key
            if ( _opit == trigger_map_.end() ) return;
            // Loop for all op
            for ( auto& op : _opit->second ) {
                switch ( op.type ) {
                // If the value has been changed, do this
                case ET_TRIGGER_CHANGED : {
                    op.handler(_cs.second, v); 
                    break;
                }
                // If the value has been changed to specifial value
                case ET_TRIGGER_CHANGED_TO : {
                    if ( v == op.values[0] ) op.handler(_cs.second, v);
                    break;
                }
                // If the old value matches the case
                case ET_TRIGGER_CHANGED_FROM : {
                    if ( _cs.second == op.values[0] ) op.handler(_cs.second, v);
                    break;
                }
                // If the the value is not in the except list
                case ET_TRIGGER_CHANGED_EXCEPT : {
                    if ( std::find(
                        op.values.begin(),
                        op.values.end(),
                        v
                    ) == op.values.end() ) op.handler(_cs.second, v);
                    break;
                }
                // Specifial from old value to new value
                case ET_TRIGGER_CHANGED_SPECIAL : {
                    if ( op.values[0] == _cs.second && op.values[1] == v ) 
                        op.handler(_cs.second, v);
                    break;
                }
                default : break;
                }
            }
        }
        void remove_value(const key_type& k) { info_data_.remove(k); }
        const value_type& get_value(const key_type& k) const { return info_data_.get(k); }
        bool has( const key_type& k ) const { return info_data_.has(k); }

        // Iterator
        iterator begin() { return info_data_.begin(); }
        iterator end() { return info_data_.end(); }
        const_iterator cbegin() { return info_data_.cbegin(); }
        const_iterator cend() { return info_data_.cend(); }
        const key_type& key( const_iterator& cit ) { return info_data_.key(cit); } 
        const value_type& value( const_iterator& cit ) { return info_data_.value(cit); }

        // Search with given keyword
        kv_map_t search( const string& keyword, search_pred_t pred ) {
            kv_map_t _r;
            for ( 
                const_iterator _it = info_data_.cbegin(); 
                _it != info_data_.cend(); 
                ++_it 
            ) {
                if ( pred(_it, keyword) ) {
                    _r[info_data_.key(_it)] = info_data_.value(_it);
                }
            }
            return _r;
        }
        
        // Trigger Manage
        void add_trigger( const key_type& key, trigger_operator_t&& t_op ) {
            auto _tit = trigger_map_.find(key);
            if ( _tit == trigger_map_.end() ) {
                trigger_map_[key] = (trigger_op_list_t){ std::move(t_op) };
            } else {
                _tit->second.emplace_back(std::move(t_op));
            }
        }
        void add_trigger(
            const key_type& key, ET_TRIGGER_TYPE tt, trigger_handler_t h
        ) {
            trigger_operator_t _t_op = {tt, {}, h};
            add_trigger( key, std::move(_t_op) );
        }
        template< typename... value_t >
        void add_trigger( 
            const key_type& key, ET_TRIGGER_TYPE tt, trigger_handler_t h, const value_t&... values 
        ) {
            trigger_operator_t _t_op = {tt, {values...}, h};
            add_trigger( key, std::move(_t_op) );
        }

        void add_trigger_changed(
            const key_type& key, trigger_handler_t h
        ) { add_trigger(key, ET_TRIGGER_CHANGED, h); }
        void add_trigger_from(
            const key_type& key, const value_type& from, trigger_handler_t h
        ) { add_trigger(key, ET_TRIGGER_CHANGED_FROM, h, from); }
        void add_trigger_to(
            const key_type& key, const value_type& to, trigger_handler_t h
        ) { add_trigger(key, ET_TRIGGER_CHANGED_TO, h, to); }
        template< typename... value_t >
        void add_trigger_except(
            const key_type& key, trigger_handler_t h, const value_t&... values
        ) { add_trigger(key, ET_TRIGGER_CHANGED_EXCEPT, h, values...); }
        void add_trigger_special(
            const key_type& key, const value_type& from, const value_type& to, trigger_handler_t h
        ) { add_trigger(key, ET_TRIGGER_CHANGED_SPECIAL, from, to); }

        void remove_trigger(
            const key_type& key, std::function< bool ( const trigger_operator_t& ) > pred
        ) {
            auto _tit = trigger_map_.find(key);
            if ( _tit == trigger_map_.end() ) return;
            auto _hit = _tit->second.begin();
            while ( _hit != _tit->second.end() ) {
                if ( pred(*_hit) ) {
                    _hit = _tit->second.erase(_hit);
                } else {
                    ++_hit;
                }
            }
        }

        void remove_trigger(
            const key_type& key, ET_TRIGGER_TYPE tt
        ) {
            remove_trigger(key, [tt](const trigger_operator_t& t) {
                return t.type == tt;
            });
        }

        void remove_trigger( const key_type& key ) {
            trigger_map_.erase(key);
        }
    };


    template < typename key_type_t, typename value_type_t >
    class et_memory_storage {
    public: 
        typedef key_type_t                                  key_type;
        typedef value_type_t                                value_type;
        typedef std::map< key_type_t, value_type_t >        storage_type_t;
        typedef typename storage_type_t::iterator           iterator;
        typedef typename storage_type_t::const_iterator     const_iterator;
        typedef std::pair< bool, value_type_t >             change_statue_t;
    protected:
        storage_type_t              data_;
    public: 

        change_statue_t set( const key_type_t& k, const value_type_t& v ) {
            static value_type_t dummy_value;
            auto _it = data_.find(k);
            if ( _it == data_.end() ) {
                data_[k] = v;
                return std::make_pair(true, dummy_value);
            } else {
                if ( _it->second == v ) {
                    return std::make_pair(false, dummy_value);
                } else {
                    auto _r(std::make_pair(true, _it->second));
                    _it->second = v;
                    return _r;
                }
            }
        }
        void remove( const key_type_t& k ) { data_.erase(k); }
        bool has( const key_type_t& k ) { return data_.find(k) != data_.end(); }
        const value_type_t& get( const key_type_t& k ) { 
            static value_type_t _dummy_empty_value;
            if ( !has(k) ) return _dummy_empty_value;
            return data_[k]; 
        }

        // Iterator
        iterator begin() { return data_.begin(); }
        iterator end() { return data_.end(); }
        const_iterator cbegin() const { return data_.cbegin(); }
        const_iterator cend() const { return data_.cend(); }
        const key_type_t& key( const_iterator& it ) { return it->first; }
        const value_type_t& value( const_iterator& it ) { return it->second; }
    };

    template< typename key_type_t, typename value_type_t >
    void et_memory_storage_create( et_memory_storage<key_type_t, value_type_t>& ) { }

    template< typename key_type_t, typename value_type_t >
    void et_memory_storage_destory( et_memory_storage<key_type_t, value_type_t>& ) { }
}}

#endif

// Push Chen
