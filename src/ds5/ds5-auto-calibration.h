// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "auto-calibrated-device.h"
#include "../core/advanced_mode.h"

namespace librealsense
{
    class auto_calibrated : public auto_calibrated_interface
    {
#pragma pack(push, 1)
#pragma pack(1)
        struct table_header
        {
            uint16_t version;        // major.minor. Big-endian
            uint16_t table_type;     // ctCalibration
            uint32_t table_size;     // full size including: TOC header + TOC + actual tables
            uint32_t param;          // This field content is defined ny table type
            uint32_t crc32;          // crc of all the actual table data excluding header/CRC
        };

        struct DirectSearchCalibrationResult
        {
            uint16_t status;      // DscStatus
            uint16_t stepCount;
            uint16_t stepSize; // 1/1000 of a pixel
            uint32_t pixelCountThreshold; // minimum number of pixels in
                                          // selected bin
            uint16_t minDepth;  // Depth range for FWHM
            uint16_t maxDepth;
            uint32_t rightPy;   // 1/1000000 of normalized unit
            float healthCheck;
            float rightRotation[9]; // Right rotation
            //uint16_t results[0]; // 1/100 of a percent
        };

        struct DscResultParams
        {
            uint16_t m_status;
            float    m_healthCheck;
        };

        struct DscResultBuffer
        {
            uint16_t m_paramSize;
            DscResultParams m_dscResultParams;
            uint16_t m_tableSize;
        };

        enum rs2_dsc_status : uint16_t
        {
            RS2_DSC_STATUS_SUCCESS = 0, /**< Self calibration succeeded*/
            RS2_DSC_STATUS_RESULT_NOT_READY = 1, /**< Self calibration result is not ready yet*/
            RS2_DSC_STATUS_FILL_FACTOR_TOO_LOW = 2, /**< There are too little textures in the scene*/
            RS2_DSC_STATUS_EDGE_TOO_CLOSE = 3, /**< Self calibration range is too small*/
            RS2_DSC_STATUS_NOT_CONVERGE = 4, /**< For tare calibration only*/
            RS2_DSC_STATUS_BURN_SUCCESS = 5,
            RS2_DSC_STATUS_BURN_ERROR = 6,
            RS2_DSC_STATUS_NO_DEPTH_AVERAGE = 7,
        };
#pragma pack(pop)

    public:
        auto_calibrated(std::shared_ptr<hw_monitor>& hwm);
        void write_calibration() const override;
        std::vector<uint8_t> run_on_chip_calibration(int timeout_ms, std::string json, float* health, update_progress_callback_ptr progress_callback) override;
        std::vector<uint8_t> run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* health, update_progress_callback_ptr progress_callback) override;
        std::vector<uint8_t> get_calibration_table() const override;
        void set_calibration_table(const std::vector<uint8_t>& calibration) override;
        void reset_to_factory_calibration() const override;

    private:
        std::vector<uint8_t> get_calibration_results(float* health) const;
        void handle_calibration_error(rs2_dsc_status status) const;
        std::map<std::string, int> parse_json(std::string json);
        std::shared_ptr< ds5_advanced_mode_base> change_preset();
        std::vector<uint8_t> _curr_calibration;
        std::shared_ptr<hw_monitor>& _hw_monitor;

        int _average_step_count = 20;
        int _step_count = 20;
        int _accuracy = 2;
        int _speed = 3;
    };

}
