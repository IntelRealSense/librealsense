// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <list>
#include <opencv2/core/types.hpp>  // for cv::Rect


namespace openvino_helpers
{
    /*
        Describe detected face
    */
    class detected_face
    {
        size_t _id;           // Some unique identifier that assigned to us
        float _age;
        float _maleScore;     // Cumulative (see updateGender)
        float _femaleScore;   // Cumulative

        cv::Rect _location;   // In the color frame
        cv::Rect _depth_location;   // In the depth frame
        float _intensity;     // Some heuristic calculated on _location, allowing approximate "identity" based on pixel intensity
        float _depth;

    public:
        using ptr = std::shared_ptr< detected_face >;

        explicit detected_face( size_t id, cv::Rect const & location, cv::Rect const & depth_location = cv::Rect {}, float intensity = 1, float depth = 0, float male_score = 0.5, float age = 0 );

        // Update the location and intensity of the face
        void move( cv::Rect const & location, cv::Rect const & depth_location = cv::Rect {}, float intensity = 1, float depth = 0 )
        {
            _location = location;
            _depth_location = depth_location;
            _intensity = intensity;
            _depth = depth;
        }
        // Update the age of the face
        void update_age( float value );
        // Update the gender of the face (detection is not accurate and so this can
        // change over time -- we calculate an "average")
        void update_gender( float value );

        cv::Rect const & get_location() const { return _location; }
        cv::Rect const & get_depth_location() const { return _depth_location; }
        float get_intensity() const { return _intensity; }
        float get_depth() const { return _depth; }
        int get_age() const { return static_cast<int>(std::floor( _age + 0.5f )); }
        bool is_male() const { return _maleScore > _femaleScore; }
        bool is_female() const { return !is_male(); }
        size_t get_id() const { return _id; }
    };

    typedef std::list< detected_face::ptr > detected_faces;

    // Returns a face found approximately in the given location, or null if none
    detected_face::ptr find_face( cv::Rect rect, detected_faces const & faces );
}
