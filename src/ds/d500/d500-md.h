// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/metadata.h>

namespace librealsense
{

    /**\brief md_safety_info_attributes - bit mask to find active attributes,
     *   md_safety_info struct */
    enum class md_safety_info_attributes : uint32_t
    {
        frame_counter_attribute = (1u << 0),
        depth_frame_counter_attribute = (1u << 1),
        frame_timestamp_attribute = (1u << 2),
        level1_signal_attribute = (1u << 3),
        level1_frame_counter_origin_attribute = (1u << 4),
        level2_signal_attribute = (1u << 5),
        level2_frame_counter_origin_attribute = (1u << 6),
        level1_verdict_attribute = (1u << 7),
        level2_verdict_attribute = (1u << 8),
        operational_mode_attribute = (1u << 9),
        vision_safety_verdict_attribute = (1u << 10),
        vision_hara_status_attribute = (1u << 11),
        safety_preset_integrity_attribute = (1u << 12),
        safety_preset_id_selected_attribute = (1u << 13),
        safety_preset_id_used_attribute = (1u << 14),
        soc_fusa_events_attribute = (1u << 15),
        soc_fusa_action_attribute = (1u << 16),
        soc_status_attribute = (1u << 17),
        mb_fusa_event_attribute = (1u << 18),
        mb_fusa_action_attribute = (1u << 19),
        mb_status_attribute = (1u << 20),
        smcu_status_attribute = (1u << 21),
        smcu_state_attribute = (1u << 22),
        non_fusa_gpio_attribute = (1u << 29),
        smcu_debug_info_attribute = (1u << 30),
        crc32_attribute = (1u << 31)
    };

    /**\brief md_occupancy_attributes - bit mask to find active attributes,
        *   md_occupancy struct */
    enum class md_occupancy_attributes : uint32_t
    {
        frame_counter_attribute = (1u << 0),
        depth_frame_counter_attribute = (1u << 1),
        frame_timestamp_attribute = (1u << 2),
        floor_detection_attribute = (1u << 3),
        diagnostic_zone_fill_rate_attribute = (1u << 4),
        depth_fill_rate_attribute = (1u << 5),
        sensor_roll_angle_attribute = (1u << 6),
        sensor_pitch_angle_attribute = (1u << 7),
        diagnostic_zone_median_height_attribute = (1u << 8),
        depth_stdev_attribute = (1u << 9), /* Only a placeholder. Not used by HKR. */
        safety_preset_info = (1u << 10),
        grid_rows_attribute = (1u << 11),
        grid_columns_attribute = (1u << 12),
        cell_size_attribute = (1u << 13),
        danger_zone = (1u << 14),
        warning_zone = (1u << 15),
        diagnostic_zone = (1u << 16),
        payload_crc32_attribute = (1u << 31)
    };

    /**\brief md_point_cloud_attributes - bit mask to find active attributes,
        *   md_point_cloud struct */
    enum class md_point_cloud_attributes : uint32_t
    {
        frame_counter_attribute = (1u << 0),
        depth_frame_counter_attribute = (1u << 1),
        frame_timestamp_attribute = (1u << 2),
        floor_detection_attribute = (1u << 3),
        diagnostic_zone_fill_rate = (1u << 4),
        depth_fill_rate_attribute = (1u << 5),
        sensor_roll_angle_attribute = (1u << 6),
        sensor_pitch_angle_attribute = (1u << 7),
        diagnostic_zone_median_height_attribute = (1u << 8),
        depth_stdev_attribute = (1u << 9), /* Only a placeholder. Not used by HKR. */
        safety_preset_info = (1u << 10),
        number_of_3d_vertices_attribute = (1u << 11),
        danger_zone = (1u << 12),
        warning_zone = (1u << 13),
        diagnostic_zone = (1u << 14),
        payload_crc32_attribute = (1u << 31)
    };

#pragma pack(push, 1)

