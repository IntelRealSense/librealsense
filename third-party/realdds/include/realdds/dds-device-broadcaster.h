// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/topics/device-info-msg.h>
#include <realdds/dds-defines.h>
#include <realdds/dds-time.h>

#include <memory>
#include <functional>


namespace realdds {


class dds_publisher;
class dds_topic_writer;


namespace detail {
    class broadcast_manager;
}


// Responsible for broadcasting a device to watchers on the DDS domain.
// 
// We have one topic on which these broadcasts are sent. On the other side, a device-watcher will be listening to pick
// up on broadcasts.
// 
// Two main events need to be picked up by the watcher:
//      - When a device is added, a message containing the device_info is expected or the device cannot be located, and
//        the message must be associated with a GUID for the device
//      - When a device is removed, this needs to be recognized
//          - Note that a device may get disconnected, crash, or otherwise disappear without proper destruction
//          - A message on the topic is therefore not expected
//          - Instead, we rely on the DDS publication-match notifications to tell the client that a writer was removed
// So for each device we maintain a DataWriter that will:
//      - Have its own GUID (supplied by DDS automatically)
//      - Supply its own dis/connect notifications (also supplied automatically)
//
class dds_device_broadcaster : public std::enable_shared_from_this< dds_device_broadcaster >
{
    friend class detail::broadcast_manager;
    std::shared_ptr< detail::broadcast_manager > _manager;

protected:
    topics::device_info _device_info;
    std::shared_ptr< dds_topic_writer > _writer;
    std::function< void() > _on_ack;
    dds_time _ack_timeout;

public:
    // Instantiating a broadcaster is enough to automatically broadcast the presence of the given device-info.
    // If a callback function 'on-ack' is provided, it will be called the first time the device broadcast is
    // acknowledged by any subscribers, AFTER the acknowledgement, so chain of events is guarranteed.
    // The ACK timeout is used only if such a callback is provided. A timeout still calls the callback.
    dds_device_broadcaster( std::shared_ptr< dds_publisher > const &,
                            topics::device_info const &,
                            std::function< void() > && on_ack,
                            dds_time ack_timeout = { 2,0 } );
    virtual ~dds_device_broadcaster();

    // Call to broadcast that the device is about to disconnect and wait for acknowledgements
    virtual bool broadcast_disconnect( dds_time ack_timeout ) const;

protected:
    virtual void broadcast() const;
};


}  // namespace realdds
