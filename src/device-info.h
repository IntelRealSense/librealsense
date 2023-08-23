// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <iosfwd>
#include <string>


namespace librealsense {


class context;
class device_interface;


// Pointer to device information, from which the device can actually be created.
// Returned by context::query_devices().
// 
class device_info : public std::enable_shared_from_this< device_info >
{
    std::shared_ptr< context > _ctx;

protected:
    explicit device_info( std::shared_ptr< context > const & ctx )
        : _ctx( ctx )
    {
    }

public:
    virtual ~device_info() = default;

    // The device-info always contains a context, since a device is always (except maybe a software device) within some
    // context.
    std::shared_ptr< context > const & get_context() const { return _ctx; }

public:
    // A one-line identifier that can be used to "point" to this device
    virtual std::string get_address() const = 0;

    // In-depth dump, possibly multi-line (default to the address)
    virtual void to_stream( std::ostream & ) const;

    // Equality function
    virtual bool is_same_as( std::shared_ptr< const device_info > const & ) const = 0;

    // Device creation. The device will assume ownership of the device-info.
    // Note that multiple devices can be created, in theory, for the same device-info. Usually this would be avoided by
    // proper use of the equality function.
    virtual std::shared_ptr< device_interface > create_device() = 0;
};


inline std::ostream & operator<<( std::ostream & os, device_info const & dev_info )
{
    dev_info.to_stream( os );
    return os;
}


}  // namespace librealsense
