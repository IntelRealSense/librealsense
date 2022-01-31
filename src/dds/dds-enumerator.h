// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

namespace eprosima
{
    namespace fastdds
    {
        namespace dds
        {
            class DomainParticipant;
        }
    }
}

namespace librealsense {

    
class dds_enumerator
{
public:
    dds_enumerator();
    ~dds_enumerator();
    void init();

private:
    eprosima::fastdds::dds::DomainParticipant* participant_;
};
}  // namespace librealsense
