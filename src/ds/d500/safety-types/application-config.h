// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include "common.h"
#include <rsutils/json.h>

namespace librealsense
{

    using rsutils::json;

#pragma pack(push, 1)

    /***
     * safety_ip (SIP) class
     * Handles General Status - Required for normal operation mode
     *
     * Fields:
     *         SIP Immediate Mode Safety Features Selection (uint8_t):
     *            Bit Mask to notify which of the 8 SIP immediate SM triggers are active
     *          For each monitor in range [0..7] a corresponding bit shall be toggled to notify AICV and S.MCU on the particular Safety Mechanism activation
     *
     *         SIP Temporal Safety Features Selection (uint8_t):
     *             Bit Mask to notify which of the 8 SIP generic mechanisms shall be monitored
     *             For each monitor in range [0..7] a corresponding bit shall be toggled on in order to activate Safety Mechanism in S.MCU
     *
     *         SIP Mechanisms thresholds (uint8_t[16]):
     *             Byte array for HaRa continuous metrics evidence present in X out of Y consecutive frames.
     *             The bytes are arranged in eight pairs corresponding to SIP Temporal SMs with indexes [0..7], where:
     *             Byte (N) : X value for the trigger threshold value X. Valid range is [ 0..150]
     *             Byte (N +1 ): Y value. Designates the frames window size in [ 0..150] range within which the value of X shall be looked for.
     *             Note that for each pair the following condition must hold  0 <=X<=Y
     *
     *         SIP Mechanism sampling interval    (uint8_t[8]):
     *             Byte array specifying the expected generated sampling rate per each of the corresponding Temporal SM in range [0...7]:
     *             Each number is the array is a positive number specifting for the parties at which rate the specified metric is expected to be updated.
     *             The metric shall be tracked in S.MCU to establish whether the SM failed to provide an update in a predefined manner
     *
     *         Reserved    64    uint8_t[64]    zero-ed
     */
    class safety_ip
    {
    public:
        safety_ip(const json &j) : m_immediate_mode_safety_features_selection(j["immediate_mode_safety_features_selection"].get<uint8_t>()),
                                   m_temporal_safety_features_selection(j["temporal_safety_features_selection"].get<uint8_t>())
        {
            std::vector<uint8_t> mechanisms_thresholds_vec = j["mechanisms_thresholds"].get<std::vector<uint8_t>>();
            std::memcpy(m_mechanisms_thresholds, mechanisms_thresholds_vec.data(), mechanisms_thresholds_vec.size());

            std::vector<uint8_t> mechanisms_sampling_interval_vec = j["mechanisms_sampling_interval"].get<std::vector<uint8_t>>();
            std::memcpy(m_mechanisms_sampling_interval, mechanisms_sampling_interval_vec.data(), mechanisms_sampling_interval_vec.size());

            std::vector<uint8_t> reserved_vec = j["reserved"].get<std::vector<uint8_t>>();
            std::memcpy(m_reserved, reserved_vec.data(), reserved_vec.size());
        }

        json to_json() const
        {
            json j;
            j["immediate_mode_safety_features_selection"] = m_immediate_mode_safety_features_selection;
            j["temporal_safety_features_selection"] = m_temporal_safety_features_selection;
            j["mechanisms_thresholds"] = native_arr_to_std_vector(m_mechanisms_thresholds);
            j["mechanisms_sampling_interval"] = native_arr_to_std_vector(m_mechanisms_sampling_interval);
            j["reserved"] = native_arr_to_std_vector(m_reserved);
            return j;
        }

    private:
        uint8_t m_immediate_mode_safety_features_selection;
        uint8_t m_temporal_safety_features_selection;
        uint8_t m_mechanisms_thresholds[16];
        uint8_t m_mechanisms_sampling_interval[8];
        uint8_t m_reserved[64];
    };

