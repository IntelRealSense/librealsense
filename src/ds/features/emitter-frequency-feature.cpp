// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/ds/features/emitter-frequency-feature.h>
#include <src/platform/uvc-option.h>
#include <src/ds/ds-private.h>
#include <src/sensor.h>


namespace librealsense {

/* static */ const feature_id emitter_frequency_feature::ID = "Emitter frequency feature";

emitter_frequency_feature::emitter_frequency_feature( synthetic_sensor & sensor )
{
    auto emitter_freq_option = std::make_shared< uvc_xu_option< uint16_t > >(
        std::dynamic_pointer_cast< uvc_sensor >( sensor.get_raw_sensor() ),
        ds::depth_xu,
        ds::DS5_EMITTER_FREQUENCY,
        "Controls the emitter frequency, 57 [KHZ] / 91 [KHZ]",
        std::map< float, std::string >{ { (float)RS2_EMITTER_FREQUENCY_57_KHZ, "57 KHZ" },
                                        { (float)RS2_EMITTER_FREQUENCY_91_KHZ, "91 KHZ" } },
        false );
 
    sensor.register_option( RS2_OPTION_EMITTER_FREQUENCY, emitter_freq_option );
}

feature_id emitter_frequency_feature::get_id() const
{
    return ID;
}

}  // namespace librealsense
