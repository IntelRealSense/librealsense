// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace librealsense {

class feature_interface
{
public:
    feature_interface( const std::string& name ) : _name( name )
    {
    }

    const std::string & get_name() const { return _name; }

    virtual ~feature_interface() = default;

private:
    std::string _name;
};

}  // namespace librealsense
