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
    class detected_object
    {
        size_t _id;           // Some unique identifier that assigned to us
        std::string _label;   // Any string assigned to us in the ctor

        cv::Rect _location;   // In the color frame
        cv::Rect _depth_location;   // In the depth frame
        float _intensity;     // Some heuristic calculated on _location, allowing approximate "identity" based on pixel intensity
        float _depth;

    public:
        using ptr = std::shared_ptr< detected_object >;

        explicit detected_object( size_t id, std::string const & label, cv::Rect const & location,
            cv::Rect const & depth_location = cv::Rect {}, float intensity = 1, float depth = 0 );
        virtual ~detected_object() {}

        // Update the location and intensity of the face
        void move( cv::Rect const & location, cv::Rect const & depth_location = cv::Rect {}, float intensity = 1, float depth = 0 )
        {
            _location = location;
            _depth_location = depth_location;
            _intensity = intensity;
            _depth = depth;
        }

        cv::Rect const & get_location() const { return _location; }
        cv::Rect const & get_depth_location() const { return _depth_location; }
        float get_intensity() const { return _intensity; }
        float get_depth() const { return _depth; }
        size_t get_id() const { return _id; }
        std::string const & get_label () const { return _label; }
    };

    typedef std::list< detected_object::ptr > detected_objects;

    // Returns a face found approximately in the given location, or null if none
    detected_object::ptr find_object( cv::Rect rect, detected_objects const & objects );
}
