// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <functional>
#include <memory>
#include <string>

namespace realdds {


// Represents a stream of information (images, motion data, etc..) from a single source received via the DDS system.
// A stream can have several profiles, i.e different data frequency, image resolution, etc..
class dds_stream
{
public:
    struct profile
    {
        int16_t uid;       // Stream unique ID
        int16_t framerate; // FPS
        int8_t type;       // Kind of video, motion etc..
        int8_t format;     // Data format specification
        int8_t index;      // Used to distinguish similar streams like IR L / R
    };

    virtual int8_t get_type() const = 0;
    virtual const std::string & get_group_name() const = 0;

    virtual void add_profile( const dds_stream::profile & prof, bool default_profile ) = 0;
    virtual size_t foreach_profile( std::function< void( const dds_stream::profile & prof, bool def_prof ) > fn ) const = 0;

    virtual ~dds_stream() = default;
};

class dds_video_stream : public dds_stream
{
public:
    struct profile : public dds_stream::profile
    {
        int16_t width;     // Resolution width [pixels]
        int16_t height;    // Resolution height [pixels]
        //intrinsics - TODO
    };

    dds_video_stream( int8_t type, const std::string & group_name );

    int8_t get_type() const override;
    const std::string & get_group_name() const override;

    void add_profile( const dds_stream::profile & prof, bool default_profile ) override;
    size_t foreach_profile( std::function< void( const dds_stream::profile & prof, bool def_prof ) > fn ) const override;

private:
    class impl;
    std::shared_ptr< impl > _impl;
};

class dds_motion_stream : public dds_stream
{
public:
    struct profile : public dds_stream::profile
    {
    };

    dds_motion_stream( int8_t type, const std::string & group_name );

    int8_t get_type() const override;
    const std::string & get_group_name() const override;

    void add_profile( const dds_stream::profile & prof, bool default_profile );
    size_t foreach_profile( std::function< void( const dds_stream::profile & prof, bool def_prof ) > fn ) const override;

private:
    class impl;
    std::shared_ptr< impl > _impl;
};


}  // namespace realdds
