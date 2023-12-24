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
        cliff_detection_attribute = (1u << 4),
        depth_fill_rate_attribute = (1u << 5),
        sensor_roll_angle_attribute = (1u << 6),
        sensor_pitch_angle_attribute = (1u << 7),
        floor_median_height_attribute = (1u << 8),
        depth_stdev_mm_attribute = (1u << 9),
        safety_preset_id_attribute = (1u << 10),
        grid_rows_attribute = (1u << 11),
        grid_columns_attribute = (1u << 12),
        cell_size_attribute = (1u << 13),
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
        cliff_detection_attribute = (1u << 4),
        depth_fill_rate_attribute = (1u << 5),
        sensor_roll_angle_attribute = (1u << 6),
        sensor_pitch_angle_attribute = (1u << 7),
        floor_median_height_attribute = (1u << 8),
        depth_stdev_mm_attribute = (1u << 9),
        safety_preset_id_attribute = (1u << 10),
        number_of_3d_vertices_attribute = (1u << 11),
        payload_crc32_attribute = (1u << 31)
    };

#pragma pack(push, 1)

    /**\brief md_safety_info - Safety Frame info */
    /* Bitmasks details:
    * uint32_t    vision_safety_verdict;          // Bitmask:
                                                  // (0x1 << 0) - Depth Visual Safety Verdict(0:Safe, 1 : Not Safe)
                                                  // (0x1 << 1) Collision(s) identified in Danger Safety Zone
                                                  // (0x1 << 2) Collision(s) identified in Warning Safety Zone
                                                  // Setting bit_0 = 1 (high) implies having one or more of mask bits[1 - 2] set to 1 as well
    * uint16_t    vision_hara_status;             // Bitmask:
                                                  // (0x1 << 0) - HaRa triggers identified(1 - yes; 0 - no)
                                                  // (0x1 << 1) - Collision(s) identified in Danger Safety Zone
                                                  // (0x1 << 2) - Collision(s) identified in Warning Safety Zone
                                                  // (0x1 << 3) - Depth Fill Rate is lower than the require confidence level(1 - yes; 0 - no)
                                                  // (0x1 << 4) - Floor Detection(fill rate) is lower than the required confidence level(1 - yes; 0 - no)
                                                  // (0x1 << 5) - (Reserved for) Cliff detection was triggered(1 - yes; 0 - no).opt*
                                                  // (0x1 << 6) - (Reserved for)  Depth noise standard deviation  is higher than permitted level(1 - yes; 0 - no).opt*
                                                  // (0x1 << 7) - Camera Posture / Floor position critical deviation is detected(calibration offset)
                                                  // (0x1 << 8) - Safety Preset error (1 - yes; 0 - no)
                                                  // (0x1 << 9) – Image Depth Fill Rate is lower than the require confidence level (1 - yes; 0 - no)
                                                  // (0x1 << 10) – Frame drops/insufficient data (1 - yes; 0 - no)
                                                  // bits[11..15] are reserved and must be zeroed
                                                  // Setting bit_0 = 1 (high)implies having one or more of mask bits[1 - 8] set to 1 as well
    * uint8_t     safety_preset_integrity;        // Bitmask:
                                                  // (0x1 << 0) - Preset Integrity / inconsistency identified
                                                  // (0x1 << 1) - Preset CRC check invalid
                                                  // (0x1 << 2) - Discrepancy between actual and expected Preset Id.
                                                  // Setting bit_0 = 1 (high)implies having one or more of mask bits[1 - 2] are set to 1 as well
    *
    * uint32_t    mb_fusa_events;                 // Bitmask:
                                                  // (0x1 <<0) -Motherboard FuSa Verdict (0:Safe, 1: Not Safe)
                                                  // (0x1 <<1) ADC1 - over-voltage (T10/DIV_3.3V)
                                                  // (0x1 <<2) ADC1 - under-voltage (T10/DIV_3.3V)
                                                  // (0x1 <<3) ADC2 - over-voltage. (U10/VDD_1.8V)
                                                  // (0x1 <<4) ADC2 - under-voltage (U10/VDD_1.8V)
                                                  // (0x1 <<5) ADC3 - over-voltage (W9/VDD_1.2V)
                                                  // (0x1 <<6) ADC3 - under-voltage (W9/VDD_1.2V)
                                                  // (0x1 <<7) ADC4 - over-voltage (U9/VDD_1.1V)
                                                  // (0x1 <<8) ADC4 - under-voltage (U9/VDD_1.1V)
                                                  // (0x1 <<9) ADC5 - over-voltage (T9/VDD_0.8V)
                                                  // (0x1 <<10) ADC5 - under-voltage (T9/VDD_0.8V)
                                                  // (0x1 <<11) ADC6 - over-voltage (Y9/VDD_0.6V)
                                                  // (0x1 <<12) ADC6 - under-voltage (Y9/VDD_0.6V)
                                                  // (0x1 <<13) ADC7 - over-voltage (T8/VDD_5V)
                                                  // (0x1 <<14) ADC7 - under-voltage (T8/VDD_5V)
                                                  // (0x1 <<15) #define MB_FUSA_EVENT_IR_R_O_POS                    (15U)
                                                  // (0x1 <<16) #define MB_FUSA_EVENT_IR_R_U_POS                    (16U)
                                                  // (0x1 <<17) #define MB_FUSA_EVENT_IR_L_O_POS                    (17U)
                                                  // (0x1 <<18) #define MB_FUSA_EVENT_IR_L_U_POS                    (18U)
                                                  // (0x1 <<19) #define MB_FUSA_EVENT_RGB_SENSOR_O_POS              (19U)
                                                  // (0x1 <<20) #define MB_FUSA_EVENT_RGB_SENSOR_U_POS              (20U)
                                                  // (0x1 <<21) #define MB_FUSA_EVENT_APM_PRIN_L_O_POS              (21U)
                                                  // (0x1 <<22) #define MB_FUSA_EVENT_APM_PRIN_L_U_POS              (22U)
                                                  // (0x1 <<23) #define MB_FUSA_EVENT_APM_PRIN_R_O_POS              (23U)
                                                  // (0x1 <<24) #define MB_FUSA_EVENT_APM_PRIN_R_U_POS              (24U)
                                                  // (0x1 <<25) #define MB_FUSA_EVENT_THERMAL_O_POS                 (25U)
                                                  // (0x1 <<26) #define MB_FUSA_EVENT_THERMAL_U_POS                 (26U)
                                                  // (0x1 <<27) #define MB_FUSA_EVENT_HUMIDITY_O_POS                (27U)
                                                  // (0x1 <<28) #define MB_FUSA_EVENT_HUMIDITY_U_POS                (28U)
                                                  // (0x1 <<28) #define MB_FUSA_EVENT_APM_MCU_O_POS                 (29U)
                                                  // (0x1 <<29) #define MB_FUSA_EVENT_APM_MCU_U_POS                 (30U)
                                                  // (0x1 <<30) #define MB_FUSA_EVENT_SMCU_TEMP_O_POS               (31U)
                                                  // (0x1 <<31) #define MB_FUSA_EVENT_SMCU_TEMP_U_POS               (32U)
                                                  // Setting bit_0=1 (high) implies having one or more of mask bits [1-31] set to 1 as well
                                                  // bits [15..31] are reserved (for MB/Peripheral Temperature and Humidity sensors) and must be zeroed

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
        uint8_t     level1_signal;                  // Designates the “Yellow” zone status: 0x1 – High, 0x0 - Low 
        uint32_t    level1_frame_counter_origin;    // When l1 is low – equals to frame_counter in safety_header - For l1=0x1 : hold the Frame id on last transition to “High” state 
        uint8_t     level2_signal;                  // Designates the “Red” zone status: 0x1 – High, 0x0 - Low 
        uint32_t    level2_frame_counter_origin;    // When l2 is low – equals to frame_counter in safety_header - For l2=0x1 : hold the Frame id on last transition to “High” state 
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
        uint8_t     vision_reserved[11];            // zeroed
        // Soc Monitor
        uint32_t    soc_fusa_events;                // Bitmask: see details above the struct
        uint8_t     soc_fusa_action;                // Bitmask, enumerated
        uint64_t    soc_status;                     // Bitmask, enumerated  uint8_ soc_status[8] in ICD
        uint8_t     soc_reserved[15];               // zeroed
        // MB Monitor
        uint64_t    mb_fusa_event;                  // Bitmask: see details above the struct
        uint8_t     mb_fusa_action;                 // Bitmask
        uint32_t    mb_status;                      // Provision for future enhancements
        uint8_t     mb_reserved[11];                // zeroed
        // MCU Monitor
        uint32_t    smcu_status;                    // Bitmask
        uint8_t     smcu_state;                     // Bitmask
        uint8_t     mcu_reserved[15];               // zeroed
        // CRC
        uint32_t    crc32;
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
        uint8_t     cliff_detection;                 // Percentage
        uint8_t     depth_fill_rate;                 // signed value in range of [0..100]. Use [x = 101] if not applicable
        int32_t     sensor_roll_angle;               // In millidegrees. Relative to X (forward) axis. Positive value is CCW
        int32_t     sensor_pitch_angle;              // In millidegrees. Relative to Y (left) axis. Positive value is CCW 
        int32_t     floor_median_height;             // In millimeters. Relative to the “leveled pointcloud” CS 
        uint16_t    depth_stdev;                     // Spatial accuracy in millimetric units: 
                                                     // [0..1023] - valid range
                                                     // [1024] - attribute was not calculated / not applicable
                                                     // [1025 - 0xFFFF] undefined / invalid range
        uint8_t     safety_preset_id;                // Designates the Safety Zone index in [0..63] range used in algo pipe
        uint8_t     reserved[14];                    // Zero-ed
        uint16_t    grid_rows;                       // Number of rows in the grid. Max value is 250 (corresponding to 5M width with 2cm tile) 
        uint16_t    grid_columns;                    // Number of columns in the grid. Max value is 320 (corresponding to ~6.5M depth with 2cm tile) 
        uint8_t     cell_size;                       // Edge size of each tile, measured in cm 
        uint8_t     reserved2[15];                    // Zero-ed
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
        uint8_t     cliff_detection;                 // Percentage
        uint8_t     depth_fill_rate;                 // signed value in range of [0..100]. Use [x = 101] if not applicable
        int32_t     sensor_roll_angle;               // In millidegrees. Relative to X (forward) axis. Positive value is CCW
        int32_t     sensor_pitch_angle;              // In millidegrees. Relative to Y (left) axis. Positive value is CCW 
        int32_t     floor_median_height;             // In millimeters. Relative to the “leveled pointcloud” CS 
        uint16_t    depth_stdev;                     // Spatial accuracy in millimetric units: 
                                                     // [0..1023] - valid range
                                                     // [1024] - attribute was not calculated / not applicable
                                                     // [1025 - 0xFFFF] undefined / invalid range
        uint8_t     safety_preset_id;                // Designates the Safety Zone index in [0..63] range used in algo pipe
        uint8_t     reserved[14];                    // Zero-ed
        uint16_t    number_of_3d_vertices;           // The max number of points is 320X240 
        uint8_t     reserved2[18];                   // Zero-ed
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
