// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>
#include <rsutils/exceptions.h>

#include <string>
#include <iosfwd>


namespace rsutils {
namespace type {


// An IP address, version 4.
// 
// 4 decimals separated by a period:
//     W.X.Y.Z
//
class ip_address
{
    uint8_t _b[4];

public:
    ip_address() : _b{ 0 } {}
    ip_address( uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4 ) : _b{ b1, b2, b3, b4 } {}
    ip_address( uint8_t const b[4] ) : _b{ b[0], b[1], b[2], b[3] } {}
    ip_address( ip_address const & ) = default;

    explicit ip_address( std::string const & ) noexcept;
    explicit ip_address( std::string const &, _throw_if_not_valid );

    ip_address & operator=( ip_address const & ) = default;

    bool is_valid() const { return _b[0] || _b[1] || _b[2] || _b[3]; }

    bool empty() const { return ! _b[0] && ! _b[1] && ! _b[2] && ! _b[3]; }
    void clear() { _b[0] = _b[1] = _b[2] = _b[3] = 0; }

    std::string to_string() const;

    bool operator==( ip_address const & other ) const
    {
        return _b[0] == other._b[0] && _b[1] == other._b[1] && _b[2] == other._b[2] && _b[3] == other._b[3];
    }
    bool operator!=( ip_address const & other ) const
    {
        return _b[0] != other._b[0] || _b[1] != other._b[1] || _b[2] != other._b[2] || _b[3] != other._b[3];
    }

    void get_components( uint8_t & b0, uint8_t & b1, uint8_t & b2, uint8_t & b3 ) const
    {
        b0 = _b[0];
        b1 = _b[1];
        b2 = _b[2];
        b3 = _b[3];
    }
    void get_components( uint8_t b[4] ) const { get_components( b[0], b[1], b[2], b[3] ); }

private:
    friend std::ostream & operator<<( std::ostream &, ip_address const & );
};


std::ostream & operator<<( std::ostream &, ip_address const & );


// Allow j["key"] = ip_address( "1.2.3.4" );
void to_json( rsutils::json &, const ip_address & );
// Allow j.get< ip_address >();
void from_json( rsutils::json const &, ip_address & );
// See https://github.com/nlohmann/json#arbitrary-types-conversions


}  // namespace type
}  // namespace rsutils
