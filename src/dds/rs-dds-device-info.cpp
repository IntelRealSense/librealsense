// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "rs-dds-device-info.h"
#include "rs-dds-device-proxy.h"

#include <realdds/dds-participant.h>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>

#include <rsutils/string/from.h>


namespace librealsense {


std::shared_ptr< device_interface > dds_device_info::create_device()
{
    return std::make_shared< dds_device_proxy >( shared_from_this(), _dev );
}


std::string dds_device_info::get_address() const
{
    auto const domain_id = _dev->participant()->get()->get_domain_id();

    return rsutils::string::from() << "dds." << domain_id << "://"
                                   << _dev->participant()->print( _dev->server_guid() ) << "@"
                                   << _dev->device_info().topic_root();
}


void dds_device_info::to_stream( std::ostream & os ) const
{
    os << "DDS device (" << _dev->participant()->print( _dev->guid() ) << " on domain "
       << _dev->participant()->get()->get_domain_id() << "):" << std::setw( 4 ) << _dev->device_info().to_json();
}


bool dds_device_info::is_same_as( std::shared_ptr< const device_info > const & other ) const
{
    return get_address() == other->get_address();
}


}  // namespace librealsense
