// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <rs-vino/detected-object.h>


namespace
{
    // Returns <intersection> / <union> of two rectangles ("source" and "destination").
    // The higher the quotient (max 1), the more confidence we have that it's the same
    // rectangle...
    float calcIoU( cv::Rect const & src, cv::Rect const & dst )
    {
        cv::Rect i = src & dst;
        cv::Rect u = src | dst;

        return static_cast<float>(i.area()) / static_cast<float>(u.area());
    }
}


namespace openvino_helpers
{
    detected_object::detected_object(
        size_t id,
        std::string const & label,
        cv::Rect const & location,
        cv::Rect const & depth_location,
        float intensity,
        float depth
    )
        : _location( location )
        , _depth_location( depth_location )
        , _intensity( intensity )
        , _depth( depth )
        , _id( id )
        , _label( label )
    {
    }


    detected_object::ptr find_object( cv::Rect rect, detected_objects const & objects )
    {
        detected_object::ptr face_ptr;

        // The more static the face locations, the higher we can set the maxIoU; if a face stays put
        // but grows/shrinks (gets closer/farther), the IoU doesn't change much, but if it moves then
        // the IoU gets small very fast.
        float maxIoU = 0.35f;
            
        for( auto && f : objects )
        {
            float iou = calcIoU( rect, f->get_location() );
            if( iou > maxIoU )
            {
                face_ptr = f;
                maxIoU = iou;
            }
        }

        return face_ptr;
    }
}
