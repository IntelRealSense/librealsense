// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>

#include <string>
#include <iosfwd>


namespace rsutils {
namespace string {


// An IP address, version 4.
// 
// 4 decimals separated by a period:
//     W.X.Y.Z
//
class ip_address
{
    std::string _str;

public:
    ip_address() = default;
    ip_address( ip_address const & ) = default;
    ip_address( ip_address && ) = default;
    explicit ip_address( std::string );

    ip_address & operator=( ip_address const & ) = default;
    ip_address & operator=( ip_address && ) = default;

    bool is_valid() const { return ! empty(); }

    bool empty() const { return _str.empty(); }
    void clear() { _str.clear(); }

    std::string to_string() const { return _str; }

private:
    friend std::ostream & operator<<( std::ostream &, ip_address const & );
};


inline std::ostream & operator<<( std::ostream & os, ip_address const & ip )
{
    return operator<<( os, ip._str );
}


// Allow j["key"] = ip_address( "1.2.3.4" );
void to_json( rsutils::json &, const ip_address & );
// Allow j.get< ip_address >();
void from_json( rsutils::json const &, ip_address & );
// See https://github.com/nlohmann/json#arbitrary-types-conversions


}  // namespace string
}  // namespace rsutils