    /**\brief md_safety_info - Safety Frame info */
    /* Bitmasks details:
    * uint32_t    vision_safety_verdict;        Bitmask:
                                                (0x1 << 0) - Depth Visual Safety Verdict(0:Safe, 1 : Not Safe)
                                                (0x1 << 1) Collision(s) identified in Danger Safety Zone
                                                (0x1 << 2) Collision(s) identified in Warning Safety Zone
                                                Setting bit_0 = 1 (high) implies having one or more of mask bits[1 - 2] set to 1 as well
    * uint16_t    vision_hara_status;           Bitmask:
                                                (0x1 << 0) - HaRa triggers identified(1 - yes; 0 - no)
                                                (0x1 << 1) - Collision(s) identified in Danger Safety Zone
                                                (0x1 << 2) - Collision(s) identified in Warning Safety Zone
                                                (0x1 << 3) - Depth Fill Rate is lower than the require confidence level(1 - yes; 0 - no)
                                                (0x1 << 4) - Floor Detection(fill rate) is lower than the required confidence level(1 - yes; 0 - no)
                                                (0x1 << 5) - (Reserved for) Cliff detection was triggered(1 - yes; 0 - no).opt*
                                                (0x1 << 6) - (Reserved for)  Depth noise standard deviation  is higher than permitted level(1 - yes; 0 - no).opt*
                                                (0x1 << 7) - Camera Posture / Floor position critical deviation is detected(calibration offset)
                                                (0x1 << 8) - Safety Preset error (1 - yes; 0 - no)
                                                (0x1 << 9) – Image Depth Fill Rate is lower than the require confidence level (1 - yes; 0 - no)
                                                (0x1 << 10) – Frame drops/insufficient data (1 - yes; 0 - no)
                                                bits[11..15] are reserved and must be zeroed
                                                Setting bit_0 = 1 (high)implies having one or more of mask bits[1 - 8] set to 1 as well
    * uint8_t     safety_preset_integrity;      Bitmask:
                                                (0x1 << 0) - Preset Integrity / inconsistency identified
                                                (0x1 << 1) - Preset CRC check invalid
                                                (0x1 << 2) - Discrepancy between actual and expected Preset Id.
                                                Setting bit_0 = 1 (high)implies having one or more of mask bits[1 - 2] are set to 1 as well

    * uint32_t    soc_monitor_l2_error_type;    Current Soc Monitor Escalation of L2 error to Arbiter (bitMask):
                                                L2_ERROR_TYPES_NO_ERROR = 0
                                                L2_ERROR_TYPES_L0_PASS_CNT = 1 <<0
                                                L2_ERROR_TYPES_L1_PASS_CNT = 1 <<1,
                                                L2_ERROR_TYPES_L0_PASS_RATE = 1 <<2,
                                                L2_ERROR_TYPES_L1_PASS_RATE = 1 <<3,
                                                L2_ERROR_TYPES_STL_TIMEOUT= 1 <<4,
                                                L2_ERROR_TYPES_STL_FAILED= 1 <<5"

    * uint32_t    soc_monitor_l3_error_type;    Current Soc Monitor Escalation of L3 error to Arbiter (bitMask):
                                                L3_ERROR_TYPES_NO_ERROR = 0
                                                L3_ERROR_L2_PASS_CNT = 1 <<0

    * uint32_t    mb_fusa_events                Bitmask:
                                                MB_FUSA_EVENT_NOT_SAFE_BIT	                1LU << 0U
                                                MB_FUSA_EVENT_ADC1_OV_BIT	                1LU << 1U
                                                MB_FUSA_EVENT_ADC1_UV_BIT	                1LU << 2U
                                                MB_FUSA_EVENT_ADC2_OV_BIT	                1LU << 3U
                                                MB_FUSA_EVENT_ADC2_UV_BIT	                1LU << 4U
                                                MB_FUSA_EVENT_ADC3_OV_BIT	                1LU << 5U
                                                MB_FUSA_EVENT_ADC3_UV_BIT	                1LU << 6U
                                                MB_FUSA_EVENT_ADC4_OV_BIT	                1LU << 7U
                                                MB_FUSA_EVENT_ADC4_UV_BIT	                1LU << 8U
                                                MB_FUSA_EVENT_ADC5_OV_BIT	                1LU << 9U
                                                MB_FUSA_EVENT_ADC5_UV_BIT	                1LU << 10U
                                                MB_FUSA_EVENT_ADC6_OV_BIT	                1LU << 11U
                                                MB_FUSA_EVENT_ADC6_UV_BIT	                1LU << 12U
                                                MB_FUSA_EVENT_ADC7_OV_BIT	                1LU << 13U
                                                MB_FUSA_EVENT_ADC7_UV_BIT	                1LU << 14U
                                                MB_FUSA_EVENT_PVT_0_OV_BIT	                1LU << 15U
                                                MB_FUSA_EVENT_PVT_0_UV_BIT	                1LU << 16U
                                                MB_FUSA_EVENT_PVT_1_OV_BIT	                1LU << 17U
                                                MB_FUSA_EVENT_PVT_1_UV_BIT	                1LU << 18U
                                                MB_FUSA_EVENT_PVT_2_OV_BIT	                1LU << 19U
                                                MB_FUSA_EVENT_PVT_2_UV_BIT	                1LU << 20U
                                                MB_FUSA_EVENT_PVT_3_OV_BIT	                1LU << 21U
                                                MB_FUSA_EVENT_PVT_3_UV_BIT	                1LU << 22U
                                                MB_FUSA_EVENT_PVT_4_OV_BIT	                1LU << 23U
                                                MB_FUSA_EVENT_PVT_4_UV_BIT	                1LU << 24U
                                                MB_FUSA_EVENT_PVT_5_OV_BIT	                1LU << 25U
                                                MB_FUSA_EVENT_PVT_5_UV_BIT	                1LU << 26U
                                                MB_FUSA_EVENT_PVT_6_OV_BIT	                1LU << 27U
                                                MB_FUSA_EVENT_PVT_6_UV_BIT	                1LU << 28U
                                                MB_FUSA_EVENT_IR_L_OV_BIT	                1LU << 29U
                                                MB_FUSA_EVENT_IR_L_UV_BIT	                1LU << 30U
                                                MB_FUSA_EVENT_PROJECTOR_L_OV_BIT            1LU << 31U
                                                MB_FUSA_EVENT_PROJECTOR_L_UV_BIT            1LU << 32U
                                                MB_FUSA_EVENT_IMU_SENSOR_OV_BIT	            1LU << 33U
                                                MB_FUSA_EVENT_IMU_SENSOR_UV_BIT	            1LU << 34U
                                                MB_FUSA_EVENT_RGB_SENSOR_OV_BIT	            1LU << 35U
                                                MB_FUSA_EVENT_RGB_SENSOR_UV_BIT	            1LU << 36U
                                                MB_FUSA_EVENT_IR_R_OV_BIT	                1LU << 37U
                                                MB_FUSA_EVENT_IR_R_UV_BIT	                1LU << 38U
                                                MB_FUSA_EVENT_PROJECTOR_R_OV_BIT	        1LU << 39U
                                                MB_FUSA_EVENT_PROJECTOR_R_UV_BIT	        1LU << 40U
                                                MB_FUSA_EVENT_APM_PRIN_L_OV_BIT             1LU << 41U
                                                MB_FUSA_EVENT_APM_PRIN_L_UV_BIT             1LU << 42U
                                                MB_FUSA_EVENT_APM_PRIN_R_OV_BIT             1LU << 43U
                                                MB_FUSA_EVENT_APM_PRIN_R_UV_BIT             1LU << 44U
                                                MB_FUSA_EVENT_THERMAL_OV_BIT                1LU << 45U
                                                MB_FUSA_EVENT_THERMAL_UV_BIT                1LU << 46U
                                                MB_FUSA_EVENT_HUMIDITY_OV_BIT               1LU << 47U
                                                MB_FUSA_EVENT_HUMIDITY_UV_BIT               1LU << 48U
                                                MB_FUSA_EVENT_SMCU_TEMP_OV_BIT              1LU << 49U
                                                MB_FUSA_EVENT_SMCU_TEMP_UV_BIT              1LU << 50U
                                                Setting bit_0=1 (high) implies having one or more of mask bits [1-31] set to 1 as well
                                                bits [15..31] are reserved (for MB/Peripheral Temperature and Humidity sensors) and must be zeroed

    uint8_t     mb_fusa_action                  Bitmask: action taken
                                                0x1 << 0  - HKR Reset
                                                0x1 << 1  - HKR ShutDown


    uint8_t     non_fusa_gpio;                  Bitmask.
                                                0x1 << 0 - OSSD2_A_present
                                                0x1 << 1 - OSSD2_A status: Raised/Idle
                                                0x1 << 2 - OSSD2_B_present
                                                0x1 << 3 - OSSD2_B status: Raised/Idle
                                                0x1 << 4 -Device_Ready_present
                                                0x1 << 5 -Device_Ready on/off
                                                0x1 << 6 -Error signal present
                                                0x1 << 7 -Error signal on/off"

    *
    *
    */ 
    struct md_safety_info
    {
        md_header   header;
        uint32_t    version;
        uint32_t    flags;
        uint32_t    frame_counter;
        uint32_t    depth_frame_counter;
        uint32_t    frame_timestamp;

