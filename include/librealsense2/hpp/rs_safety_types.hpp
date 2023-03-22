// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SAFETY_TYPES_HPP
#define LIBREALSENSE_RS2_SAFETY_TYPES_HPP

#include "../h/rs_safety_types.h"
#include <iostream>
#include <sstream>
#include <cstring>

inline std::string sc_reserved_arr_to_string(const uint8_t* data)
{
    std::stringstream ss;
    for (auto i = 0; i < 16; i++)
        ss << " [" << i << "] = " << data[i] << "\t";
    return ss.str();
}

inline std::string sc_zone_type_to_string(rs2_safety_zone_type const& szt) {
    std::string result;
    switch (szt)
    {
    case rs2_safety_zone_type_values::zone_danger:
        result = "DANGER";
        break;
    case rs2_safety_zone_type_values::zone_warning:
        result = "WARNING";
        break;
    case rs2_safety_zone_type_values::zone_mask:
        result = "MASK";
        break;
    default:
        result = "UNKNOWN";
        break;
    }
    return result;
}

inline std::string sc_mos_target_to_string(rs2_safety_mos_type const& mos) {
    std::string result;
    switch (mos)
    {
    case rs2_safety_mos_type_value::mos_hand:
        result = "HAND";
        break;
    case rs2_safety_mos_type_value::mos_leg:
        result = "LEG";
        break;
    case rs2_safety_mos_type_value::mos_body:
        result = "BODY";
        break;
    default:
        result = "UNKNOWN";
        break;
    }
    return result;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_zone const& sz)
{
    out << "\t\t" << "Flags: " << sz.flags;
    out << "\n\t\t" << "Zone Type: " << sc_zone_type_to_string(sz.zone_type);
    out << "\n\t\t" << "Zone Polygon - Point #0: { x:" << sz.zone_polygon[0].x << ", y:" << sz.zone_polygon[0].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #1: { x:" << sz.zone_polygon[1].x << ", y:" << sz.zone_polygon[1].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #2: { x:" << sz.zone_polygon[2].x << ", y:" << sz.zone_polygon[2].y << " }";
    out << "\n\t\t" << "Zone Polygon - Point #3: { x:" << sz.zone_polygon[3].x << ", y:" << sz.zone_polygon[3].y << " }";
    out << "\n\t\t" << "Masking Zone Boundaries - Min Height: " << sz.masking_zone_v_boundary.x << " [m]";
    out << "\n\t\t" << "Masking Zone Boundaries - Max Height: " << sz.masking_zone_v_boundary.y << " [m]";
    out << "\n\t\t" << "Safety Trigger Confidence: " << sz.safety_trigger_confidence;
    out << "\n\t\t" << "Minimum Object Size - Diameter: " << sz.minimum_object_size.x << " [mm]";
    out << "\n\t\t" << "Minimum Object Size - Length: " << sz.minimum_object_size.y << " [mm]";
    out << "\n\t\t" << "Minimum Object Size - Target Type: " << sc_mos_target_to_string(sz.mos_target_type);
    out << "\n\t\t" << "Reserved[16]: " << sc_reserved_arr_to_string(sz.reserved);
    return out;
}
//
inline std::ostream& operator<<(std::ostream& out, rs2_safety_environment const& se)
{
    out << "\t\t" << "Grid Cell Size: " << se.grid_cell_size << " [cm^2]";
    out << "\n\t\t" << "Safety Trigger Duration: " << se.safety_trigger_duration << " [sec]";
    out << "\n\t\t" << "Max Linear Velocity: " << se.max_linear_velocity << " [m/sec]";
    out << "\n\t\t" << "Max Angular Velocity: " << se.max_angular_velocity << " [rad/sec]";
    out << "\n\t\t" << "Payload Weight: " << se.payload_weight << " [kg]";
    out << "\n\t\t" << "Surface Inclination: " << se.surface_inclination << " [deg]";
    out << "\n\t\t" << "Surface Height: " << se.surface_height << " [m]";
    out << "\n\t\t" << "Surface Confidence: " << (int)(se.surface_confidence) << " [%]";
    out << "\n\t\t" << "Reserved[16]: " << sc_reserved_arr_to_string(se.reserved);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_extrinsics_table const& ext)
{
    out << "\t\t\t" << "Rotation:";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.x.x << ", y:" << ext.rotation.x.y << ", z:" << ext.rotation.x.z << " }";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.y.x << ", y:" << ext.rotation.y.y << ", z:" << ext.rotation.y.z << " }";
    out << "\n\t\t\t\t" << "{ x:" << ext.rotation.z.x << ", y:" << ext.rotation.z.y << ", z:" << ext.rotation.z.z << " }";
    out << "\n\t\t\t" << "Translation: { x:" << ext.translation.x << ", y : " << ext.translation.y << ", z : " << ext.translation.z << " }";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, rs2_safety_preset const& sp)
{
    out << "Safety Preset:";
    out << "\n\t" << "Platform Config:";
    out << "\n\t\t" << "Transformation Link:" << "\n" << sp.platform_config.transformation_link;
    out << "\n\t\t" << "Robot Height: " << sp.platform_config.robot_height << " [m]";
    out << "\n\t\t" << "Robot Mass: " << sp.platform_config.robot_mass << " [kg]";
    out << "\n\t\t" << "Resereved[16]: " << sc_reserved_arr_to_string(sp.platform_config.reserved);
    out << "\n\t" << "Safety Zone #0:" << "\n" << sp.safety_zones[0];
    out << "\n\t" << "Safety Zone #1:" << "\n" << sp.safety_zones[1];
    out << "\n\t" << "Safety Zone #2:" << "\n" << sp.safety_zones[2];
    out << "\n\t" << "Safety Zone #3:" << "\n" << sp.safety_zones[3];
    out << "\n\t" << "Environment:" << "\n" << sp.environment;
    return out;
}

inline bool operator==(rs2_safety_preset const& self, rs2_safety_preset const& other)
{
    return !std::memcmp(&self, &other, sizeof(rs2_safety_preset));
}

#endif // LIBREALSENSE_RS2_SAFETY_TYPES_HPP