    /***
     * temp_thresholds class
     * Handles temperature thresholds of sensors
     *
     * Generic notes for all temperature thresholds fields below (except the reserved fields):
     *         - All units are in degrees
     *         - Assume |Danger threshold| >= Warning threshold
     *         - Byte0: Upper Threshold Warn
     *         - Byte1: Upper Threshold Danger
     *         - Byte2: Lower Threshold Warn
     *         - Byte3: Lower Threshold Danger"
     * Fields:
     *         Temp Thresholds: IR-R
     *         Temp Thresholds: IR-L
     *         Temp Thresholds: APM-L
     *         Temp Thresholds: APM-R
     *         reserved (zero-ed)
     *         Temp Thresholds: HKR (core)
     *         Temp Thresholds: SMCU
     *         reserved (zero-ed)
     *         Temp Thresholds: SHT4x
     *         reserved (zero-ed)
     *         Temp Thresholds: IMU
     */
    class temp_thresholds
    {
    public:
        temp_thresholds(const json &j)
        {
            std::vector<int8_t> ir_right_vec = j["ir_right"].get<std::vector<int8_t>>();
            std::memcpy(m_ir_right, ir_right_vec.data(), ir_right_vec.size());

            std::vector<int8_t> ir_left_vec = j["ir_left"].get<std::vector<int8_t>>();
            std::memcpy(m_ir_left, ir_left_vec.data(), ir_left_vec.size());

            std::vector<int8_t> apm_left_vec = j["apm_left"].get<std::vector<int8_t>>();
            std::memcpy(m_apm_left, apm_left_vec.data(), apm_left_vec.size());

            std::vector<int8_t> apm_right_vec = j["apm_right"].get<std::vector<int8_t>>();
            std::memcpy(m_apm_right, apm_right_vec.data(), apm_right_vec.size());

            std::vector<int8_t> reserved1_vec = j["reserved1"].get<std::vector<int8_t>>();
            std::memcpy(m_reserved1, reserved1_vec.data(), reserved1_vec.size());

            std::vector<int8_t> hkr_core_vec = j["hkr_core"].get<std::vector<int8_t>>();
            std::memcpy(m_hkr_core, hkr_core_vec.data(), hkr_core_vec.size());

            std::vector<int8_t> smcu_right_vec = j["smcu_right"].get<std::vector<int8_t>>();
            std::memcpy(m_smcu_right, smcu_right_vec.data(), smcu_right_vec.size());

            std::vector<int8_t> reserved2_vec = j["reserved2"].get<std::vector<int8_t>>();
            std::memcpy(m_reserved2, reserved2_vec.data(), reserved2_vec.size());

            std::vector<int8_t> sht4x_vec = j["sht4x"].get<std::vector<int8_t>>();
            std::memcpy(m_sht4x, sht4x_vec.data(), sht4x_vec.size());

            std::vector<int8_t> reserved3_vec = j["reserved3"].get<std::vector<int8_t>>();
            std::memcpy(m_reserved3, reserved3_vec.data(), reserved3_vec.size());

            std::vector<int8_t> imu_vec = j["imu"].get<std::vector<int8_t>>();
            std::memcpy(m_imu, imu_vec.data(), imu_vec.size());
        }

        json to_json() const
        {
            json j;
            j["ir_right"] = std::vector<int8_t>(std::begin(m_ir_right), std::end(m_ir_right));
            j["ir_left"] = native_arr_to_std_vector(m_ir_left);
            j["apm_left"] = native_arr_to_std_vector(m_apm_left);
            j["apm_right"] = native_arr_to_std_vector(m_apm_left);
            j["reserved1"] = native_arr_to_std_vector(m_reserved1);
            j["hkr_core"] = native_arr_to_std_vector(m_hkr_core);
            j["smcu_right"] = native_arr_to_std_vector(m_smcu_right);
            j["reserved2"] = native_arr_to_std_vector(m_reserved2);
            j["sht4x"] = native_arr_to_std_vector(m_sht4x);
            j["reserved3"] = native_arr_to_std_vector(m_reserved3);
            j["imu"] = native_arr_to_std_vector(m_imu);
            return j;
        }

    private:
        int8_t m_ir_right[4];
        int8_t m_ir_left[4];
        int8_t m_apm_left[4];
        int8_t m_apm_right[4];
        int8_t m_reserved1[4];
        int8_t m_hkr_core[4];
        int8_t m_smcu_right[4];
        int8_t m_reserved2[4];
        int8_t m_sht4x[4];
        int8_t m_reserved3[4];
        int8_t m_imu[4];
    };