        // FUSA verdict
        uint8_t     level1_signal;                  // Designates the "Yellow" zone status: 0x1 - High, 0x0 - Low
        uint32_t    level1_frame_counter_origin;    // When L1 is low - equals to frame_counter in safety_header. For L1=0x1: hold the Frame id on last transition to "High" state
        uint8_t     level2_signal;                  // Designates the "Red" zone status: 0x1 - High, 0x0 - Low
        uint32_t    level2_frame_counter_origin;    // When L2 is low - equals to frame_counter in safety_header. For L2=0x1: hold the Frame id on last transition to "High" state
        uint8_t     level1_verdict;                 // Current verdict for l1 Safety Signal - May differ from l1_signal due to additional logics applied
        uint8_t     level2_verdict;                 // Current verdict for l2 Safety Signal - May differ from l2_signal due to additional logics applied
        uint8_t     operational_mode;               // Reflects the SC operational mode (XU control)
        uint8_t     fusa_reserved[15];              // zeroed

        // Vision Monitor
        uint32_t    vision_safety_verdict;          // Bitmask: see details above the struct
        uint16_t    vision_hara_status;             // Bitmask: see details above the struct
        uint8_t     safety_preset_integrity;        // Bitmask: see details above the struct
        uint8_t     safety_preset_id_selected;      // Safety Preset index set via Adaptive Field selection GPIO
        uint8_t     safety_preset_id_used;          // Safety Preset index used in the latest Vision Safety algo processing - retrieved from cpfpVisionSafetyResults message
        uint8_t     vision_reserved[25];            // zeroed

