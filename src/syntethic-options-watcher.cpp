// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include <src/syntethic-options-watcher.h>
#include <src/sensor.h>


namespace librealsense {


syntethic_options_watcher::syntethic_options_watcher( const std::shared_ptr< raw_sensor_base > & raw_sensor )
    : _raw_sensor( raw_sensor )
{
}

std::map< rs2_option, std::shared_ptr< option > > syntethic_options_watcher::update_options()
{
    std::map< rs2_option, std::shared_ptr< option > > updated_options;

    std::shared_ptr< raw_sensor_base > strong = _raw_sensor.lock();
    if( ! strong )
        return updated_options;

    strong->prepare_for_bulk_operation();
    updated_options = options_watcher::update_options();
    strong->finished_bulk_operation();

    return updated_options;
}

}  // namespace librealsense
