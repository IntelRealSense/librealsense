// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"

#include <memory>


namespace librealsense {
namespace dds {


namespace topics {
class device_info;
}  // namespace topics
class dds_participant;


class dds_device
{
    class impl;
    std::shared_ptr< impl > _impl;

    // Ctor is private: use find() or create() instead. Same for dtor -- it should be automatic
    dds_device( std::shared_ptr< impl > );
    ~dds_device();

    static std::shared_ptr< dds::dds_device > find_or_create_dds_device( std::shared_ptr< dds::dds_participant > const &,
                                                                         dds::dds_guid const &,
                                                                         dds::topics::device_info const & info,
                                                                         bool create_it );

public:
    static std::shared_ptr< dds_device > find( std::shared_ptr< dds::dds_participant > const & participant,
                                               dds::dds_guid const & guid,
                                               dds::topics::device_info const & info )
    {
        return find_or_create_dds_device( participant, guid, info, false );
    }
    static std::shared_ptr< dds_device > create( std::shared_ptr< dds::dds_participant > const & participant,
                                                 dds::dds_guid const & guid,
                                                 dds::topics::device_info const & info )
    {
        return find_or_create_dds_device( participant, guid, info, true );
    }

public:
    topics::device_info const & device_info() const;
};  // class dds_device


}  // namespace dds
}  // namespace librealsense
