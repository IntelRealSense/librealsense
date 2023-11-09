// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>

#include <string>


namespace librealsense {

class emitter_frequency_feature : public feature_interface
{
public:
    static constexpr char * ID = "Emitter frequency feature";

    emitter_frequency_feature() : feature_interface( ID )
    {
    }
};

}  // namespace librealsense
