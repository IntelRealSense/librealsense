// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>

#include <functional>
#include <memory>

namespace librealsense {


// Interface for objects that have features
class has_features_interface
{
public:
    // Returns if a feature is supported by the device. Optional second parameter to get feature object.
    virtual bool supports_feature( feature_id id, std::shared_ptr< feature_interface > * = nullptr ) { return false; }

    // Performs an operation for each of the features. Returns numbers of features it operated on.
    virtual size_t foreach_feature( std::function< void( std::shared_ptr< feature_interface > const & ) > const & ) { return 0; }
};


}  // namespace librealsense
