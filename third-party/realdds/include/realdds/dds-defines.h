// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <librealsense2/utilities/json.h>

namespace eprosima {
namespace fastrtps {
namespace rtps {
    struct GUID_t;
    struct GuidPrefix_t;
    struct EntityId_t;
    class Time_t;
}  // namespace rtps
namespace types {
    class DynamicType_ptr;
}  // namespace types
}  // namespace fastrtps
}  // namespace eprosima

namespace realdds {


using dds_time = eprosima::fastrtps::rtps::Time_t;
using dds_nsec = int64_t;  // the type returned from dds_time::to_ns()
using dds_guid = eprosima::fastrtps::rtps::GUID_t;
using dds_guid_prefix = eprosima::fastrtps::rtps::GuidPrefix_t;
using dds_entity_id = eprosima::fastrtps::rtps::EntityId_t;
typedef int dds_domain_id;


// Internal properties of a video stream.
// The sensor has a lens and thus has properties such as a focal point, distortion, and principal point.
struct video_intrinsics
{
    int   width = 0;                    // Horizontal resolution, in pixels
    int   height = 0;                   // Vertical resolution, in pixels
    float principal_point_x = 0.0f;     // Pixel offset from the left edge
    float principal_point_y = 0.0f;     // Pixel offset from the top edge
    float focal_lenght_x = 0.0f;        // As a multiple of pixel width
    float focal_lenght_y = 0.0f;        // As a multiple of pixel height
    int   distortion_model = 0;         // Distortion model of the image
    std::array< float, 5 > distortion_coeffs = { 0 }; // Distortion model coefficients

    bool is_valid() const { return focal_lenght_x > 0 && focal_lenght_y > 0; }
    bool operator<( const video_intrinsics & rhs ) const {
        return width < rhs.width || ( width == rhs.width && height < rhs.height );
    }

    nlohmann::json to_json() const;
    static video_intrinsics from_json( nlohmann::json const & j );
};

// Internal properties of a motion stream.
// Contain scale, bias and variances for each dimension.
struct motion_intrinsics
{
    std::array< std::array< float, 4 >, 3 > data = { 0 }; // float[3][4] array
    std::array< float, 3 > noise_variances = { 0 };
    std::array< float, 3 > bias_variances = { 0 };

    nlohmann::json to_json() const;
    static motion_intrinsics from_json( nlohmann::json const & j );
};


// Cross-stream extrinsics: encodes the topology describing how the different devices are oriented.
struct extrinsics
{
    std::array< float, 9 > rotation;    // Column-major 3x3 rotation matrix
    std::array< float, 3 > translation; // Three-element translation vector, in meters

    nlohmann::json to_json() const;
    static extrinsics from_json( nlohmann::json const & j );
};


// Maps < from stream_name, to stream_name > pairs to extrinsics
typedef std::map< std::pair< std::string, std::string >, std::shared_ptr< extrinsics > > extrinsics_map;


}  // namespace realdds
