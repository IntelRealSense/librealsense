#pragma once

#include "rendering.h"  // for rs2::rect
#include <string>
#include <vector>
#include <mutex>
#include <imgui.h>


struct object_in_frame
{
    rs2::rect normalized_color_bbox, normalized_depth_bbox;
    std::string name;
    float mean_depth;
    size_t id;

    object_in_frame( size_t id, std::string const & name, rs2::rect bbox_color, rs2::rect bbox_depth, float depth )
        : id( id )
        , name( name )
        , normalized_color_bbox( bbox_color )
        , normalized_depth_bbox( bbox_depth )
        , mean_depth( depth )
    {
    }
};


typedef std::vector< object_in_frame > objects_in_frame;


struct atomic_objects_in_frame : public objects_in_frame
{
    std::mutex mutex;
    bool sensor_is_on = true;
};
