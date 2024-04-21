// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>

#include <array>
#include <map>
#include <memory>
#include <string>


namespace realdds {


// Internal properties of a video stream.
// The sensor has a lens and thus has properties such as a focal point, distortion, and principal point.
struct video_intrinsics
{
    int width = 0;                                     // Horizontal resolution, in pixels
    int height = 0;                                    // Vertical resolution, in pixels
    float principal_point_x = 0.0f;                    // Pixel offset from the left edge
    float principal_point_y = 0.0f;                    // Pixel offset from the top edge
    float focal_lenght_x = 0.0f;                       // As a multiple of pixel width
    float focal_lenght_y = 0.0f;                       // As a multiple of pixel height
    int distortion_model = 0;                          // Distortion model of the image
    std::array< float, 5 > distortion_coeffs = { 0 };  // Distortion model coefficients

    bool is_valid() const { return focal_lenght_x > 0 && focal_lenght_y > 0; }
    bool operator<( const video_intrinsics & rhs ) const
    {
        return width < rhs.width || ( width == rhs.width && height < rhs.height );
    }

    rsutils::json to_json() const;
    static video_intrinsics from_json( rsutils::json const & j );
};

// Internal properties of a motion stream.
// Contain scale, bias and variances for each dimension.
struct motion_intrinsics
{
    std::array< std::array< float, 4 >, 3 > data = { 0 };  // float[3][4] array
    std::array< float, 3 > noise_variances = { 0 };
    std::array< float, 3 > bias_variances = { 0 };

    rsutils::json to_json() const;
    static motion_intrinsics from_json( rsutils::json const & j );
};


// Cross-stream extrinsics: encodes the topology describing how the different devices are oriented.
struct extrinsics
{
    std::array< float, 9 > rotation = { 0 };     // Column-major 3x3 rotation matrix
    std::array< float, 3 > translation = { 0 };  // Three-element translation vector, in meters

    rsutils::json to_json() const;
    static extrinsics from_json( rsutils::json const & j );
};


// Maps < from stream_name, to stream_name > pairs to extrinsics
typedef std::map< std::pair< std::string, std::string >, std::shared_ptr< extrinsics > > extrinsics_map;


}  // namespace realdds
