// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <map>
#include <memory>


namespace librealsense {

using feature_id = std::string;

// Base class for all features
class feature_interface
{
public:
    feature_interface( feature_id id ) : _id( id )
    {
    }

    feature_id get_id() const { return _id; }

    virtual ~feature_interface() = default;

private:
    std::string _id;
};


}  // namespace librealsense