        // SOC Monitor
        uint32_t    soc_fusa_events;                // Bitmask: see details above the struct
        uint8_t     soc_fusa_action;                // Bitmask, enumerated
        uint16_t    soc_l_0_counter;                // Total number of L0 notifications captured
        uint8_t     soc_l_0_rate;                   // L_0 notifications per FDTI
        uint16_t    soc_l_1_counter;                // Total number of L1 notifications captured
        uint8_t     soc_l_1_rate;                   // L_1 notifications per FDTI
        uint8_t     soc_gmt_status;                 // Result, enumerated: 0 - GMT Clock Ok, 1- GMT Clock is outside Safe threshold, 2 - GMT Clock is not available
        uint8_t     soc_hkr_critical_error_gpio;    // Critical-erro GPIO Status: 0 - off, 1 - on
        uint32_t    soc_monitor_l2_error_type;      // Soc Monitor Escalation of L2 error to Arbiter
        uint32_t    soc_monitor_l3_error_type;      // Soc Monitor Escalation of L3 error to Arbiter
        uint8_t     soc_reserved[25];               // zeroed

        // MB Monitor
        uint64_t    mb_fusa_event;                  // Bitmask: see details above the struct
        uint8_t     mb_fusa_action;                 // Bitmask: see details above the struct
        uint32_t    mb_status;                      // Provision for future enhancements
        uint8_t     mb_reserved[15];                // zeroed

        // MCU Monitor
        uint32_t    smcu_status;                    // Bitmask: TBD
        uint8_t     smcu_state;                     // TBD

        // SMCU DEBUG Info
        uint32_t    smcu_status_bitmask;            // Bitmask: The values are state-specific and non-FuSa related. Shall be presented as HEX string
        uint8_t     smcu_internal_state;            // Enumerated: according to S.MCU FSM
        uint8_t     smcu_bist_status;               // Bitmask. To be presented as HEX value

        // OUT
        uint8_t     non_fusa_gpio;                  // Bitmask: see details above the struct

        uint8_t     mcu_reserved[32];               // zeroed
        
        uint32_t    crc32;                          // CRC
    };



    REGISTER_MD_TYPE(md_safety_info, md_type::META_DATA_INTEL_SAFETY_ID)


    struct md_safety_mode
    {
        md_safety_info          intel_safety_info;
    };

