// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>
#include <string>


namespace rsutils {
namespace json {


extern nlohmann::json const null_json;
extern nlohmann::json const empty_json_string;
extern nlohmann::json const empty_json_object;


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
nlohmann::json const & _nested( nlohmann::json const & j )
{
    return j;
}
template< typename... Rest >
nlohmann::json const & _nested( nlohmann::json const & j, std::string const & inner, Rest... rest )
{
    auto it = j.find( inner );
    if( it == j.end() )
        return null_json;
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
    nlohmann::json const & _j;

public:
    nested() : _j( null_json ) {}

    template< typename... Rest >
    nested( nlohmann::json const & j, Rest... rest )
        : _j( _nested( j, std::forward< Rest >( rest )... ) )
    {}

    nlohmann::json const * operator->() const { return &_j; }

    bool exists() const { return ! _j.is_null(); }
    operator bool() const { return exists(); }

    nlohmann::json const & get() const { return _j; }
    operator nlohmann::json const & () const { return get(); }

    bool is_array() const { return _j.is_array(); }
    bool is_object() const { return _j.is_object(); }
    bool is_string() const { return _j.is_string(); }

    // Dig deeper
    template< typename... Rest >
    inline nested find( Rest... rest ) const
    {
        return nested( _j, std::forward< Rest >( rest )... );
    }
    inline nested operator[]( std::string const & key ) const { return find( key ); }

    // Get the JSON as a value
    template< class T > T value() const { return json::value< T >( get() ); }
    // Get the JSON as a value, or a default if not there (throws if wrong type)
    template < class T > T default_value( T const & default_value ) const { return json::value< T >( get(), default_value ); }
    // Get the object, with a default being an empty one; does not throw
    nlohmann::json const & default_object() const { return is_object() ? _j : empty_json_object; }
    // Get the object, with a default being an empty one; does not throw
    nlohmann::json const & default_string() const { return is_string() ? _j : empty_json_string; }
    // Get a JSON string by reference (zero copy); it must be a string or it'll throw
    inline std::string const & string_ref() const { return json::string_ref( get() ); }
    // Get a JSON string by reference (zero copy); does not throw
    inline std::string const & string_ref_or_empty() const { return json::string_ref( default_string() ); }
};


// Recursively patches existing 'j' with contents of 'patches', which must be a JSON object.
// A 'null' value inside erases previous contents. Any other value overrides.
// See: https://json.nlohmann.me/api/basic_json/merge_patch/
// Example below, for load_app_settings.
// Use 'what' to denote what it is we're patching in, if a failure happens. The std::runtime_error will populate with
// it.
//
void patch( nlohmann::json & j, nlohmann::json const & patches, std::string const & what = {} );


// Loads configuration settings from 'global' content.
// E.g., a configuration file may contain:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         ...
//     }
// This function will load a specific key 'context' inside and return it. The result will be a disabling of dds:
// Besides this "global" key, application-specific settings can override the global settings, e.g.:
//     {
//         "context": {
//             "dds": {
//                 "enabled": false,
//                 "domain" : 5
//             }
//         },
//         "realsense-viewer": {
//             "context": {
//                 "dds": { "enabled": null }
//             }
//         },
//         ...
//     }
// If the current application is 'realsense-viewer', then the global 'context' settings will be patched with the
// application-specific 'context' and returned:
//     {
//         "dds": {
//             "domain" : 5
//         }
//     }
// See rules for patching in patch().
// The 'application' is usually any single-word executable name (without extension).
// The 'subkey' is mandatory.
// The 'error_context' is used for error reporting, to show what failed. Like application, it should be a single word
// that can be used to denote hierarchy within the global json.
//
nlohmann::json load_app_settings( nlohmann::json const & global,
                                  std::string const & application,
                                  std::string const & subkey,
                                  std::string const & error_context );


// Same as above, but automatically takes the application name from the executable-name.
//
nlohmann::json load_settings( nlohmann::json const & global,
                              std::string const & subkey,
                              std::string const & error_context );


}  // namespace json
}  // namespace rsutils
