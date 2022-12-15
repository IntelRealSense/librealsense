// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SAFETY_TYPES_HPP
#define LIBREALSENSE_RS2_SAFETY_TYPES_HPP

#include "../h/rs_safety_types.h"

inline std::ostream& operator<<(std::ostream& ss, sc_float2 const& obj) {
	ss << "{ x:" << obj.x << ", y:" << obj.y << " }\n";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, sc_float3 const& obj) {
	ss << "{ x:" << obj.x << ", y:" << obj.y << ", z:" << obj.z << " }";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, sc_float3x3 const& obj) {
	ss << obj.x << "\n";
	ss << obj.y << "\n";
	ss << obj.z << "\n";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, rs2_safety_extrinsics_table const& obj) {
	ss << "Rotation:\n" << obj.rotation << "\n";
	ss << "Translation:\n" << obj.translation << "\n";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, rs2_safety_platform const& obj) {
	ss << "Transformation Link:\n" << obj.transformation_link << "\n";
	ss << "Robot Height" << ": " << obj.robot_height << "\n";
	ss << "Robot Mass" << ": " << obj.robot_mass << "\n";
	ss << "\t" << "Reserved[16]" << ": " << obj.reserved << "\n";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, rs2_safety_zone const& obj) {
	ss << "Flags" << ": " << obj.flags << "\n";
	ss << "Zone Type" << ": " << obj.zone_type << "\n";
	ss << "Zone Polygon [0]" << ": " << obj.zone_polygon[0] << "\n";
	ss << "Zone Polygon [1]" << ": " << obj.zone_polygon[1] << "\n";
	ss << "Zone Polygon [2]" << ": " << obj.zone_polygon[2] << "\n";
	ss << "Zone Polygon [3]" << ": " << obj.zone_polygon[3] << "\n";
	ss << "Masking Zone V Boundary" << ": " << obj.masking_zone_v_boundary << "\n";
	ss << "Safety Trigger Confidence" << ": " << obj.safety_trigger_confidence << "\n";
	ss << "Miminum Object Size" << ": " << obj.miminum_object_size << "\n";
	ss << "MOS Target Type" << ": " << obj.mos_target_type << "\n";
	ss << "Reserved[16]" << ": " << obj.reserved << "\n";
	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, rs2_safety_environment const& obj) {
	ss << "Grid Cell Size" << ": " << obj.grid_cell_size << "\n";
	ss << "Safety Trigger Duration" << ": " << obj.safety_trigger_duration << "\n";
	ss << "Max Linear Velocity" << ": " << obj.max_linear_velocity << "\n";
	ss << "Max Angular Velocity" << ": " << obj.max_angular_velocity << "\n";
	ss << "Payload Weight" << ": " << obj.payload_weight << "\n";
	ss << "Surface Inclination" << ": " << obj.surface_inclination << "\n";
	ss << "Surface Height" << ": " << obj.surface_height << "\n";
	ss << "Surface Confidence" << ": " << obj.surface_confidence << "\n";
	ss << "Reserved[16]" << ": " << obj.reserved << "\n";

	return ss;
}

inline std::ostream& operator<<(std::ostream& ss, rs2_safety_preset const& obj) {
	ss << "Safety Preset:\n";
	ss << "Platform Config:\n";
	ss << obj.platform_config << "\n";
	ss << "Safety Zone[0]:" << "\n";
	ss << obj.safety_zones[0] << "\n";
	ss << "Safety Zone[1]:" << "\n";
	ss << obj.safety_zones[1] << "\n";
	ss << "Safety Zone[2]:" << "\n";
	ss << obj.safety_zones[2] << "\n";
	ss << "Safety Zone[3]:" << "\n";
	ss << obj.safety_zones[3] << "\n";
	ss << "Environment:" << "\n";
	ss << obj.environment << "\n";
	return ss;
}

#endif // LIBREALSENSE_RS2_SAFETY_TYPES_HPP