    /**\brief md_occupancy - Occupancy Frame */
    struct md_occupancy
    {
        md_header   header;
        uint32_t    version;
        uint32_t    flags;
        uint32_t    frame_counter;
        uint32_t    depth_frame_counter;             // counter of the depth frame upon which it was calculated
        uint64_t    frame_timestamp;                 // HW Timestamp for Occupancy map, calculated in lower level algo
        uint8_t     floor_detection;                 // Percentage
        uint8_t     diagnostic_zone_fill_rate;       // Percentage
        uint8_t     depth_fill_rate;                 // Unsigned value in range of [0..100]. Use [x = 0xFF] if not applicable
        float       sensor_roll_angle;               // In millidegrees. Relative to X (forward) axis. Positive value is CCW
        float       sensor_pitch_angle;              // In millidegrees. Relative to Y (left) axis. Positive value is CCW
        int32_t     diagnostic_zone_median_height;   // In millimeters. Relative to the "leveled pointcloud" CS 
        uint16_t    depth_stdev;                     // Spatial accuracy in millimetric units: 
                                                     // [0..1023] - valid range
                                                     // [1024] - attribute was not calculated / not applicable
                                                     // [1025 - 0xFFFF] undefined / invalid range
                                                     // Only a placeholder. Not used by HKR.
        uint8_t     safety_preset_id;                // Designates the Safety Zone index in [0..63] range used in algo pipe
        uint16_t    safety_preset_error_type;        // Safety Preset Error bitmask
        uint8_t     safety_preset_error_param_1;     // Preset Error Param. Enumerated
        uint8_t     safety_preset_error_param_2;     // Preset Error Param. Enumerated

        int16_t     danger_zone_point_0_x_cord;      // Danger zone point #0, X coord in mm
        int16_t     danger_zone_point_0_y_cord;      // Danger zone point #0, Y coord in mm
        int16_t     danger_zone_point_1_x_cord;      // Danger zone point #1, X coord in mm
        int16_t     danger_zone_point_1_y_cord;      // Danger zone point #1, Y coord in mm
        int16_t     danger_zone_point_2_x_cord;      // Danger zone point #2, X coord in mm
        int16_t     danger_zone_point_2_y_cord;      // Danger zone point #2, Y coord in mm
        int16_t     danger_zone_point_3_x_cord;      // Danger zone point #3, X coord in mm
        int16_t     danger_zone_point_3_y_cord;      // Danger zone point #3, Y coord in mm

        int16_t     warning_zone_point_0_x_cord;     // Warning zone point #0, X coord in mm
        int16_t     warning_zone_point_0_y_cord;     // Warning zone point #0, Y coord in mm
        int16_t     warning_zone_point_1_x_cord;     // Warning zone point #1, X coord in mm
        int16_t     warning_zone_point_1_y_cord;     // Warning zone point #1, Y coord in mm
        int16_t     warning_zone_point_2_x_cord;     // Warning zone point #2, X coord in mm
        int16_t     warning_zone_point_2_y_cord;     // Warning zone point #2, Y coord in mm
        int16_t     warning_zone_point_3_x_cord;     // Warning zone point #3, X coord in mm
        int16_t     warning_zone_point_3_y_cord;     // Warning zone point #3, Y coord in mm

        int16_t     diagnostic_zone_point_0_x_cord;  // Diagnostic zone point #0, X coord in mm
        int16_t     diagnostic_zone_point_0_y_cord;  // Diagnostic zone point #0, Y coord in mm
        int16_t     diagnostic_zone_point_1_x_cord;  // Diagnostic zone point #1, X coord in mm
        int16_t     diagnostic_zone_point_1_y_cord;  // Diagnostic zone point #1, Y coord in mm
        int16_t     diagnostic_zone_point_2_x_cord;  // Diagnostic zone point #2, X coord in mm
        int16_t     diagnostic_zone_point_2_y_cord;  // Diagnostic zone point #2, Y coord in mm
        int16_t     diagnostic_zone_point_3_x_cord;  // Diagnostic zone point #3, X coord in mm
        int16_t     diagnostic_zone_point_3_y_cord;  // Diagnostic zone point #3, Y coord in mm

        uint8_t     reserved[10];                    // Zero-ed
        uint16_t    grid_rows;                       // Number of rows in the grid. Max value is 250 (corresponding to 5M width with 2cm tile)
        uint16_t    grid_columns;                    // Number of columns in the grid. Max value is 320 (corresponding to ~6.5M depth with 2cm tile)
        uint8_t     cell_size;                       // Edge size of each tile, measured in cm
        uint8_t     reserved2[15];                   // Zero-ed
        uint32_t    payload_crc32;                   // Crc32 for the occupancy grid payload data only, not including the metadata header.
    };// Safety Preset at the time of Occupancy grid generation 
    REGISTER_MD_TYPE(md_occupancy, md_type::META_DATA_INTEL_OCCUPANCY_ID)


