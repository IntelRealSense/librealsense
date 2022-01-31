// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-enumerator.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
//#include "../polling-device-watcher.h"

using namespace eprosima::fastdds::dds;

namespace librealsense {

dds_enumerator::dds_enumerator()  : participant_(nullptr)
{

}

dds_enumerator::~dds_enumerator() {}

void dds_enumerator::init()
{
    DomainParticipantQos pqos;
}
}  // namespace librealsense
