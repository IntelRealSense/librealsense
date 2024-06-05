/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2022-2024 Intel Corporation. All Rights Reserved. */

   /** \file rs_safety_types.h
   * \brief
   * Exposes RealSense safety structs
   */

#ifndef LIBREALSENSE_RS2_SAFETY_TYPES_H
#define LIBREALSENSE_RS2_SAFETY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

#pragma pack(push, 1)
// Convenience blocks
typedef struct sc_float2 { float x, y; } sc_float2;

typedef struct sc_2d_pixel
{
    uint16_t i;
    uint16_t j;
} sc_2d_pixel;

typedef struct rs2_safety_preset_header
{
    uint16_t version; // major.minor. Big-endian
    uint16_t table_type; // Safety Preset type
    uint32_t table_size; // full size including: header footer
    uint32_t crc32; // crc of all the data in table excluding this header/CRC
} rs2_safety_preset_header;

typedef struct rs2_safety_platform
{
    rs2_extrinsics_row_major transformation_link; // Sensor->System Rigid-body Transformation (System CS is ""Leveled World"" assumed)
    float robot_height; // Meters; above ground level. Used to calculate the max height for collision scanning
    uint8_t reserved[20];  // Can be modified  by changing the table minor version, without breaking back - compat
} rs2_safety_platform;

typedef struct rs2_safety_zone
{
    sc_float2 zone_polygon[4]; // The zone polygon (area) is defined by its four corners, ordered
                               // Internal requirements: 
                               // - Trinagular or simple quadrilateral shape
                               // - Concave or convex, self-intersections not allowed
                               // - Area must be greater than zero, thus at least 3 out of 4 vertices must be different

    // Safety Zone Actuator properties
    uint8_t safety_trigger_confidence; // number of consecutive frames to raise safety signal

    uint8_t reserved[7]; // Can be modified  by changing the table minor version, without breaking back-compat
} rs2_safety_zone;

typedef struct rs2_safety_2d_masking_zone
{
    uint16_t attributes; // Bitmask, enumerated: (0x1 <<0) - is_valid: specifies whether the specifc zone was defined by user in this preset
    float minimal_range; // in meters in forward direction (leveled CS)
    sc_2d_pixel region_of_interests[4]; // (i,j) pairs of 4 two-dimentional quadrilateral points
} rs2_safety_2d_masking_zone;

typedef struct rs2_safety_environment
{
    float safety_trigger_duration; // duration in seconds to keep safety signal high after safety MCU is back to normal

    // Platform dynamics properties
    uint8_t zero_safety_monitoring;     // 0 - Regular (default). All Safety Mechanisms activated in nominal mode.
                                        // 1 - "Zero Safety" mode.Continue monitoring but Inhibit triggering OSSD on Vision HaRA event + Collision Detections
    uint8_t hara_history_continuation; // 0 - Regular or Global History. Vision HaRa Metrics are continued when switching Safety Preset. (default value)
                                        // 1 - No History.When toggling Safety Preset all Vision HaRa metrics based on multiple sampling is being reset.
                                        // 2 - Local History*, or History per Safety Preset.In this mode Vision HaRa metrics history is tracked per each preset 
                                        // individually.When toggling Safety Presets S.MCU to check if that particular preset has history and if so - take that into consideration when reaching FuSa decision.
                                        // In case the particular Safety Preset had no recorded history tracking then the expected behavior is similar to #1.
    uint8_t reserved1[2];
    float angular_velocity; // rad/sec
    float payload_weight;   // a typical mass of the carriage payload in kg

    // Environmetal properties
    float   surface_inclination;                      // Expected floor min/max inclination angle, degrees
    float   surface_height;                           // Min height above surface to be used for obstacle avoidance (meter)
    uint8_t diagnostic_zone_fill_rate_threshold;      // Min pixel fill rate required in the diagnostic area for Safety Algo processing [0...100%]
    uint8_t floor_fill_threshold;                     // Depth fill rate threshold for the floor area [0...100%]
    uint8_t depth_fill_threshold;                     // Depth Fill rate threshold for the full image [0...100%]
    uint8_t diagnostic_zone_height_median_threshold;  // Diagnostic Zone height median in [ 0..255 ] millimeter range. Absolute
    uint8_t vision_hara_persistency;                  // Represents the number of consecutive frames with alarming Vision HaRa parameters to raise safety signal [1..5]. Default = 1
    uint8_t crypto_signature[32];                     // SHA2 or similar

    uint8_t reserved[3];                              // Can be modified by changing the table minor version, without breaking back-compat
} rs2_safety_environment;


typedef struct rs2_safety_preset
{
    rs2_safety_platform platform_config; // left camera intrinsic data, normilized
    rs2_safety_zone safety_zones[2]; // Zones: 0 - Danger; 1- Warning;
    rs2_safety_2d_masking_zone masking_zones[8]; // Masking Zones #0-7
    uint8_t reserved[16]; // Can be modified  by changing the table minor version, without breaking back-compat
    rs2_safety_environment environment; // Provides input for Zone planning and safety algo execution
} rs2_safety_preset;

typedef struct rs2_safety_preset_with_header
{
    rs2_safety_preset_header header;
    rs2_safety_preset safety_preset;
} rs2_safety_preset_with_header;

