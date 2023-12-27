// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <nlohmann/json_fwd.hpp>


namespace rsutils {


using json_key = std::string;  // default of basic_json


class json_ref;   // Our 'nested' json const reference wrapper
class json_base;  // The 'json' base class


// We use a custom base class to inject our own functionality into every 'json' object:
//     json <- nlohmann::basic_json<...> <- json_base
// 
// NOTE: everything here is templated or forward-declared: neither 'json' nor 'json_ref' are defined yet, and 'json'
// can't even be referred to because there's a circular dependency (it derives from json_base).
// 
// This class defines new functionality that the basic 'json' doesn't have that we want (like nested()), but the actual
// implementations are always in 'json_ref'!
// 
// WARNING: we cannot override already-existing 'json' functions! In fact, it's the other way around: 'json' functions
// will override ours, so, for example, if we defined find() it would not be usable.
//
class json_base
{
    // Since we know how we're derived from, we can get the json, or the reference to it, from this:
    inline json_ref _ref() const;

public:
    inline bool exists() const;

    template< class T > inline T default_value( T const & default_value ) const;

    template< class T > inline bool get_ex( T & value ) const;

    inline json_ref default_object() const;
    inline json_ref default_string() const;

    inline std::string const & string_ref() const;
    inline std::string const & string_ref_or_empty() const;

    template< typename... Rest > inline json_ref nested( Rest... rest ) const;

    // Recursively patches with contents of 'overrides', which must be a JSON object
    void override( json_ref overrides, std::string const & what = {} );

};


using json = nlohmann::basic_json< std::map,  // all template arguments are defaults
                                   std::vector,
                                   json_key,
                                   bool,
                                   std::int64_t,
                                   std::uint64_t,
                                   double,
                                   std::allocator,
                                   nlohmann::adl_serializer,
                                   std::vector< std::uint8_t >,
                                   json_base >;  // except custom base class!


// We can't put these inside json, unfortunately...
extern json const null_json;     // default json state
extern json const missing_json;  // i.e., not there: exists() will be 'false'
extern json const empty_json_string;
extern json const empty_json_object;


}  // namespace rsutils
