// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace librealsense {

class feature_interface
{
public:
    feature_interface( const std::string& id ) : _id( id )
    {
    }

    const std::string & get_id() const { return _id; }

    virtual ~feature_interface() = default;

private:
    std::string _id;
};

}  // namespace librealsense
