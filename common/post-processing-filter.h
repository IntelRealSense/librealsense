// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>  // for projection utilities
#include <model-views.h>
#include <viewer.h>


/*
    Generic base class for any custom post-processing filter that is added after all
    the usual supported filters.

    A worker thread does the main job in the background, once start() is called.
    See object_detection_filters.
*/
class post_processing_filter : public rs2::filter
{
    std::string _name;
    bool _pb_enabled;

protected:
    // Default ctor to use with post_processing_filters_list::register_filter() cannot take
    // anything but a name. This name is what the user will see in the list of post-processors.
    post_processing_filter( std::string const & name )
        : rs2::filter( [&]( rs2::frame f, rs2::frame_source & source ) {
                // Process the frame, take the result and post it for the next post-processor
                source.frame_ready( process_frameset( f.as< rs2::frameset >() ) );
            } )
        , _name( name )
        , _pb_enabled( true )
    {
    }

    bool is_pb_enabled() const { return _pb_enabled; }

public:
    // Called from post_processing_filter_model when this particular processing block
    // is enabled or disabled
    virtual void on_processing_block_enable( bool e )
    {
        _pb_enabled = e;
    }

    // Returns the name of the filter
    std::string const & get_name() const { return _name; }

    virtual void start( rs2::subdevice_model & model )
    {
        LOG(INFO) << "Starting " + get_name();
    }

protected:
    // Main handler for each frameset we get
    virtual rs2::frameset process_frameset( rs2::frameset fs ) = 0;


    // Helper fn to get the frame context when logs are written, e.g.:
    //     LOG(ERROR) << get_context(fs) << "just a dummy error";
    std::string get_context( rs2::frame f = rs2::frame() ) const
    {
        std::ostringstream ss;
        if( f )
            ss << "[#" << f.get_frame_number() << "] ";
        return ss.str();
    }


    // Returns the intrinsics and extrinsics for a depth and color frame.
    void get_trinsics(
        rs2::video_frame cf, rs2::depth_frame df,
        rs2_intrinsics & color_intrin, rs2_intrinsics & depth_intrin,
        rs2_extrinsics & color_extrin, rs2_extrinsics & depth_extrin )
    {
        rs2::video_stream_profile color_profile = cf.get_profile().as< rs2::video_stream_profile >();
        color_intrin = color_profile.get_intrinsics();
        if( df )
        {
            rs2::video_stream_profile depth_profile = df.get_profile().as< rs2::video_stream_profile >();
            depth_intrin = depth_profile.get_intrinsics();
            depth_extrin = depth_profile.get_extrinsics_to( color_profile );
            color_extrin = color_profile.get_extrinsics_to( depth_profile );
        }
    }


    // Returns a bounding box, input in the color-frame coordinates, after converting it to the corresponding
    // coordinates in the depth frame.
    rs2::rect project_rect_to_depth(
        rs2::rect const & bbox, rs2::depth_frame df,
        rs2_intrinsics const & color_intrin, rs2_intrinsics const & depth_intrin,
        rs2_extrinsics const & color_extrin, rs2_extrinsics const & depth_extrin )
    {
        // NOTE: this is a bit expensive as it actually searches along a projected beam from 0.1m to 10 meter
        float top_left_depth[2], top_left_color[2] = { bbox.x, bbox.y };
        float bottom_right_depth[2], bottom_right_color[2] = { bbox.x + bbox.w, bbox.y + bbox.h };
        rs2_project_color_pixel_to_depth_pixel( top_left_depth,
            reinterpret_cast<const uint16_t *>(df.get_data()), df.get_units(), 0.1f, 10,
            &depth_intrin, &color_intrin,
            &color_extrin, &depth_extrin,
            top_left_color );
        rs2_project_color_pixel_to_depth_pixel( bottom_right_depth,
            reinterpret_cast<const uint16_t *>(df.get_data()), df.get_units(), 0.1f, 10,
            &depth_intrin, &color_intrin,
            &color_extrin, &depth_extrin,
            bottom_right_color );
        float left = top_left_depth[0];
        float top = top_left_depth[1];
        float right = bottom_right_depth[0];
        float bottom = bottom_right_depth[1];
        return rs2::rect { left, top, right - left, bottom - top };
    }
};