    /***
     * voltage_thresholds class
     * Handles voltage thresholds
     *
     * Generic notes for all voltage thresholds fields below:
     *         Threshold percentage units [0..100]
     *         E.g threshold of 30% meaning that the valid range for 3.3v: 3.3v +- 1.1v
     *
     * Fields:
     *         Voltage Threshold VDD3V3
     *         Voltage Threshold VDD1V8
     *         Voltage Threshold VDD1V2
     *         Voltage Threshold VDD1V1
     *         Voltage Threshold VDD0V8
     *         Voltage Threshold VDD0V6     --> Upper threshold percentage units: (5VDD up/low thresholds are not symmetric by design)
     *         Voltage Threshold VDD5V0_U
     *         Voltage Threshold VDD5V0_L
     *         Voltage Threshold VDD0V8_DDR
     */
    class voltage_thresholds
    {
    public:
        voltage_thresholds(const json &j) : m_vdd3v3(j["vdd3v3"].get<uint8_t>()),
                                            m_vdd1v8(j["vdd1v8"].get<uint8_t>()),
                                            m_vdd1v2(j["vdd1v2"].get<uint8_t>()),
                                            m_vdd1v1(j["vdd1v1"].get<uint8_t>()),
                                            m_vdd0v8(j["vdd0v8"].get<uint8_t>()),
                                            m_vdd0v6(j["vdd0v6"].get<uint8_t>()),
                                            m_vdd5vo_u(j["vdd5vo_u"].get<uint8_t>()),
                                            m_vdd5vo_l(j["vdd5vo_l"].get<uint8_t>()),
                                            m_vdd0v8_ddr(j["vdd0v8_ddr"].get<uint8_t>())
        {
        }

        json to_json() const
        {
            json j;
            j["vdd3v3"] = m_vdd3v3;
            j["vdd1v8"] = m_vdd3v3;
            j["vdd1v2"] = m_vdd1v2;
            j["vdd1v1"] = m_vdd1v1;
            j["vdd0v8"] = m_vdd0v8;
            j["vdd0v6"] = m_vdd0v6;
            j["vdd5vo_u"] = m_vdd5vo_u;
            j["vdd5vo_l"] = m_vdd5vo_l;
            j["vdd0v8_ddr"] = m_vdd0v8_ddr;
            return j;
        }

    private:
        uint8_t m_vdd3v3;
        uint8_t m_vdd1v8;
        uint8_t m_vdd1v2;
        uint8_t m_vdd1v1;
        uint8_t m_vdd0v8;
        uint8_t m_vdd0v6;
        uint8_t m_vdd5vo_u;
        uint8_t m_vdd5vo_l;
        uint8_t m_vdd0v8_ddr;
    };

    /***
     * developer_mode class
     *
     * Fields:
     *         HKR Developer Mode (uint8_t):
     *             Bitmaks Functional Limitation:
     *                 1 << 0 : Unlock UVC Controls in Op. Mode (default: 0==Locked)
     *                 1 << 1:  Unlock HWMC in Op. Mode except for Writing to NVM Calibration and Safety-related data. (default: 0==Locked)
     *
     *         SMCU Developer Mod (uint8_t):
     *             Bitmask  SMCU Error Handling override:
     *                 1 << 0:  Non-critical HKR-induced errors (L2 and lower) at Init stage to be inhibited and ignored and the system may go into Operational mode
     *                 1 << 1:  Non-critical HKR-induced errors (L2 and lower) at Operational stage to be inhibited and ignored and the system may go into Operational mode
     *                 1 << 2:  Ignore time-based message Error Detection (Message drops/delays)
     *                 1 <<3:  S.MCU Feat #1
     *                 1 <<4:  S.MCU Feat #2
     *                 1 <<4:  S.MCU Feat #3"
     *
     *         HKR Developer Mode: Simulated "lock" state (uint8_t):
     *             Toggle off/on - HKR to report device is in "Locked" state and enforce all the derived logics internally (HWMC limitations)
     *
     *         SC Developer Mode (uint8_t):
     *             Safety interface behavior bitmask:
     *                 1 << 0 : Disregard invalid M12 Safety Zone selection input eventhough the device reports ""locked"" state.
     *                 1 << 1: Require M12 presence check regardless of ""Locked state"". In case both bits 0 and 1 are defined bit_0 is in higher priority
     *                 bits 2-7: TBD
     */
    class developer_mode
    {
    public:
        developer_mode(const json &j) : m_hkr(j["hkr"].get<uint8_t>()),
                                        m_smcu(j["smcu"].get<uint8_t>()),
                                        m_hkr_simulated_lock_state(j["hkr_simulated_lock_state"].get<uint8_t>()),
                                        m_sc(j["sc"].get<uint8_t>())
        {
        }

