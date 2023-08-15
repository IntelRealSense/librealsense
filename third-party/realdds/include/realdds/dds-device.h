// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include "dds-stream-profile.h"
#include "dds-stream.h"

#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace realdds {

namespace topics {
class device_info;
}  // namespace topics


class dds_participant;

// Represents a device via the DDS system. Such a device exists as of its identification by the device-watcher, and
// always contains a device-info and GUID of the remote DataWriter to which it belongs.
// 
// The device may not be ready for use (will not contain sensors, profiles, etc.) until it received all handshake
// notifications from the server, but it will start receiving notifications and be able to send controls right away.
//
class dds_device
{
public:
    static std::shared_ptr< dds_device > find( dds_guid const & guid );

    static std::shared_ptr< dds_device > create( std::shared_ptr< dds_participant > const & participant,
                                                 dds_guid const & guid,
                                                 topics::device_info const & info );

    std::shared_ptr< dds_participant > const & participant() const;
    std::shared_ptr< dds_subscriber > const & subscriber() const;
    topics::device_info const & device_info() const;

    // The device GUID is that of the DataWriter which declares it!
    dds_guid const & guid() const;

    // A device is ready for use after it's gone through handshake and can start streaming
    bool is_ready() const;

    // Wait until ready. Will throw if not ready within the timeout!
    void wait_until_ready( size_t timeout_ns = 5000 );

    //----------- below this line, a device must be running!

    size_t number_of_streams() const;

    size_t foreach_stream( std::function< void( std::shared_ptr< dds_stream > stream ) > fn ) const;
    size_t foreach_option( std::function< void( std::shared_ptr< dds_option > option ) > fn ) const;

    void open( const dds_stream_profiles & profiles );

    void set_option_value( const std::shared_ptr< dds_option > & option, float new_value );
    float query_option_value( const std::shared_ptr< dds_option > & option );

    bool has_extrinsics() const;
    std::shared_ptr< extrinsics > get_extrinsics( std::string const & from, std::string const & to ) const;

    bool supports_metadata() const;

    typedef std::function< void( nlohmann::json && md ) > on_metadata_available_callback;
    void on_metadata_available( on_metadata_available_callback cb );

    typedef std::function< void(
        dds_time const & timestamp, char type, std::string const & text, nlohmann::json const & data ) >
        on_device_log_callback;
    void on_device_log( on_device_log_callback cb );

private:
    class impl;
    std::shared_ptr< impl > _impl;

    // Ctor is private: use find() or create() instead. Same for dtor -- it should be automatic
    dds_device( std::shared_ptr< impl > );

    //Called internally by other functions, mutex should be locked prior to calling this function
    //Solves possible race conditions when serching for an item and creating if does not exist.
    static std::shared_ptr< dds_device > find_internal( dds_guid const & guid );
};  // class dds_device


}  // namespace realdds
