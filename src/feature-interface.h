// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>


namespace librealsense {

using feature_id = std::string;

// Base class for all features
class feature_interface
{
public:
    virtual feature_id get_id() const = 0;

    virtual ~feature_interface() = default;
};


}  // namespace librealsense
