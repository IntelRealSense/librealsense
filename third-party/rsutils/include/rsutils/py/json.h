// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

// This code is a copy of the one provided in this header:
//#include <pybind11_json/pybind11_json.hpp>
// Except that it's adapted to rsutils::json, instead.

#include <rsutils/json.h>

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include <string>
#include <vector>


inline py::object json_to_py( const rsutils::json & j )
{
    if( j.is_null() )
        return py::none();
    else if( j.is_boolean() )
        return py::bool_( j.get< bool >() );
    else if( j.is_number_unsigned() )
        return py::int_( j.get< rsutils::json::number_unsigned_t >() );
    else if( j.is_number_integer() )
        return py::int_( j.get< rsutils::json::number_integer_t >() );
    else if( j.is_number_float() )
        return py::float_( j.get< double >() );
    else if( j.is_string() )
        return py::str( j.get< std::string >() );
    else if( j.is_array() )
    {
        py::list obj( j.size() );
        for( std::size_t i = 0; i < j.size(); i++ )
        {
            obj[i] = json_to_py( j[i] );
        }
        return std::move( obj );
    }
    else  // Object
    {
        py::dict obj;
        for( rsutils::json::const_iterator it = j.cbegin(); it != j.cend(); ++it )
        {
            obj[py::str( it.key() )] = json_to_py( it.value() );
        }
        return std::move( obj );
    }
}

inline rsutils::json py_to_json( const py::handle & obj )
{
    if( obj.ptr() == nullptr || obj.is_none() )
        return nullptr;
    
    if( py::isinstance< py::bool_ >( obj ) )
        return obj.cast< bool >();
    
    if( py::isinstance< py::int_ >( obj ) )
    {
        // NOTE: JSON RFC 8259 section 6 describes number formats, without 'signed' vs 'unsigned' distinctions:
        //     https://json.nlohmann.me/features/types/number_handling/
        // When parsing text, Nlohmann's JSON chooses 'unsigned' by default! See lexer.hpp (scan_number()).
        // So we match here (and diverge from the default pybind11_json handling!):
        try
        {
            rsutils::json::number_unsigned_t u = obj.cast< rsutils::json::number_unsigned_t >();
            if( py::int_( u ).equal( obj ) )
                return u;
        }
        catch( ... ) {}
        try
        {
            rsutils::json::number_integer_t s = obj.cast< rsutils::json::number_integer_t >();
            if( py::int_( s ).equal( obj ) )
                return s;
        }
        catch( ... ) {}
        throw std::runtime_error( "py_to_json received out-of-range for both number_integer_t and number_unsigned_t: "
                                  + py::repr( obj ).cast< std::string >() );
    }
    if( py::isinstance< py::float_ >( obj ) )
        return obj.cast< double >();
    
    if( py::isinstance< py::bytes >( obj ) )
    {
        py::module base64 = py::module::import( "base64" );
        return base64.attr( "b64encode" )( obj ).attr( "decode" )( "utf-8" ).cast< std::string >();
    }
    if( py::isinstance< py::str >( obj ) )
        return obj.cast< std::string >();
    
    if( py::isinstance< py::tuple >( obj ) || py::isinstance< py::list >( obj ) )
    {
        auto out = rsutils::json::array();
        for( const py::handle value : obj )
        {
            out.push_back( py_to_json( value ) );
        }
        return out;
    }
    if( py::isinstance< py::dict >( obj ) )
    {
        auto out = rsutils::json::object();
        for( const py::handle key : obj )
        {
            out[py::str( key ).cast< std::string >()] = py_to_json( obj[key] );
        }
        return out;
    }
    throw std::runtime_error( "py_to_json not implemented for this object: " + py::repr( obj ).cast< std::string >() );
}



namespace nlohmann {
#define MAKE_RSJSON_SERIALIZER_DESERIALIZER( T )                                                                       \
    template<>                                                                                                         \
    struct adl_serializer< T >                                                                                         \
    {                                                                                                                  \
        inline static void to_json( rsutils::json & j, const T & obj ) { j = py_to_json( obj ); }                      \
                                                                                                                       \
        inline static T from_json( const rsutils::json & j ) { return json_to_py( j ); }                               \
    }

#define MAKE_RSJSON_SERIALIZER_ONLY( T )                                                                               \
    template<>                                                                                                         \
    struct adl_serializer< T >                                                                                         \
    {                                                                                                                  \
        inline static void to_json( rsutils::json & j, const T & obj ) { j = py_to_json( obj ); }                      \
    }

MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::object );

MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::bool_ );
MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::int_ );
MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::float_ );
MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::str );

MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::list );
MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::tuple );
MAKE_RSJSON_SERIALIZER_DESERIALIZER( py::dict );

MAKE_RSJSON_SERIALIZER_ONLY( py::handle );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::item_accessor );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::list_accessor );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::tuple_accessor );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::sequence_accessor );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::str_attr_accessor );
MAKE_RSJSON_SERIALIZER_ONLY( py::detail::obj_attr_accessor );

#undef MAKE_RSJSON_SERIALIZER_DESERIALIZER
#undef MAKE_RSJSON_SERIALIZER_ONLY
}  // namespace nlohmann

namespace pybind11 {
namespace detail {

template<>
struct type_caster< rsutils::json >
{
public:
    PYBIND11_TYPE_CASTER( rsutils::json, _( "json" ) );

    bool load( handle src, bool )
    {
        try
        {
            value = py_to_json( src );
            return true;
        }
        catch( ... )
        {
            return false;
        }
    }

    static handle cast( rsutils::json src, return_value_policy /* policy */, handle /* parent */ )
    {
        object obj = json_to_py( src );
        return obj.release();
    }
};

}  // namespace detail
}  // namespace pybind11
