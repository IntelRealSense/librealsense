// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/feature-interface.h>

#include <string>


namespace librealsense {

class synthetic_sensor;
class option;

class emitter_frequency_feature : public feature_interface
{
public:
    static constexpr const char * ID = "Emitter frequency feature";

    emitter_frequency_feature( synthetic_sensor & raw_sensor );

    private:
    synthetic_sensor & _sensor;
    std::shared_ptr< option > _emitter_freq_option;
};

}  // namespace librealsense
