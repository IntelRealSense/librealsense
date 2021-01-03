// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    // Enforce complile-time verification of all the assigned FPS profiles
    enum class IMU_OUTPUT_DATA_RATES : uint16_t
    {
        IMU_FPS_63  = 63,
        IMU_FPS_100 = 100,
        IMU_FPS_200 = 200,
        IMU_FPS_250 = 250,
        IMU_FPS_400 = 400
    };

    using odr = IMU_OUTPUT_DATA_RATES;

#ifdef _WIN32
    static const std::string gyro_sensor_name = "HID Sensor Class Device: Gyroscope";
    static const std::string accel_sensor_name = "HID Sensor Class Device: Accelerometer";
    static const std::map<odr, uint16_t> hid_fps_translation =
    {  //FPS   Value to send to the Driver
        {odr::IMU_FPS_63,   1000},
        {odr::IMU_FPS_100,  1000},
        {odr::IMU_FPS_200,  500},
        {odr::IMU_FPS_250,  400},
        {odr::IMU_FPS_400,  250} };

#else
    static const std::string gyro_sensor_name = "gyro_3d";
    static const std::string accel_sensor_name = "accel_3d";
    static const std::map<IMU_OUTPUT_DATA_RATES, unsigned> hid_fps_translation =
    {  //FPS   Value to send to the Driver
        {odr::IMU_FPS_63,   1},
        {odr::IMU_FPS_100,  1},
        {odr::IMU_FPS_200,  2},
        {odr::IMU_FPS_250,  3},
        {odr::IMU_FPS_400,  4} };
