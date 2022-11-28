// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds-motion-common.h"

#include "l500/l500-private.h"

namespace librealsense
{
    mm_calib_handler::mm_calib_handler(std::shared_ptr<hw_monitor> hw_monitor, uint16_t pid) :
        _hw_monitor(hw_monitor), _pid(pid)
    {
        _imu_eeprom_raw = [this]() {
            if (_pid == L515_PID)
                return get_imu_eeprom_raw_l515();
            else
                return get_imu_eeprom_raw();
        };

        _calib_parser = [this]() {

            std::vector<uint8_t> raw(ds::tm1_eeprom_size);
            uint16_t calib_id = ds::dm_v2_eeprom_id; //assume DM V2 IMU as default platform
            bool valid = false;

            if (_pid == L515_PID) calib_id = ds::l500_eeprom_id;

            try
            {
                raw = *_imu_eeprom_raw;
                calib_id = *reinterpret_cast<uint16_t*>(raw.data());
                valid = true;
            }
            catch (const std::exception&)
            {
                // in case calibration table errors (invalid table, empty table, or corrupted table), data is invalid and default intrinsic and extrinsic will be used
                LOG_WARNING("IMU Calibration is not available, default intrinsic and extrinsic will be used.");
            }

            std::shared_ptr<mm_calib_parser> prs = nullptr;
            switch (calib_id)
            {
            case ds::dm_v2_eeprom_id: // DM V2 id
                prs = std::make_shared<dm_v2_imu_calib_parser>(raw, _pid, valid); break;
            case ds::tm1_eeprom_id: // TM1 id
                prs = std::make_shared<tm1_imu_calib_parser>(raw); break;
            case ds::l500_eeprom_id: // L515
                prs = std::make_shared<l500_imu_calib_parser>(raw, valid); break;
            default:
                throw recoverable_exception(to_string() << "Motion Intrinsics unresolved - "
                    << ((valid) ? "device is not calibrated" : "invalid calib type "),
                    RS2_EXCEPTION_TYPE_BACKEND);
            }
            return prs;
        };
    }

    std::vector<uint8_t> mm_calib_handler::get_imu_eeprom_raw() const
    {
        const int offset = 0;
        const int size = ds::eeprom_imu_table_size;
        command cmd(ds::MMER, offset, size);
        return _hw_monitor->send(cmd);
    }

    std::vector<uint8_t> mm_calib_handler::get_imu_eeprom_raw_l515() const
    {
        // read imu calibration table on L515
        // READ_TABLE 0x243 0
        command cmd(ivcam2::READ_TABLE, ivcam2::L515_IMU_TABLE, 0);
        return _hw_monitor->send(cmd);
    }

    ds::imu_intrinsic mm_calib_handler::get_intrinsic(rs2_stream stream)
    {
        return (*_calib_parser)->get_intrinsic(stream);
    }

    rs2_extrinsics mm_calib_handler::get_extrinsic(rs2_stream stream)
    {
        return (*_calib_parser)->get_extrinsic_to(stream);
    }

    const std::vector<uint8_t> mm_calib_handler::get_fisheye_calib_raw()
    {
        auto fe_calib_table = (*(ds::check_calib<ds::tm1_eeprom>(*_imu_eeprom_raw))).calibration_table.calib_model.fe_calibration;
        uint8_t* fe_calib_ptr = reinterpret_cast<uint8_t*>(&fe_calib_table);
        return std::vector<uint8_t>(fe_calib_ptr, fe_calib_ptr + ds::fisheye_calibration_table_size);
    }
}