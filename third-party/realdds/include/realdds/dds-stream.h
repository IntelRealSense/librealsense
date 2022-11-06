// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-stream-base.h"
#include "dds-stream-profile.h"

#include <memory>
#include <string>
#include <vector>

namespace realdds {


// Represents a stream of information (images, motion data, etc..) from a single source received via the DDS system.
// A stream can have several profiles, i.e different data frequency, image resolution, etc..
// 
// This is a base class: you need to specify the type of stream via the instantiation of a video_stream, etc.
//
class dds_stream : public dds_stream_base
{
    typedef dds_stream_base super;

protected:
    dds_stream( std::string const & stream_name, std::string const & sensor_name );

    // dds_stream_base
public:
    bool is_open() const override;
    bool is_streaming() const override;

    std::shared_ptr< dds_topic > const & get_topic() const override;
};

class dds_video_stream : public dds_stream
{
    typedef dds_stream super;

public:
    dds_video_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "video"; }

private:
    class impl;
    std::shared_ptr< impl > _impl;
};

class dds_motion_stream : public dds_stream
{
    typedef dds_stream super;

public:
    dds_motion_stream( std::string const & stream_name, std::string const & sensor_name );

    char const * type_string() const override { return "motion"; }

private:
    class impl;
    std::shared_ptr< impl > _impl;
};


typedef std::vector< std::shared_ptr< dds_stream > > dds_streams;


}  // namespace realdds
