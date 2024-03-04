// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <realdds/dds-stream-server.h>

#include <map>
#include <vector>
#include <memory>
#include <functional>


namespace realdds {


// Bridge between a device-server that's stream-based to a sensor-based paradigm: with the assumption
// that every stream has an associated sensor, we need a mechanism that translates stream requests to
// sensor requests.
//
// E.g., if a sensor 'Stereo Module' has three streams 'Depth', 'Infrared_1', 'Infrared_2', streaming
// the first will have to start the sensor, then the 2nd will have to use the already-started one. Only
// when all streams are closed will the sensor close.
//
// This is how things usually work in RealSense devices, so this class is useful for anything trying to
// bridge a RealSense device, e.g. a tool like rs-dds-adapter.
//
// By default, we obviously do not know anything about RealSense and therefore do not actually start or
// stop anything; instead we call notification functions that need implementing to customize behavior...
//
class dds_stream_sensor_bridge
{
    struct stream_bridge
    {
        std::shared_ptr< dds_stream_server > server;
        std::shared_ptr< dds_stream_profile > profile;  // locked in when sensor starts
        bool is_explicit = false;   // open() was called for the stream
        bool is_implicit = false;   // by add_implicit_profiles()
        bool is_streaming = false;  // this stream has readers and we need to publish data on it
    };
    struct sensor_bridge
    {
        bool is_streaming = false;  // implies is_committed
        bool is_committed = false;  // true once commit() is called and at least one stream is open/streaming
        std::map< std::string, stream_bridge > streams;

        bool should_stream() const;
        bool should_commit() const;
        void verify_compatible_profile( std::shared_ptr< realdds::dds_stream_profile > const & ) const;
    };

public:
    using readers_changed_callback = dds_stream_server::readers_changed_callback;
    typedef std::function< void( std::string const & sensor_name,
                                 std::vector< std::shared_ptr< realdds::dds_stream_profile > > const & ) >
        start_sensor_callback;
    typedef std::function< void( std::string const & sensor_name ) > stop_sensor_callback;
    typedef std::function< void( std::string const & error_string ) > on_error_callback;

private:
    std::map< std::string, sensor_bridge > _sensors;

    readers_changed_callback _on_readers_changed;
    start_sensor_callback _on_start_sensor;
    stop_sensor_callback _on_stop_sensor;
    on_error_callback _on_error;

public:
    dds_stream_sensor_bridge();
    ~dds_stream_sensor_bridge();

    // Populate with stream-servers.
    // Should be called BEFORE a device-server is initialized with these streams (i.e., before they're opened)
    void init( std::vector< std::shared_ptr< dds_stream_server > > const & streams );

    // Mark a profile as explicit for streaming:
    //      - the profile will be matched against when other profiles are opened, explicitly or implicitly
    //      - this profile will not be changed until reset() or close()
    //      - does not actually change streaming status or the underlying sensor!
    void open( std::shared_ptr< dds_stream_profile > const & );

    // Unmark a stream and restore its profile to the default; does not affect the sensor
    void close( std::shared_ptr< dds_stream_server > const & );

    // Add any implicit profiles to a non-streaming sensor
    //      - ignore committed/streaming sensors
    //      - happens automatically when a reader subscribes to a stream; otherwise calling this is optional
    void add_implicit_profiles();

    // Apply the current state of streaming to any sensors:
    //      - only has effect if stream.is_streaming state requires such a change
    //      - happens automatically when a reader subscribes to a stream; otherwise calling this is optional
    //      - will call on_start_/stop_sensor() notifications
    // The sensor is committed and stream profiles cannot be changed once this is called until reset() is used
    void commit();

    // Bring every stream back to the default profile, and unmark it for streaming
    // NOTE: streams that are streaming (i.e., have readers) cannot be reset except by_force
    void reset( bool by_force = false );

    // Return true if the stream for the given server is currently streaming
    bool is_streaming( std::shared_ptr< dds_stream_server > const & ) const;

    // Notifications
public:
    void on_readers_changed( readers_changed_callback callback ) { _on_readers_changed = std::move( callback ); }
    void on_start_sensor( start_sensor_callback callback ) { _on_start_sensor = std::move( callback ); }
    void on_stop_sensor( stop_sensor_callback callback ) { _on_stop_sensor = std::move( callback ); }
    void on_error( on_error_callback callback ) { _on_error = std::move( callback ); }

    // Impl
protected:
    void on_streaming_needed( stream_bridge &, bool streaming_needed );
    std::shared_ptr< dds_stream_server > find_server( std::shared_ptr< dds_stream_profile > const & );
    void start_sensor( std::string const & sensor_name, sensor_bridge & sensor );
    void stop_sensor( std::string const & sensor_name, sensor_bridge & sensor );
    dds_stream_profiles active_profiles( sensor_bridge const & ) const;
};


}  // namespace realdds