    /**\brief md_point_cloud - Point Cloud Frame */
    struct md_point_cloud
    {
        md_header   header;
        uint32_t    version;
        uint32_t    flags;
        uint32_t    frame_counter;
        uint32_t    depth_frame_counter;             // counter of the depth frame upon which it was calculated
        uint64_t    frame_timestamp;                 // HW Timestamp for Occupancy map, calculated in lower level algo
        uint8_t     floor_detection;                 // Percentage
        uint8_t     diagnostic_zone_fill_rate;       // Percentage
        uint8_t     depth_fill_rate;                 // Unsigned value in range of [0..100]. Use [x = 0xFF] if not applicable
        float       sensor_roll_angle;               // In millidegrees. Relative to X (forward) axis. Positive value is CCW
        float       sensor_pitch_angle;              // In millidegrees. Relative to Y (left) axis. Positive value is CCW
        int32_t     diagnostic_zone_median_height;   // In millimeters. Relative to the "leveled pointcloud" CS 
        uint16_t    depth_stdev;                     // Spatial accuracy in millimetric units: 
                                                     // [0..1023] - valid range
                                                     // [1024] - attribute was not calculated / not applicable
                                                     // [1025 - 0xFFFF] undefined / invalid range
                                                     // Only a placeholder. Not used by HKR.
        uint8_t     safety_preset_id;                // Designates the Safety Zone index in [0..63] range used in algo pipe
        uint16_t    safety_preset_error_type;        // Safety Preset Error bitmask
        uint8_t     safety_preset_error_param_1;     // Preset Error Param. Enumerated
        uint8_t     safety_preset_error_param_2;     // Preset Error Param. Enumerated

        int16_t     danger_zone_point_0_x_cord;      // Danger zone point #0, X coord in mm
        int16_t     danger_zone_point_0_y_cord;      // Danger zone point #0, Y coord in mm
        int16_t     danger_zone_point_1_x_cord;      // Danger zone point #1, X coord in mm
        int16_t     danger_zone_point_1_y_cord;      // Danger zone point #1, Y coord in mm
        int16_t     danger_zone_point_2_x_cord;      // Danger zone point #2, X coord in mm
        int16_t     danger_zone_point_2_y_cord;      // Danger zone point #2, Y coord in mm
        int16_t     danger_zone_point_3_x_cord;      // Danger zone point #3, X coord in mm
        int16_t     danger_zone_point_3_y_cord;      // Danger zone point #3, Y coord in mm

        int16_t     warning_zone_point_0_x_cord;     // Warning zone point #0, X coord in mm
        int16_t     warning_zone_point_0_y_cord;     // Warning zone point #0, Y coord in mm
        int16_t     warning_zone_point_1_x_cord;     // Warning zone point #1, X coord in mm
        int16_t     warning_zone_point_1_y_cord;     // Warning zone point #1, Y coord in mm
        int16_t     warning_zone_point_2_x_cord;     // Warning zone point #2, X coord in mm
        int16_t     warning_zone_point_2_y_cord;     // Warning zone point #2, Y coord in mm
        int16_t     warning_zone_point_3_x_cord;     // Warning zone point #3, X coord in mm
        int16_t     warning_zone_point_3_y_cord;     // Warning zone point #3, Y coord in mm

        int16_t     diagnostic_zone_point_0_x_cord;  // Diagnostic zone point #0, X coord in mm
        int16_t     diagnostic_zone_point_0_y_cord;  // Diagnostic zone point #0, Y coord in mm
        int16_t     diagnostic_zone_point_1_x_cord;  // Diagnostic zone point #1, X coord in mm
        int16_t     diagnostic_zone_point_1_y_cord;  // Diagnostic zone point #1, Y coord in mm
        int16_t     diagnostic_zone_point_2_x_cord;  // Diagnostic zone point #2, X coord in mm
        int16_t     diagnostic_zone_point_2_y_cord;  // Diagnostic zone point #2, Y coord in mm
        int16_t     diagnostic_zone_point_3_x_cord;  // Diagnostic zone point #3, X coord in mm
        int16_t     diagnostic_zone_point_3_y_cord;  // Diagnostic zone point #3, Y coord in mm

        uint8_t     reserved[10];                    // Zero-ed
        uint32_t    number_of_3d_vertices;           // The max number of points is 320X240
        uint8_t     reserved2[16];                   // Zero-ed
        uint32_t    payload_crc32;                   // Crc32 for the occupancy grid payload data only, not including the metadata header.
    };
    REGISTER_MD_TYPE(md_point_cloud, md_type::META_DATA_INTEL_POINT_CLOUD_ID)


    union md_mapping_mode
    {
        md_occupancy       intel_occupancy;
        md_point_cloud     intel_point_cloud;
    };

#pragma pack(pop)
}