#endif

    class mm_calib_parser
    {
    public:
        virtual rs2_extrinsics get_extrinsic_to(rs2_stream) = 0;    // Extrinsics are referenced to the Depth stream, except for TM1
        virtual ds::imu_intrinsic get_intrinsic(rs2_stream) = 0;    // With extrinsic from FE<->IMU only
        virtual float3x3 imu_to_depth_alignment() = 0;
    };

    class tm1_imu_calib_parser : public mm_calib_parser
    {
    public:
        tm1_imu_calib_parser(const std::vector<uint8_t>& raw_data)
        {
            calib_table = *(ds::check_calib<ds::tm1_eeprom>(raw_data));
        }
        tm1_imu_calib_parser(const tm1_imu_calib_parser&);
        virtual ~tm1_imu_calib_parser(){}

        float3x3 imu_to_depth_alignment() { return {{1,0,0},{0,1,0},{0,0,1}}; }

        rs2_extrinsics get_extrinsic_to(rs2_stream stream)
        {
            if (!(RS2_STREAM_ACCEL == stream) && !(RS2_STREAM_GYRO == stream) && !(RS2_STREAM_FISHEYE == stream))
                throw std::runtime_error(to_string() << "TM1 Calibration does not provide extrinsic for : " << rs2_stream_to_string(stream) << " !");

            auto fe_calib = calib_table.calibration_table.calib_model.fe_calibration;

            auto rot = fe_calib.fisheye_to_imu.rotation;
            auto trans = fe_calib.fisheye_to_imu.translation;

            pose ex = { { { rot(0,0), rot(1,0),rot(2,0)},
                          { rot(0,1), rot(1,1),rot(2,1) },
                          { rot(0,2), rot(1,2),rot(2,2) } },
                          { trans[0], trans[1], trans[2] } };

            if (RS2_STREAM_FISHEYE == stream)
                return inverse(from_pose(ex));
            else
                return from_pose(ex);
        }

        ds::imu_intrinsic get_intrinsic(rs2_stream stream)
        {
            ds::imu_intrinsics in_intr;
            switch (stream)
            {
            case RS2_STREAM_ACCEL:
                in_intr = calib_table.calibration_table.imu_calib_table.accel_intrinsics; break;
            case RS2_STREAM_GYRO:
                in_intr = calib_table.calibration_table.imu_calib_table.gyro_intrinsics; break;
            default:
                throw std::runtime_error(to_string() << "TM1 IMU Calibration does not support intrinsic for : " << rs2_stream_to_string(stream) << " !");
            }
            ds::imu_intrinsic out_intr{};
            for (auto i = 0; i < 3; i++)
            {
                out_intr.sensitivity(i,i)   = in_intr.scale[i];
                out_intr.bias[i]            = in_intr.bias[i];
            }
            return out_intr;
        }

    private:
        ds::tm1_eeprom  calib_table;
    };

    class dm_v2_imu_calib_parser : public mm_calib_parser
    {
    public:
        dm_v2_imu_calib_parser(const std::vector<uint8_t>& raw_data, uint16_t pid, bool valid = true)
        {
            // product id to identify platform specific parameters, imu models and physical location
            _pid = pid;

            _calib_table.module_info.dm_v2_calib_table.extrinsic_valid = 0;
            _calib_table.module_info.dm_v2_calib_table.intrinsic_valid = 0;

            _valid_intrinsic = false;
            _valid_extrinsic = false;

            // default parser to be applied when no FW calibration is available
            if (valid)
            {
                try
                {
                    _calib_table = *(ds::check_calib<ds::dm_v2_eeprom>(raw_data));
                    _valid_intrinsic = (_calib_table.module_info.dm_v2_calib_table.intrinsic_valid == 1) ? true : false;
                    _valid_extrinsic = (_calib_table.module_info.dm_v2_calib_table.extrinsic_valid == 1) ? true : false;
                }
                catch (...)
                {
                    _valid_intrinsic = false;
                    _valid_extrinsic = false;
                }
            }

            // TODO - review possibly refactor into a map if necessary
            //
            // predefined platform specific extrinsic, IMU assembly transformation based on mechanical drawing (meters)
            rs2_extrinsics _def_extr;

            if (_pid == ds::RS435I_PID)
            {
                // D435i specific - Bosch BMI055
                _def_extr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 }, { -0.00552f, 0.0051f, 0.01174f} };
                _imu_2_depth_rot = { {-1,0,0},{0,1,0},{0,0,-1} };
            }
            else if (_pid == ds::RS455_PID)
            {
                // D455 specific - Bosch BMI055
                _def_extr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ -0.03022f, 0.0074f, 0.01602f } };
                _imu_2_depth_rot = { { -1,0,0 },{ 0,1,0 },{ 0,0,-1 } };
            }
            else if (_pid == ds::RS465_PID)
            {
                // D465 specific - Bosch BMI085
                // TODO - verify with mechanical drawing
                _def_extr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ -0.10125f, -0.00375f, -0.0013f } };
                _imu_2_depth_rot = { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } };
            }
            else // unmapped configurations
            {
                // IMU on new devices is oriented such that FW output is consistent with D435i
                // use same rotation matrix as D435i so that librealsense output from unsupported
                // devices will still be correctly aligned with depth coordinate system.
                _def_extr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ 0.f, 0.f, 0.f } };
                _imu_2_depth_rot = { { -1,0,0 },{ 0,1,0 },{ 0,0,-1 } };
                LOG_ERROR("Undefined platform with IMU, use default intrinsic/extrinsic data, PID: " << _pid);
            }

            // default intrinsic in case no valid calibration data is available
            // scale = 1 and offset = 0
            _def_intr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ 0.0, 0.0, 0.0 } };

            // handling extrinsic
            if (_valid_extrinsic)
            {
                // extrinsic from calibration table, by user custom calibration, The extrinsic is stored as array of floats / little-endian
                librealsense::copy(&_extr, &_calib_table.module_info.dm_v2_calib_table.depth_to_imu, sizeof(rs2_extrinsics));
            }
            else
            {
                LOG_INFO("IMU extrinsic table not found; using CAD values");
                // default extrinsic based on mechanical drawing
                _extr = _def_extr;
            }
        }

        dm_v2_imu_calib_parser(const dm_v2_imu_calib_parser&);
        virtual ~dm_v2_imu_calib_parser() {}

        float3x3 imu_to_depth_alignment() { return _imu_2_depth_rot; }

        rs2_extrinsics get_extrinsic_to(rs2_stream stream)
        {
            if (!(RS2_STREAM_ACCEL == stream) && !(RS2_STREAM_GYRO == stream))
                throw std::runtime_error(to_string() << "Depth Module V2 does not support extrinsic for : " << rs2_stream_to_string(stream) << " !");

            return _extr;
        }

        ds::imu_intrinsic get_intrinsic(rs2_stream stream)
        {
            ds::dm_v2_imu_intrinsic in_intr;
            switch (stream)
            {
                case RS2_STREAM_ACCEL:
                    if (_valid_intrinsic)
                    {
                        in_intr = _calib_table.module_info.dm_v2_calib_table.accel_intrinsic;
                    }
                    else
                    {
                        LOG_INFO("Depth Module V2 IMU " << rs2_stream_to_string(stream) << "no valid intrinsic available, use default values.");
                        in_intr = _def_intr;
                    }
                    break;
                case RS2_STREAM_GYRO:
                    if (_valid_intrinsic)
                    {
                        in_intr = _calib_table.module_info.dm_v2_calib_table.gyro_intrinsic;
                        in_intr.bias = in_intr.bias * static_cast<float>(d2r);        // The gyro bias is calculated in Deg/sec
                    }
                    else
                    {
                        LOG_INFO("Depth Module V2 IMU " << rs2_stream_to_string(stream) << "intrinsic not valid, use default values.");
                        in_intr = _def_intr;
                    }
                    break;
                default:
                    throw std::runtime_error(to_string() << "Depth Module V2 does not provide intrinsic for stream type : " << rs2_stream_to_string(stream) << " !");
            }

            return { in_intr.sensitivity, in_intr.bias, {0,0,0}, {0,0,0} };
        }

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
        l500_imu_calib_parser(const std::vector<uint8_t>& raw_data, bool valid = true)
        {
            // default parser to be applied when no FW calibration is available
            _valid_intrinsic = false;
            _valid_extrinsic = false;

            // in case calibration table is provided but with incorrect header and CRC, both intrinsic and extrinsic will use default values
            // calibration table has flags to indicate if it contains valid intrinsic and extrinsic, use this as further indication if the data is valid
            // currently, the imu calibration script only calibrates intrinsic so only the intrinsic_valid field is set during calibration, extrinsic
            // will use default values derived from mechanical CAD drawing, however, if the calibration script in the future or user calibration provide
            // valid extrinsic, the extrinsic_valid field should be set in the table and detected here so the values from the table can be used.
            if (valid)
            {
                try
                {
                    imu_calib_table = *(ds::check_calib<ds::dm_v2_calibration_table>(raw_data));
                    _valid_intrinsic = (imu_calib_table.intrinsic_valid == 1) ? true : false;
                    _valid_extrinsic = (imu_calib_table.extrinsic_valid == 1) ? true : false;
                }
                catch (...)
                {
                    _valid_intrinsic = false;
                    _valid_extrinsic = false;
                }
            }

            // L515 specific
            // Bosch BMI085 assembly transformation based on mechanical drawing (meters)
            // device thickness 26 mm from front glass to back surface
            // depth ground zero is 4.5mm from front glass into the device
            // IMU reference in z direction is at 20.93mm from back surface
            //
            // IMU offset in Z direction = 4.5 mm - (26 mm - 20.93 mm) = 4.5 mm - 5.07mm = - 0.57mm
            // IMU offset in x and Y direction (12.45mm, -16.42mm) from center
            //
            // coordinate system as reference, looking from back of the camera towards front,
            // the positive x-axis points to the right, the positive y-axis points down, and the
            // positive z-axis points forward.
            // origin in the center but z-direction 4.5mm from front glass into the device
            // the matrix below is such that output of motion data is consistent with convention
            // that positive direction aligned with gravity leads to -1g and opposite direction
            // leads to +1g, for example, positive z_aixs points forward away from front glass of
            // the device, 1) if place the device flat on a table, facing up, positive z-axis points
            // up, z-axis acceleration is around +1g; 2) facing down, positive z-axis points down,
            // z-axis accleration would be around -1g
            rs2_extrinsics _def_extr;
            _def_extr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ -0.01245f, 0.01642f, 0.00057f } };
            _imu_2_depth_rot = { { -1, 0, 0 },{ 0, 1, 0 },{ 0, 0, -1 } };

            // default intrinsic in case no valid calibration data is available
            // scale = 1 and offset = 0
            _def_intr = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 },{ 0.0, 0.0, 0.0 } };


            // handling extrinsic
            if (_valid_extrinsic)
            {
                // only in case valid extrinsic is available in calibration data by calibration script in future or user custom calibration
                librealsense::copy(&_extr, &imu_calib_table.depth_to_imu, sizeof(rs2_extrinsics));
            }
            else
            {
                // L515 - BMI085 assembly transformation based on mechanical drawing
                LOG_INFO("IMU extrinsic using CAD values");
                _extr = _def_extr;
            }
        }

        virtual ~l500_imu_calib_parser() {}

        float3x3 imu_to_depth_alignment() { return _imu_2_depth_rot; }

        ds::imu_intrinsic get_intrinsic(rs2_stream stream)
        {
            ds::dm_v2_imu_intrinsic in_intr;
            switch (stream)
            {
            case RS2_STREAM_ACCEL:
                if (_valid_intrinsic)
                {
                    in_intr = imu_calib_table.accel_intrinsic;
                }
                else
                {
                    LOG_INFO("L515 IMU " << rs2_stream_to_string(stream) << "no valid intrinsic available, use default values.");
                    in_intr = _def_intr;
                }
                break;
            case RS2_STREAM_GYRO:
                if (_valid_intrinsic)
                {
                    in_intr = imu_calib_table.gyro_intrinsic;
                    in_intr.bias = in_intr.bias * static_cast<float>(d2r);        // The gyro bias is calculated in Deg/sec
                }
                else
                {
                    LOG_INFO("L515 IMU " << rs2_stream_to_string(stream) << "no valid intrinsic available, use default values.");
                    in_intr = _def_intr;
                }
                break;
            default:
                throw std::runtime_error(to_string() << "L515 does not provide intrinsic for stream type : " << rs2_stream_to_string(stream) << " !");
            }

            return{ in_intr.sensitivity, in_intr.bias,{ 0,0,0 },{ 0,0,0 } };
        }

        rs2_extrinsics get_extrinsic_to(rs2_stream stream)
        {
            if (!(RS2_STREAM_ACCEL == stream) && !(RS2_STREAM_GYRO == stream))
                throw std::runtime_error(to_string() << "L515 does not support extrinsic for : " << rs2_stream_to_string(stream) << " !");

            return _extr;
        }

    private:
        ds::dm_v2_calibration_table  imu_calib_table;
        rs2_extrinsics      _extr;
        float3x3            _imu_2_depth_rot;
        ds::dm_v2_imu_intrinsic _def_intr;
        bool                _valid_intrinsic;
        bool                _valid_extrinsic;
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
        lazy< std::shared_ptr<mm_calib_parser>> _calib_parser;
        lazy<std::vector<uint8_t>>      _imu_eeprom_raw;
        std::vector<uint8_t>            get_imu_eeprom_raw() const;
        std::vector<uint8_t>            get_imu_eeprom_raw_l515() const;
        lazy<std::vector<uint8_t>>      _fisheye_calibration_table_raw;
        uint16_t _pid;
    };

    class ds5_motion : public virtual ds5_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_hid_device(std::shared_ptr<context> ctx,
                                                      const std::vector<platform::hid_device_info>& all_hid_infos,
                                                      const firmware_version& camera_fw_version);

        ds5_motion(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

        std::shared_ptr<auto_exposure_mechanism> register_auto_exposure_options(synthetic_sensor* ep,
                                                                                const platform::extension_unit* fisheye_xu);

    private:

        friend class ds5_fisheye_sensor;
        friend class ds5_hid_sensor;

        void initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group);

        optional_value<uint8_t> _fisheye_device_idx;
        optional_value<uint8_t> _motion_module_device_idx;

        std::shared_ptr<mm_calib_handler>        _mm_calib;
        std::shared_ptr<lazy<ds::imu_intrinsic>> _accel_intrinsic;
        std::shared_ptr<lazy<ds::imu_intrinsic>> _gyro_intrinsic;
        lazy<std::vector<uint8_t>>              _fisheye_calibration_table_raw;
        std::shared_ptr<lazy<rs2_extrinsics>>   _depth_to_imu;                  // Mechanical installation pose

        uint16_t _pid;    // product PID

        // Bandwidth parameters required for HID sensors
        // The Acceleration configuration will be resolved according to the IMU sensor type at run-time
        std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles =
        { { gyro_sensor_name,     {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, 0, 1, 1, int(odr::IMU_FPS_200)}},
          { gyro_sensor_name,     {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, 0, 1, 1, int(odr::IMU_FPS_400)}}};

        // Translate frequency to SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL.
        std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream =
        { { RS2_STREAM_GYRO,     {{unsigned(odr::IMU_FPS_200),  hid_fps_translation.at(odr::IMU_FPS_200)},
                                 { unsigned(odr::IMU_FPS_400),  hid_fps_translation.at(odr::IMU_FPS_400)}}} };

    protected:
        std::shared_ptr<stream_interface> _fisheye_stream;
        std::shared_ptr<stream_interface> _accel_stream;
        std::shared_ptr<stream_interface> _gyro_stream;
    };
}
