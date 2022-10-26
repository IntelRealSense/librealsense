// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-stream-profile.h"

#include <functional>
#include <memory>
#include <string>

namespace realdds {


// Represents a stream of information (images, motion data, etc..) from a single source received via the DDS system.
// A stream can have several profiles, i.e different data frequency, image resolution, etc..
class dds_stream
{
public:
    virtual const std::string & get_group_name() const = 0;

    virtual size_t foreach_profile( std::function< void( const dds_stream_profile & prof, bool def_prof ) > fn ) const = 0;

    virtual ~dds_stream() = default;
};

class dds_video_stream : public dds_stream
{
public:
    dds_video_stream( const std::string & group_name );

    const std::string & get_group_name() const override;

    void add_profile( dds_video_stream_profile && prof, bool default_profile );
    size_t foreach_profile( std::function< void( const dds_stream_profile & prof, bool def_prof ) > fn ) const override;

private:
    class impl;
    std::shared_ptr< impl > _impl;
};

class dds_motion_stream : public dds_stream
{
public:
    dds_motion_stream( const std::string & group_name );

    const std::string & get_group_name() const override;

    void add_profile( dds_motion_stream_profile && prof, bool default_profile );
    size_t foreach_profile( std::function< void( const dds_stream_profile & prof, bool def_prof ) > fn ) const override;

private:
    class impl;
    std::shared_ptr< impl > _impl;
};


}  // namespace realdds
