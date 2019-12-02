// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <rs-vino/detected-face.h>


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
    detected_face::detected_face(
        size_t id,
        std::string const & label,
        cv::Rect const & location,
        cv::Rect const & depth_location,
        float intensity,
        float depth,
        float male_score,
        float age
    )
        : _location( location )
        , _depth_location( depth_location )
        , _intensity( intensity )
        , _depth( depth )
        , _id( id )
        , _label( label )
        , _age( age )
        , _maleScore( male_score > 0.5f ? male_score - 0.5f : 0.f )
        , _femaleScore( male_score > 0.5f ? 0.f : 0.5f - male_score )
    {
    }


    void detected_face::update_age( float value )
    {
        _age = (_age == -1) ? value : 0.95f * _age + 0.05f * value;
    }


    void detected_face::update_gender(float value)
    {
        if (value < 0)
            return;

        if (value > 0.5) {
            _maleScore += value - 0.5f;
        } else {
            _femaleScore += 0.5f - value;
        }
    }


    detected_face::ptr find_face( cv::Rect rect, detected_faces const & faces )
    {
        detected_face::ptr face_ptr;

        // The more static the face locations, the higher we can set the maxIoU; if a face stays put
        // but grows/shrinks (gets closer/farther), the IoU doesn't change much, but if it moves then
        // the IoU gets small very fast.
        float maxIoU = 0.35f;
            
        for( auto && f : faces )
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
