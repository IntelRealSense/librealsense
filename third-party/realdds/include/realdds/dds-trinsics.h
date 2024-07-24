// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>
#include <rsutils/number/float3.h>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <iosfwd>


namespace realdds {


// Like rs2_distortion
//
enum class distortion_model
{
    none = 0,   // No un/distort
    brown,

    // These are to support librealsense legacy devices (for use in the adapter)
    modified_brown,
    inverse_brown
};

std::ostream & operator<<( std::ostream &, distortion_model );


struct distortion_parameters
{
    distortion_model model;
    std::array< float, 5 > coeffs;

    bool operator==( distortion_parameters const & other ) const
    {
        return model == other.model && coeffs == other.coeffs;
    }
    bool operator!=( distortion_parameters const & other ) const
    {
        return model != other.model || coeffs != other.coeffs;
    }
};

std::ostream & operator<<( std::ostream &, distortion_parameters const & );



// Internal properties of a video stream.
// The sensor has a lens and thus has properties such as a focal point, distortion, and principal point.
struct video_intrinsics
{
    using float2 = rsutils::number::float2;

    int width = 0;                                     // Horizontal resolution, in pixels
    int height = 0;                                    // Vertical resolution, in pixels
    float2 principal_point = { 0, 0 };                 // Pixel offset from top-left edge
    float2 focal_length = { 0, 0 };                    // As a multiple of pixel width and height
    distortion_parameters distortion = { distortion_model::none, { 0 } };
    bool force_symmetry = false;

    bool is_valid() const { return focal_length.x > 0 && focal_length.y > 0; }

    bool operator<( const video_intrinsics & rhs ) const
    {
        // Arrange the intrinsics in order of increasing resolution (width then height)
        return width < rhs.width || ( width == rhs.width && height < rhs.height );
    }
    bool operator==( video_intrinsics const & other ) const
    {
        return width == other.width && height == other.height && principal_point == other.principal_point
            && focal_length == other.focal_length && distortion == other.distortion
            && force_symmetry == other.force_symmetry;
    }
    bool operator!=( video_intrinsics const & other ) const { return ! operator==( other ); }

    video_intrinsics scaled_to( int width, int height ) const;

    static distortion_parameters distortion_from_json( rsutils::json const & );

    rsutils::json to_json() const;
    static video_intrinsics from_json( rsutils::json const & );
    void override_from_json( rsutils::json const & );
};

std::ostream & operator<<( std::ostream &, video_intrinsics const & );


// Internal properties of a motion stream.
// Contain scale, bias and variances for each dimension.
struct motion_intrinsics
{
    std::array< std::array< float, 4 >, 3 > data = { 0 };  // float[3][4] array
    std::array< float, 3 > noise_variances = { 0 };
    std::array< float, 3 > bias_variances = { 0 };

    rsutils::json to_json() const;
    static motion_intrinsics from_json( rsutils::json const & j );

    bool operator!=( motion_intrinsics const & other ) const
    {
        return data != other.data || noise_variances != other.noise_variances || bias_variances != other.bias_variances;
    }
    bool operator==( motion_intrinsics const & other ) const { return ! operator!=( other ); }
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
