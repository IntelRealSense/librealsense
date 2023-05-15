/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2022 Intel Corporation. All Rights Reserved. */

   /** \file rs_safety_types.h
   * \brief
   * Exposes RealSense safety structs
   */

#ifndef LIBREALSENSE_RS2_SAFETY_TYPES_H
#define LIBREALSENSE_RS2_SAFETY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#pragma pack(push, 1)
// Convenience blocks
typedef struct sc_float2 { float x, y; } sc_float2;
typedef struct sc_float3 { float x, y, z; } sc_float3;
typedef struct sc_float3x3 { sc_float3 x, y, z; } sc_float3x3;  // column-major

typedef struct rs2_safety_extrinsics_table
{
    sc_float3x3 rotation; // Rotation matrix Sensor->Robot CS (""Leveled World"" assumed)
    sc_float3 translation; // Metric units
} rs2_safety_extrinsics_table;

enum rs2_safety_mos_type_value
{
    mos_hand = 0,
    mos_leg = 1,
    mos_body = 2,
    mos_max = 3,
};

typedef uint8_t rs2_safety_mos_type;

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
    rs2_safety_extrinsics_table transformation_link; // Sensor->System Rigid-body Transformation (System CS is ""Leveled World"" assumed)
    float robot_height; // meters. Used to calculate the max height for collision scanning
    float robot_mass; // mass of SRSS (kg) - For reference only
    uint8_t reserved[16];  // Can be modified  by changing the table minor version, without breaking back - compat
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

    sc_float2 minimum_object_size; // MOS is two-dimentional: {diameter (mm), length (mm) }
    rs2_safety_mos_type mos_target_type; // enumerated- hand/leg/body
    uint8_t reserved[8]; // Can be modified  by changing the table minor version, without breaking back-compat
} rs2_safety_zone;

typedef struct rs2_safety_2d_masking_zone
{
    uint16_t attributes; // Bitmask, enumerated: (0x1 <<0) - is_valid: specifies whether the specifc zone was defined by user in this preset
    float minimal_range; // in meters in forward direction (leveled CS)
    sc_2d_pixel region_of_interests[4]; // (i,j) pairs of 4 two-dimentional quadrilateral points
} rs2_safety_2d_masking_zone;

typedef struct rs2_safety_environment
{
    float grid_cell_size; // Denotes the square tile size for occupancy grid in cm (square)
    float safety_trigger_duration; // duration in seconds to keep safety signal high after safety MCU is back to normal

    // Platform dynamics properties
    float linear_velocity; // m/sec
    float angular_velocity; // rad/sec
    float payload_weight; // a typical mass of the carriage payload in kg

    // Environmetal properties
    float surface_inclination; // expected floor min/max inclination angle, degrees
    float  surface_height; // min height above surface to be used for obstacle avoidance (meter)
    uint8_t surface_confidence; // min fill rate required for safe floor detection [0...100%]

    uint8_t             reserved[15]; // Can be modified  by changing the table minor version, without breaking back-compat
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

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif
