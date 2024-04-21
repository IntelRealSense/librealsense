// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include <realdds/dds-stream-profile.h>
#include <realdds/dds-option.h>
#include <realdds/dds-defines.h>

#include <memory>
#include <string>
#include <vector>


namespace realdds {


class dds_topic;


// Base class for both client/subscriber and server/publisher stream implementations: contains
// information needed to identify a stream, its properties, and its profiles.
//
class dds_stream_base : public std::enable_shared_from_this< dds_stream_base >
{
protected:
    std::string const _name;
    std::string const _sensor_name;
    size_t _default_profile_index = 0;
    dds_stream_profiles _profiles;
    dds_options _options;
    std::vector< std::string > _recommended_filters;
    bool _metadata_enabled = false;

    dds_stream_base( std::string const & stream_name, std::string const & sensor_name );
    
public:
    virtual ~dds_stream_base() = default;

    // Init functions can only be called once!
    void enable_metadata(); // Must call before init_profiles
    void init_profiles( dds_stream_profiles const & profiles, size_t default_profile_index = 0 );
    void init_options( dds_options const & options );
    void set_recommended_filters( std::vector< std::string > && recommended_filters );

    std::string const & name() const { return _name; }
    std::string const & sensor_name() const { return _sensor_name; }
    dds_stream_profiles const & profiles() const { return _profiles; }
    size_t default_profile_index() const { return _default_profile_index; }
    dds_options const & options() const { return _options; }
    std::vector< std::string > const & recommended_filters() const { return _recommended_filters; }
    bool metadata_enabled() const { return _metadata_enabled; }

    std::shared_ptr< dds_stream_profile > default_profile() const
    {
        std::shared_ptr< dds_stream_profile > profile;
        if( default_profile_index() < profiles().size() )
            profile = profiles()[default_profile_index()];
        return profile;
    }

    // For serialization, we need a string representation of the stream type (also the profile types)
    virtual char const * type_string() const = 0;

    virtual bool is_open() const = 0;
    virtual bool is_streaming() const = 0;

    // Returns the topic - will throw if not open!
    virtual std::shared_ptr< dds_topic > const & get_topic() const = 0;

protected:
    // Allows custom checking of each profile from init_profiles() - if there's a problem, throws
    virtual void check_profile( std::shared_ptr< dds_stream_profile > const & ) const;
};


}  // namespace realdds