        json to_json() const
        {
            json j;
            j["hkr"] = m_hkr;
            j["smcu"] = m_smcu;
            j["hkr_simulated_lock_state"] = m_hkr_simulated_lock_state;
            j["sc"] = m_sc;
            return j;
        }

    private:
        uint8_t m_hkr;
        uint8_t m_smcu;
        uint8_t m_hkr_simulated_lock_state;
        uint8_t m_sc;
    };

    /***
     * Application Config Table (according to flash0.92 specs)
     * Version:    0x01 0x00 (major.minor)
     * Table type: 0xC0DE (ctAppConfig)
     * Table size: 464 (bytes)
     *
     *
     * Fields:
     *         sip (safety_ip):
     *             see safety_ip class description above
     *         dev_rules_selection:
     *             Bit Mask values:
     *                 0x1 <<0 - Dev Mode is active. Note that if this bit is inactive then all the other can be safely disregarde
     *                 0x1 <<1 - Feat_#1 enable
     *                 0x1 <<2 - Feat_#2 enable
     *                 0x1 <<3 - ...
     *                 0x1 <<63 - Feat_#63 enabled
     *         depth_pipe_safety_checks_override:
     *             Enumerated values:
     *             0 - Depth pipeline checks - Nominal case. Enforce checks on locked units only (Default)
     *               - Checking S.N of OHM/APM/SMCU
     *            - Checking integrity of Depth calibration table
     *             1 - Depth pipeline checks are skipped. I.e. the S.MCU will not enforce the checks
     *             2 - Depth pipeline checks to be enforced even though the device may not be "locked"
     *         triggered_calib_safety_checks_override:
     *             Enumerated values:
     *                 0 - Safety Pipeline TC (Triggered Calibration)  checks  -  Nominal case. Enforce checks on locked units only (Default)
     *                      - Checking TC results table itegrity + last result
     *                 1 - Safety Pipeline TC checks are skipped. I.e. the S.MCU will not enforce the checks
     *                 2 - Safety Pipeline TC checks to be enforced even though the device may not be "locked"
     *         smcu_bypass_directly_to_maintenance_mode:
     *             Enum value:
     *                 0 - regular case (default)
     *                 1 - Upon exiting Init state disregard all non-critical (L2 and lower) error cases, and goes directly into Maintenance state
     *         mcu_skip_spi_error:
     *             Enumerated values: handling HS, CRC and Timeouts
     *                 0  - No changes. Verify all SPI communication is in place (default)
     *                 1 - Disregard critical Handshake/communication errors over HKR FuSa monitor SPI session
     *                 2 - Disregard critical Handshake/communication errors over Any SPI sessions
     *         temp_thresholds:
     *             See description above the temp_thresholds class
     *         sht4x_humidity_threshold:
     *             Threshold percentage units [0..100]
     *         voltage_thresholds:
     *             See description above the voltage_thresholds class
     *         reserved1: zero-ed
     *         reserved2: zero-ed
     *         developer_mode:
     *             See description above the developer_mode class
     *         depth_pipeline_config:
     *             Bitmask for Depth pipe config:
     *                 1 << 0 - Depth FPS selection for Safety Camera [0:60 FPS (Default), 1:30 FPS]
     *         depth_roi:
     *            Bitmask for Depth pipe config:
     *                 1 << 0 - Depth ROI is:
     *                 - 0 - Based on Diagnostic Zone
     *                 - 1: Global ROI
     *         ir_for_sip:
     *             Enum: IR Frames resolution for SIP Generics infrastructure
     *                 0 - 1280x720 (Default)
     *                 1 - 640x360
     *         reserved3[39]: zero-ed. Reserved for Bypass mode features
     *         digital_signature[32]: SHA2 or similar
     *         reserved4[228]: zero-ed
     */

