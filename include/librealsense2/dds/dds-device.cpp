// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-device.h"
#include "dds-participant.h"
#include "topics/device-info/device-info-msg.h"
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <fastdds/rtps/common/Guid.h>

#include <map>

using namespace eprosima::fastdds::dds;

namespace librealsense {
namespace dds {


namespace {
// The map from guid to device is global
typedef std::map< dds::dds_guid, std::weak_ptr< dds::dds_device > > guid_to_device_map;
guid_to_device_map guid_to_device;
std::mutex devices_mutex;
}  // namespace


class dds_device::impl
{
public:
    topics::device_info _info;
    std::shared_ptr< dds::dds_participant > const _participant;

    impl( std::shared_ptr< dds::dds_participant > const & participant,
          dds::dds_guid const & guid,
          dds::topics::device_info const & info )
        : _info( info )
        , _participant( participant )
    {
        (void)guid;
    }
};


std::shared_ptr< dds::dds_device >
dds_device::find_or_create_dds_device( std::shared_ptr< dds::dds_participant > const & participant,
                                       dds::dds_guid const & guid,
                                       dds::topics::device_info const & info,
                                       bool create_it )
{
    std::weak_ptr< dds::dds_device > wdev;
    std::shared_ptr< dds::dds_device > dev;

    std::lock_guard< std::mutex > lock( devices_mutex );
    auto it = guid_to_device.find( guid );
    if( it != guid_to_device.end() )
    {
        dev = it->second.lock();
        if( ! dev )
        {
            // The device is no longer in use; clear it out
            guid_to_device.erase( it );
        }
    }
    else if( create_it )
    {
        guid_to_device.emplace( guid, dev );
        auto impl = std::make_shared< dds_device::impl >( participant, guid, info );
        // Use a custom deleter to automatically remove the device from the map when it's done with
        dev = std::shared_ptr< dds::dds_device >( new dds_device( impl ), [guid]( dds::dds_device * ptr ) {
            std::lock_guard< std::mutex > lock( devices_mutex );
            guid_to_device.erase( guid );
            delete ptr;
        } );
    }
    return dev;
}


dds_device::dds_device( std::shared_ptr< impl > impl )
    : _impl( impl )
{
}


topics::device_info const& dds_device::device_info() const
{
    return _impl->_info;
}



}  // namespace dds
}  // namespace librealsense