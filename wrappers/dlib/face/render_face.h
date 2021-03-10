// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <dlib/image_processing/full_object_detection.h>
#include <dlib/gui_widgets.h>
#include <vector>
#include "markup_68.h"


/*
    Return lines rendering facial landmarks for a 68-point detection, that can
    easily be used with dlib::image_window::add_overlay().

    Based on dlib::render_face_detections(), made a little simpler.
*/
std::vector< dlib::image_window::overlay_line > render_face(
    dlib::full_object_detection const & face,
    dlib::rgb_pixel const & color
)
{
    typedef dlib::image_window::overlay_line overlay_line;
    std::vector< overlay_line > lines;

    // Around Chin. Ear to Ear
    for( unsigned long i = markup_68::JAW_FROM; i < markup_68::JAW_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );

    // Nose
    for( unsigned long i = markup_68::NOSE_RIDGE_FROM; i < markup_68::NOSE_RIDGE_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );
    lines.push_back( overlay_line( face.part( markup_68::NOSE_TIP ), face.part( markup_68::NOSE_BOTTOM_FROM ), color ) );
    lines.push_back( overlay_line( face.part( markup_68::NOSE_TIP ), face.part( markup_68::NOSE_BOTTOM_TO ), color ) );

    // Left eyebrow
    for( unsigned long i = markup_68::RIGHT_EYEBROW_FROM; i < markup_68::RIGHT_EYEBROW_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );

    // Right eyebrow
    for( unsigned long i = markup_68::LEFT_EYEBROW_FROM; i < markup_68::LEFT_EYEBROW_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );

    // Right eye
    for( unsigned long i = markup_68::RIGHT_EYE_FROM; i < markup_68::RIGHT_EYE_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );
    lines.push_back( overlay_line( face.part( markup_68::RIGHT_EYE_FROM ), face.part( markup_68::RIGHT_EYE_TO ), color ) );

    // Left eye
    for( unsigned long i = markup_68::LEFT_EYE_FROM; i < markup_68::LEFT_EYE_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );
    lines.push_back( overlay_line( face.part( markup_68::LEFT_EYE_FROM ), face.part( markup_68::LEFT_EYE_TO ), color ) );

    // Lips inside part
    for( unsigned long i = markup_68::MOUTH_INNER_FROM; i < markup_68::MOUTH_INNER_TO; ++i )
        lines.push_back( overlay_line( face.part( i ), face.part( i + 1 ), color ) );
    lines.push_back( overlay_line( face.part( markup_68::MOUTH_INNER_FROM ), face.part( markup_68::MOUTH_INNER_TO ), color ) );

    return lines;
}