    class application_config
    {
    public:
        application_config(const json &j) : m_sip(j["sip"]),
                                            m_dev_rules_selection(j["dev_rules_selection"]),
                                            m_depth_pipe_safety_checks_override(j["depth_pipe_safety_checks_override"]),
                                            m_triggered_calib_safety_checks_override(j["triggered_calib_safety_checks_override"]),
                                            m_smcu_bypass_directly_to_maintenance_mode(j["smcu_bypass_directly_to_maintenance_mode"]),
                                            m_smcu_skip_spi_error(j["smcu_skip_spi_error"]),
                                            m_temp_thresholds(j["temp_thresholds"]),
                                            m_sht4x_humidity_threshold(j["sht4x_humidity_threshold"]),
                                            m_voltage_thresholds(j["voltage_thresholds"]),
                                            m_reserved1(j["reserved1"]),
                                            m_reserved2(j["reserved2"]),
                                            m_developer_mode(j["developer_mode"]),
                                            m_depth_pipeline_config(j["depth_pipeline_config"]),
                                            m_depth_roi(j["depth_roi"]),
                                            m_ir_for_sip(j["ir_for_sip"])
        {
            std::vector<uint8_t> reserved3_vec = j["reserved3"].get<std::vector<uint8_t>>();
            std::memcpy(m_reserved3, reserved3_vec.data(), reserved3_vec.size());

            std::vector<uint8_t> digital_signature_vec = j["digital_signature"].get<std::vector<uint8_t>>();
            std::memcpy(m_digital_signature, digital_signature_vec.data(), digital_signature_vec.size());

            std::vector<uint8_t> reserved4_vec = j["reserved4"].get<std::vector<uint8_t>>();
            std::memcpy(m_reserved4, reserved4_vec.data(), reserved4_vec.size());
        }

        rsutils::json to_json()
        {
            rsutils::json j;
            auto &app_config_json = j["application_config"];
            app_config_json["sip"] = m_sip.to_json();
            app_config_json["dev_rules_selection"] = m_dev_rules_selection;
            app_config_json["depth_pipe_safety_checks_override"] = m_depth_pipe_safety_checks_override;
            app_config_json["triggered_calib_safety_checks_override"] = m_triggered_calib_safety_checks_override;
            app_config_json["smcu_bypass_directly_to_maintenance_mode"] = m_smcu_bypass_directly_to_maintenance_mode;
            app_config_json["smcu_skip_spi_error"] = m_smcu_skip_spi_error;
            app_config_json["temp_thresholds"] = m_temp_thresholds.to_json();
            app_config_json["sht4x_humidity_threshold"] = m_sht4x_humidity_threshold;
            app_config_json["voltage_thresholds"] = m_voltage_thresholds.to_json();
            app_config_json["reserved1"] = m_reserved1;
            app_config_json["reserved2"] = m_reserved2;
            app_config_json["developer_mode"] = m_developer_mode.to_json();
            app_config_json["depth_pipeline_config"] = m_depth_pipeline_config;
            app_config_json["depth_roi"] = m_depth_roi;
            app_config_json["ir_for_sip"] = m_ir_for_sip;
            app_config_json["reserved3"] = native_arr_to_std_vector(m_reserved3);
            app_config_json["digital_signature"] = native_arr_to_std_vector(m_digital_signature);
            app_config_json["reserved4"] = native_arr_to_std_vector(m_reserved4);
            return j;
        }

    private:
        safety_ip m_sip;
        uint64_t m_dev_rules_selection;
        uint8_t m_depth_pipe_safety_checks_override;
        uint8_t m_triggered_calib_safety_checks_override;
        uint8_t m_smcu_bypass_directly_to_maintenance_mode;
        uint8_t m_smcu_skip_spi_error;
        temp_thresholds m_temp_thresholds;
        uint8_t m_sht4x_humidity_threshold;
        voltage_thresholds m_voltage_thresholds;
        uint8_t m_reserved1;
        uint8_t m_reserved2;
        developer_mode m_developer_mode;
        uint8_t m_depth_pipeline_config;
        uint8_t m_depth_roi;
        uint8_t m_ir_for_sip;
        uint8_t m_reserved3[39];
        uint8_t m_digital_signature[32];
        uint8_t m_reserved4[228];
    };


    /***
     *  application_config_with_header class
     *  Consists applicaion config table and a table header
     */
    class application_config_with_header
    {
    public:
        application_config_with_header(table_header header, application_config app_config) : m_header(header), m_app_config(app_config)
        {
        }

        application_config get_application_config() const
        {
            return m_app_config;
        }

        table_header get_table_header() const
        {
            return m_header;
        }

    private:
        table_header m_header;
        application_config m_app_config;
    };

#pragma pack(pop)

}
