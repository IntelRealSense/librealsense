// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <librealsense2/utilities/json.h>

#include <unordered_map>

namespace realdds {
    class dds_device_server;
}

namespace tools {

// This class is in charge of handling a RS device: streaming, control..
class lrs_device_controller
{
public:
    lrs_device_controller( rs2::device dev, std::shared_ptr< realdds::dds_device_server > dds_device_server );
    ~lrs_device_controller();
    void start_streaming( const nlohmann::json & msg );
    void stop_streaming( const nlohmann::json & msg );

private:
    bool find_sensor( const std::string & requested_stream_name, size_t & sensor_index );

    rs2::device _rs_dev;
    std::string _device_sn;
    std::vector< rs2::sensor > _sensors;
    std::shared_ptr< realdds::dds_device_server > _dds_device_server;
};  // class lrs_device_controller

}  // namespace tools
