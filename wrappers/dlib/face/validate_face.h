// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API
#include <dlib/image_processing/full_object_detection.h>
#include "markup_68.h"


/*
    Calculates the average depth for a range of two-dimentional points in face, such that:
        point(n) = face.part(n)
    and puts the result in *p_average_depth.

    Points for which no depth is available (is 0) are ignored and not factored into the average.

    Returns true if an average is available (at least one point has depth); false otherwise.
*/
bool find_depth_from(
    rs2::depth_frame const & frame,
    float const depth_scale,
    dlib::full_object_detection const & face,
    markup_68 markup_from, markup_68 markup_to,
    float * p_average_depth
)
{
    uint16_t const * data = reinterpret_cast<uint16_t const *>(frame.get_data());

    float average_depth = 0;
    size_t n_points = 0;
    for( int i = markup_from; i <= markup_to; ++i )
    {
        auto pt = face.part( i );
        auto depth_in_pixels = *(data + pt.y() * frame.get_width() + pt.x());
        if( !depth_in_pixels )
            continue;
        average_depth += depth_in_pixels * depth_scale;
        ++n_points;
    }
    if( !n_points )
        return false;
    if( p_average_depth )
        *p_average_depth = average_depth / n_points;
    return true;
}


/*
    Returns whether the given 68-point facial landmarks denote the face of a real
    person (and not a picture of one), using the depth data in depth_frame.

    See markup_68 for an explanation of the point topology.

    NOTE: requires the coordinates in face align with those of the depth frame.
*/
bool validate_face(
    rs2::depth_frame const & frame,
    float const depth_scale,
    dlib::full_object_detection const & face
)
{
    // Collect all the depth information for the different facial parts

    // For the ears, only one may be visible -- we take the closer one!
    float left_ear_depth = 100, right_ear_depth = 100;
    if( !find_depth_from( frame, depth_scale, face, markup_68::RIGHT_EAR, markup_68::RIGHT_1, &right_ear_depth )
        && !find_depth_from( frame, depth_scale, face, markup_68::LEFT_1, markup_68::LEFT_EAR, &left_ear_depth ) )
        return false;
    float ear_depth = std::min( right_ear_depth, left_ear_depth );

    float chin_depth;
    if( !find_depth_from( frame, depth_scale, face, markup_68::CHIN_FROM, markup_68::CHIN_TO, &chin_depth ) )
        return false;

    float nose_depth;
    if( !find_depth_from( frame, depth_scale, face, markup_68::NOSE_RIDGE_2, markup_68::NOSE_TIP, &nose_depth ) )
        return false;

    float right_eye_depth;
    if( !find_depth_from( frame, depth_scale, face, markup_68::RIGHT_EYE_FROM, markup_68::RIGHT_EYE_TO, &right_eye_depth ) )
        return false;
    float left_eye_depth;
    if( !find_depth_from( frame, depth_scale, face, markup_68::LEFT_EYE_FROM, markup_68::LEFT_EYE_TO, &left_eye_depth ) )
        return false;
    float eye_depth = std::min( left_eye_depth, right_eye_depth );

    float mouth_depth;
    if( !find_depth_from( frame, depth_scale, face, markup_68::MOUTH_OUTER_FROM, markup_68::MOUTH_INNER_TO, &mouth_depth ) )
        return false;

    // We just use simple heuristics to determine whether the depth information agrees with
    // what's expected: that the nose tip, for example, should be closer to the camera than
    // the eyes.

    // These heuristics are fairly basic but nonetheless serve to illustrate the point that
    // depth data can effectively be used to distinguish between a person and a picture of a
    // person...

    if( nose_depth >= eye_depth )
        return false;
    if( eye_depth - nose_depth > 0.07f )
        return false;
    if( ear_depth <= eye_depth )
        return false;
    if( mouth_depth <= nose_depth )
        return false;
    if( mouth_depth > chin_depth )
        return false;

    // All the distances, collectively, should not span a range that makes no sense. I.e.,
    // if the face accounts for more than 20cm of depth, or less than 2cm, then something's
    // not kosher!
    float x = std::max( { nose_depth, eye_depth, ear_depth, mouth_depth, chin_depth } );
    float n = std::min( { nose_depth, eye_depth, ear_depth, mouth_depth, chin_depth } );
    if( x - n > 0.20f )
        return false;
    if( x - n < 0.02f )
        return false;

    return true;
}
