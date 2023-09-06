// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-types.h"
#include "../hid/hid-types.h"

#include "hid-device-info.h"
#include "uvc-device-info.h"
#include "playback-device-info.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>


namespace librealsense {


template< class T >
bool list_changed(
    const std::vector< T > & list1,
    const std::vector< T > & list2,
    std::function< bool( T, T ) > equal = []( T first, T second ) { return first == second; } )
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


inline bool operator==( const usb_device_info & a, const usb_device_info & b )
{
    return ( a.id == b.id ) && ( a.vid == b.vid ) && ( a.pid == b.pid ) && ( a.mi == b.mi )
        && ( a.unique_id == b.unique_id ) && ( a.conn_spec == b.conn_spec );
}


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

    backend_device_group( const std::vector< playback_device_info > & playback_devices )
        : playback_devices( playback_devices )
    {
    }

    std::vector< uvc_device_info > uvc_devices;
    std::vector< usb_device_info > usb_devices;
    std::vector< hid_device_info > hid_devices;
    std::vector< playback_device_info > playback_devices;

    bool operator==( const backend_device_group & other ) const
    {
        return ! list_changed( uvc_devices, other.uvc_devices ) && ! list_changed( hid_devices, other.hid_devices )
            && ! list_changed( playback_devices, other.playback_devices );
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

        s += playback_devices.size() > 0 ? "playback devices: \n" : "";
        for( auto playback_device : playback_devices )
        {
            s += playback_device;
            s += "\n\n";
        }

        return s;
    }
};


}  // namespace platform
}  // namespace librealsense
