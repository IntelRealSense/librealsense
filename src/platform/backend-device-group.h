// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-types.h"
#include "../hid/hid-types.h"

#include "hid-device-info.h"
#include "uvc-device-info.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <algorithm>


namespace librealsense {


template< class T >
bool list_changed(
    const std::vector< T > & list1,
    const std::vector< T > & list2,
    std::function< bool( T const &, T const & ) > equal = []( T const & first, T const & second ) { return first == second; } )
{
    if( list1.size() != list2.size() )
        return true;

    for( auto dev1 : list1 )
    {
        bool found = false;
        for( auto dev2 : list2 )
        {
            if( equal( dev1, dev2 ) )
                found = true;
        }

        if( ! found )
            return true;
    }
    return false;
}


namespace platform {


struct backend_device_group
{
    backend_device_group() {}

    backend_device_group( const std::vector< uvc_device_info > & uvc_devices,
                          const std::vector< usb_device_info > & usb_devices,
                          const std::vector< hid_device_info > & hid_devices )
        : uvc_devices( uvc_devices )
        , usb_devices( usb_devices )
        , hid_devices( hid_devices )
    {
    }

    backend_device_group( const std::vector< usb_device_info > & usb_devices )
        : usb_devices( usb_devices )
    {
    }

    backend_device_group( const std::vector< uvc_device_info > & uvc_devices,
                          const std::vector< usb_device_info > & usb_devices )
        : uvc_devices( uvc_devices )
        , usb_devices( usb_devices )
    {
    }

    std::vector< uvc_device_info > uvc_devices;
    std::vector< usb_device_info > usb_devices;
    std::vector< hid_device_info > hid_devices;

    bool operator==( const backend_device_group & other ) const
    {
        return ! list_changed( uvc_devices, other.uvc_devices ) && ! list_changed( hid_devices, other.hid_devices );
    }

    operator std::string() const
    {
        std::string s;
        s = uvc_devices.size() > 0 ? "uvc devices:\n" : "";
        for( auto uvc : uvc_devices )
        {
            s += uvc;
            s += "\n\n";
        }

        s += usb_devices.size() > 0 ? "usb devices:\n" : "";
        for( auto usb : usb_devices )
        {
            s += usb;
            s += "\n\n";
        }

        s += hid_devices.size() > 0 ? "hid devices: \n" : "";
        for( auto hid : hid_devices )
        {
            s += hid;
            s += "\n\n";
        }

        return s;
    }


    // Returns true if this group is completely accounted for in the right group
    //
    bool is_contained_in( backend_device_group const & second_data ) const
    {
        for( auto & uvc : uvc_devices )
        {
            if( std::find( second_data.uvc_devices.begin(), second_data.uvc_devices.end(), uvc )
                == second_data.uvc_devices.end() )
                return false;
        }
        for( auto & usb : usb_devices )
        {
            if( std::find( second_data.usb_devices.begin(), second_data.usb_devices.end(), usb )
                == second_data.usb_devices.end() )
                return false;
        }
        for( auto & hid : hid_devices )
        {
            if( std::find( second_data.hid_devices.begin(), second_data.hid_devices.end(), hid )
                == second_data.hid_devices.end() )
                return false;
        }
        return true;
    }
};


}  // namespace platform
}  // namespace librealsense
