// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <core/features/feature-interface.h>

#include <string>


namespace librealsense {

class emitter_frequency_feature : public feature_interface
{
public:
    emitter_frequency_feature() : feature_interface( "Emitter frequency feature" )
    {
    }
};

}  // namespace librealsense
