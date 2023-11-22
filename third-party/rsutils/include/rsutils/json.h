// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <nlohmann/json.hpp>
#include <string>


namespace rsutils {
namespace json {


extern nlohmann::json const null_json;


// Returns true if the json has a certain key.
// Does not check the value at all, so it could be any type or null.
inline bool has( nlohmann::json const & j, std::string const & key )
{
    auto it = j.find( key );
    if( it == j.end() )
        return false;
    return true;
}


// Returns true if the json has a certain key and its value is not null.
// Does not check the value type.
inline bool has_value( nlohmann::json const & j, std::string const & key )
{
    auto it = j.find( key );
    if( it == j.end() || it->is_null() )
        return false;
    return true;
}


// Get the JSON as a value (copy involved); it must exist
template < class T >
T value( nlohmann::json const & j )
{
    return j.get< T >();
}
// Get the JSON as a value, or a default if not there (copy involved)
template < class T >
T value( nlohmann::json const & j, T const & default_value )
{
    if( j.is_null() )
        return default_value;
    return j.get< T >();
}
// Get a JSON string by reference (zero copy); it must be a string or it'll throw
inline std::string const & string_ref( nlohmann::json const & j )
{
    return j.get_ref< const nlohmann::json::string_t & >();
}


// If there, gets the value at the given key and returns true; otherwise false.
// Turns json exceptions into runtime errors with additional info.
template< class T >
bool get_ex( nlohmann::json const & j, std::string const & key, T * pv )
{
    auto it = j.find( key );
    if( it == j.end() || it->is_null() )
        return false;
    try
    {
        // This will throw for type mismatches, etc.
        it->get_to( *pv );
    }
    catch( nlohmann::json::exception & e )
    {
        throw std::runtime_error( "[while getting '" + key + "']" + e.what() );
    }
    return true;
}


// If there, returns the value at the given key; otherwise returns a default value.
template < class T >
T get( nlohmann::json const & j, std::string const & key, T const & default_value )
{
    if( ! j.is_object() )
        return default_value;
    return j.value( key, default_value );
}


// If there, returns the value at the given key; otherwise throws!
// Turns json exceptions into runtime errors with additional info.
template < class T >
T get( nlohmann::json const & j, std::string const & key )
{
    // This will throw for type mismatches, etc.
    // Does not check for existence: will throw, too!
    return j.at(key).get< T >();
}


// If there, returns the value at the given index (in an array); otherwise throws!
// Turns json exceptions into runtime errors with additional info.
template < class T >
T get( nlohmann::json const & j, int index )
{
    // This will throw for type mismatches, etc.
    // Does not check for existence: will throw, too!
    return j.at( index ).get< T >();
}


// If there, returns the value at the given iterator; otherwise throws!
// Turns json exceptions into runtime errors with additional info.
template < class T >
T get( nlohmann::json const & j, nlohmann::json::const_iterator const & it )
{
    if( it == j.end() )
        throw std::runtime_error( "unexpected end of json" );
    // This will throw for type mismatches, etc.
    // Does not check for existence: will throw, too!
    return it->get< T >();
}


template< typename... Rest >
nlohmann::json const * _nested( nlohmann::json const & j )
{
    return &j;
}
template< typename... Rest >
nlohmann::json const * _nested( nlohmann::json const & j, std::string const & inner, Rest... rest )
{
    auto it = j.find( inner );
    if( it == j.end() )
        return nullptr;
    return _nested( *it, std::forward< Rest >( rest )... );
}


// Allow easy read-only lookup of nested json hierarchies:
//      j["one"]["two"]["three"]            // undefined; will throw/assert if a hierarchy isn't there
// But:
//      nested( j, "one", "two", "three" )  // will not throw; does not copy!
// The result is either a null JSON object or a valid one. The boolean operator can be used as an easy check:
//      if( auto inside = nested( j, "one", "two" ) )
//          { ... }
//
class nested
{
    nlohmann::json const * _pj;

public:
    template< typename... Rest >
    nested( nlohmann::json const & j, Rest... rest )
        : _pj( _nested( j, std::forward< Rest >( rest )... ) )
    {}

    nlohmann::json const * operator->() const { return _pj; }

    bool exists() const { return _pj; }
    operator bool() const { return exists(); }

    nlohmann::json const & get() const { return _pj ? *_pj : null_json; }
    operator nlohmann::json const & () const { return get(); }

    // Get the JSON as a value
    template< class T > T value() { return json::value< T >( get() ); }
    // Get the JSON as a value, or a default if not there
    template < class T > T value( T const & default_value ) { return json::value< T >( get(), default_value ); }
    // Get a JSON string by reference (zero copy); it must be a string or it'll throw
    inline std::string const & string_ref() const { return json::string_ref( get() ); }
};


}  // namespace json
}  // namespace rsutils
