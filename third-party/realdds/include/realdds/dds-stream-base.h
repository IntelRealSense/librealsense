// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include "dds-stream-profile.h"

#include <memory>
#include <string>


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
    int _default_profile_index = 0;
    dds_stream_profiles _profiles;

    dds_stream_base( std::string const & stream_name, std::string const & sensor_name );
    
public:
    virtual ~dds_stream_base() = default;

    // This function can only be called once!
    void init_profiles( dds_stream_profiles const & profiles, int default_profile_index = 0 );

    std::string const & name() const { return _name; }
    std::string const & sensor_name() const { return _sensor_name; }
    dds_stream_profiles const & profiles() const { return _profiles; }
    int default_profile_index() const { return _default_profile_index; }

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
