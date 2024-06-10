// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/option.h>


namespace librealsense {


// This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
// Additionally, it will ensure that any fields not set in a given struct will retain their original values
template<class T, class R, class W> struct struct_interface
{
    T struct_;
    R reader;
    W writer;
    bool active;

    struct_interface( R r, W w ) : reader( r ), writer( w ), active( false ) {}

    void activate() { if( !active ) { struct_ = reader(); active = true; } }
    template<class U> double get( U T:: * field ) { activate(); return static_cast<double>(struct_.*field); }
    template<class U, class V> void set( U T:: * field, V value ) { activate(); struct_.*field = static_cast<U>(value); }
    void commit() { if( active ) writer( struct_ ); }
};

template<class T, class R, class W>
std::shared_ptr<struct_interface<T, R, W>> make_struct_interface( R r, W w )
{
    return std::make_shared<struct_interface<T, R, W>>( r, w );
}


template<class T, class R, class W, class U>
class struct_field_option : public option
{
public:
    void set( float value ) override
    {
        _struct_interface->set( _field, value );
        _recording_function( *this );
    }
    float query() const override
    {
        return _struct_interface->get( _field );
    }
    option_range get_range() const override
    {
        return _range;
    }
    bool is_enabled() const override { return true; }

    explicit struct_field_option( std::shared_ptr<struct_interface<T, R, W>> struct_interface,
                                  U T:: * field, const option_range & range )
        : _struct_interface( struct_interface ), _range( range ), _field( field )
    {
    }

    const char * get_description() const override
    {
        return nullptr;
    }

    void enable_recording( std::function<void( const option & )> record_action ) override
    {
        _recording_function = record_action;
    }
private:
    std::shared_ptr<struct_interface<T, R, W>> _struct_interface;
    option_range _range;
    U T:: * _field;
    std::function<void( const option & )> _recording_function = []( const option & ) {};
};


template<class T, class R, class W, class U>
std::shared_ptr<struct_field_option<T, R, W, U>> make_field_option(
    std::shared_ptr<struct_interface<T, R, W>> struct_interface,
    U T:: * field, const option_range & range )
{
    return std::make_shared<struct_field_option<T, R, W, U>>
        ( struct_interface, field, range );
}


}