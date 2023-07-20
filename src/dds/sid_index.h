// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace librealsense {


struct sid_index
{
    int sid;    // Stream ID; assigned based on an atomic counter
    int index;  // Used to distinguish similar streams like IR L / R, 0 otherwise

    sid_index( int sid_, int index_ )
        : sid( sid_ )
        , index( index_ )
    {
    }

    sid_index() = default;
    sid_index( sid_index const & ) = default;
    sid_index( sid_index && ) = default;

    std::string to_string() const { return '(' + std::to_string( sid ) + '.' + std::to_string( index ) + ')'; }

    inline bool operator<( sid_index const & r ) const
    {
        return this->sid < r.sid || this->sid == r.sid && this->index < r.index;
    }
};


}
