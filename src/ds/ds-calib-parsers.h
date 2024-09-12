// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds-device-common.h"
#include "core/video.h"
#include <rsutils/lazy.h>


namespace librealsense
{
    class mm_calib_parser
    {
    public:
        virtual rs2_extrinsics get_extrinsic_to(rs2_stream) = 0;    // Extrinsics are referenced to the Depth stream, except for TM1
        virtual ds::imu_intrinsic get_intrinsic(rs2_stream) = 0;    // With extrinsic from FE<->IMU only
        virtual float3x3 imu_to_depth_alignment() = 0;
    };

    class mm_calib_handler
    {
    public:
        mm_calib_handler(std::shared_ptr<hw_monitor> hw_monitor, uint16_t pid);
        ~mm_calib_handler() {}

        ds::imu_intrinsic get_intrinsic(rs2_stream);
        rs2_extrinsics get_extrinsic(rs2_stream);       // The extrinsic defined as Depth->Stream rigid-body transfom.
        const std::vector<uint8_t> get_fisheye_calib_raw();
        float3x3 imu_to_depth_alignment() { return (*_calib_parser)->imu_to_depth_alignment(); }
    private:
        std::shared_ptr<hw_monitor> _hw_monitor;
        rsutils::lazy< std::shared_ptr< mm_calib_parser > > _calib_parser;
        rsutils::lazy< std::vector< uint8_t > > _imu_eeprom_raw;
        std::vector<uint8_t>            get_imu_eeprom_raw() const;
        rsutils::lazy< std::vector< uint8_t > > _fisheye_calibration_table_raw;
        uint16_t _pid;
    };

    class tm1_imu_calib_parser : public mm_calib_parser
    {
    public:
        tm1_imu_calib_parser(const std::vector<uint8_t>& raw_data)
        {
            calib_table = *(ds::check_calib<ds::tm1_eeprom>(raw_data));
        }
        tm1_imu_calib_parser(const tm1_imu_calib_parser&);
        virtual ~tm1_imu_calib_parser() {}

        float3x3 imu_to_depth_alignment() { return { {1,0,0},{0,1,0},{0,0,1} }; }

        rs2_extrinsics get_extrinsic_to(rs2_stream stream);

        ds::imu_intrinsic get_intrinsic(rs2_stream stream);

    private:
        ds::tm1_eeprom  calib_table;
    };

    class dm_v2_imu_calib_parser : public mm_calib_parser
    {
    public:
        dm_v2_imu_calib_parser(const std::vector<uint8_t>& raw_data, uint16_t pid, bool valid = true);

        dm_v2_imu_calib_parser(const dm_v2_imu_calib_parser&);
        virtual ~dm_v2_imu_calib_parser() {}

        float3x3 imu_to_depth_alignment() { return _imu_2_depth_rot; }

        rs2_extrinsics get_extrinsic_to(rs2_stream stream);

        ds::imu_intrinsic get_intrinsic(rs2_stream stream);

    private:
        ds::dm_v2_eeprom    _calib_table;
        rs2_extrinsics      _extr;
        float3x3            _imu_2_depth_rot;
        ds::dm_v2_imu_intrinsic _def_intr;
        bool                _valid_intrinsic;
        bool                _valid_extrinsic;
        uint16_t            _pid;
    };

    class l500_imu_calib_parser : public mm_calib_parser
    {
    public:
        l500_imu_calib_parser(const std::vector<uint8_t>& raw_data, bool valid = true);

        virtual ~l500_imu_calib_parser() {}

        float3x3 imu_to_depth_alignment() { return _imu_2_depth_rot; }

        ds::imu_intrinsic get_intrinsic(rs2_stream stream);
        
        rs2_extrinsics get_extrinsic_to(rs2_stream stream);

    private:
        ds::dm_v2_calibration_table  imu_calib_table;
        rs2_extrinsics      _extr;
        float3x3            _imu_2_depth_rot;
        ds::dm_v2_imu_intrinsic _def_intr;
        bool                _valid_intrinsic;
        bool                _valid_extrinsic;
    };
}
