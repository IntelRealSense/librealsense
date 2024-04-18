// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "software-device-info.h"
#include "software-device.h"
#include "librealsense-exception.h"

#include <rsutils/string/from.h>


namespace librealsense {


software_device_info::software_device_info( std::shared_ptr< context > const & ctx )
    : device_info( ctx )
    , _address()  // leave empty until set_device()
{
}


void software_device_info::set_device( std::shared_ptr< software_device > const & dev )
{
    if( ! _address.empty() )
        throw wrong_api_call_sequence_exception( "software_device_info already initialized" );
    _dev = dev;
    _address = rsutils::string::from() << "software-device://" << (unsigned long long)dev.get();
}


bool software_device_info::is_same_as( std::shared_ptr< const device_info > const & other ) const
{
    if( auto rhs = std::dynamic_pointer_cast< const software_device_info >( other ) )
        return _address == rhs->_address;
    return false;
}


std::shared_ptr< device_interface > software_device_info::create_device()
{
    return _dev.lock();
}


}  // namespace librealsense