typedef struct rs2_safety_occupancy_grid_params
{
    uint16_t grid_cell_seed;      // Grid cell (quadratic) edge size in mm

    // This comment is effective for the next 3 range quorum members
    // Each range represnt the number of Depth pixels found in a specific cell to mark it as "occupied"
    // The Zone dymanic range is defined as: (min_range <= Z << max_range)
    // The difference between these 3 memebrs, is the range they can define.
    uint8_t  close_range_quorum;  // Occupancy grid close-range [0�1000) mm
    uint8_t  mid_range_quorum;    // Occupancy grid Mid-range [1000�2000) mm
    uint8_t  long_range_quorum;   // Occupancy grid Mid-range [2000�3000) mm

} rs2_safety_occupancy_grid_params;

typedef struct rs2_safety_smcu_arbitration_params
{
    uint16_t l_0_total_threshold;             // In [ 0..2^16-1] range. Default = 100
    uint8_t l_0_sustained_rate_threshold;     // Notifications per FDTI. In [ 1..255] range. Default = 10
    uint16_t l_1_total_threshold;             // In [ 0..2^16-1] range. Default = 100
    uint8_t l_1_sustained_rate_threshold;     // Notifications per FDTI. In [ 1..255] range. Default = 10
    uint8_t l_4_total_threshold;              // Number of HKR reset before switching to "Locked" state. [ 1..255] range. Default = 10
    uint8_t hkr_stl_timeout;                  // In ms
    uint8_t mcu_stl_timeout;                  // Application Specific
    uint8_t sustained_aicv_frame_drops;       // [0..100] % of frames required to arrive within the last 10 frames.
    uint8_t ossd_self_test_pulse_width;       // Application Specific
} rs2_safety_smcu_arbitration_params;

typedef struct rs2_safety_interface_config_header
{
    uint16_t version;       // major.minor. Big-endian
    uint16_t table_type;    // type
    uint32_t table_size;    // full size including: header footer
    uint32_t calib_version; // major.minor.index
    uint32_t crc32;         // crc of all the data in table excluding this header/CRC
} rs2_safety_interface_config_header;

typedef enum rs2_safety_pin_direction
{
    RS2_SAFETY_PIN_DIRECTION_INPUT,
    RS2_SAFETY_PIN_DIRECTION_OUTPUT,
    RS2_SAFETY_PIN_DIRECTION_COUNT
} rs2_safety_pin_direction;
const char* rs2_safety_pin_direction_to_string(rs2_safety_pin_direction direction);

typedef enum rs2_safety_pin_functionality
{
    RS2_SAFETY_PIN_FUNCTIONALITY_GND = 0,
    RS2_SAFETY_PIN_FUNCTIONALITY_P24VDC,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD1_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD1_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD2_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD2_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD2_A_FEEDBACK,
    RS2_SAFETY_PIN_FUNCTIONALITY_OSSD2_B_FEEDBACK,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT1_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT1_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT2_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT2_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT3_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT3_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT4_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT4_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT5_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT5_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT6_A,
    RS2_SAFETY_PIN_FUNCTIONALITY_PRESET_SELECT6_B,
    RS2_SAFETY_PIN_FUNCTIONALITY_DEVICE_READY,
    RS2_SAFETY_PIN_FUNCTIONALITY_ERROR,
    RS2_SAFETY_PIN_FUNCTIONALITY_RESET,
    RS2_SAFETY_PIN_FUNCTIONALITY_RESTART_INTERLOCK,
    RS2_SAFETY_PIN_FUNCTIONALITY_COUNT
} rs2_safety_pin_functionality;
const char* rs2_safety_pin_functionality_to_string(rs2_safety_pin_functionality functionality);

typedef struct rs2_safety_interface_config_pin
{
    uint8_t direction;      // see rs2_safety_pin_direction enum for available valid values
    uint8_t functionality;  // see rs2_safety_pin_functionality enum for available valid values 
} rs2_safety_interface_config_pin;

typedef struct rs2_safety_interface_config
{
    rs2_safety_interface_config_pin power;
    rs2_safety_interface_config_pin ossd1_b;
    rs2_safety_interface_config_pin ossd1_a;
    rs2_safety_interface_config_pin preset3_a;
    rs2_safety_interface_config_pin preset3_b;
    rs2_safety_interface_config_pin preset4_a;
    rs2_safety_interface_config_pin preset1_b;
    rs2_safety_interface_config_pin preset1_a;
    rs2_safety_interface_config_pin gpio_0;
    rs2_safety_interface_config_pin gpio_1;
    rs2_safety_interface_config_pin gpio_3;
    rs2_safety_interface_config_pin gpio_2;
    rs2_safety_interface_config_pin preset2_b;
    rs2_safety_interface_config_pin gpio_4;
    rs2_safety_interface_config_pin preset2_a;
    rs2_safety_interface_config_pin preset4_b;
    rs2_safety_interface_config_pin ground;
    uint8_t gpio_stabilization_interval; // Time for GPIOs to stabilize before accepting the new Selection Zone index. In Millisec[15 - 150]
    rs2_extrinsics_row_major camera_position; // Sensor->System Rigid-body Transformation (System CS is "Leveled World" assumed)
    rs2_safety_occupancy_grid_params occupancy_grid_params;
    rs2_safety_smcu_arbitration_params smcu_arbitration_params;
    uint8_t crypto_signature[32]; // SHA2 or similar
    uint8_t reserved[17]; // Zeroed. Can be modified by changing the table minor version, without breaking back-compat
} rs2_safety_interface_config;

typedef struct rs2_safety_interface_config_with_header
{
    rs2_safety_interface_config_header header;
    rs2_safety_interface_config payload;
    
} rs2_safety_interface_config_with_header;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif
