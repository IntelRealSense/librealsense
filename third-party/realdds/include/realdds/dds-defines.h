// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>


namespace eprosima {
namespace fastrtps {
namespace rtps {
    struct GUID_t;
    struct GuidPrefix_t;
    struct EntityId_t;
}  // namespace rtps
struct Time_t;
namespace types {
    class DynamicType_ptr;
}  // namespace types
}  // namespace fastrtps
}  // namespace eprosima

namespace realdds {


using dds_time = eprosima::fastrtps::Time_t;
using dds_nsec = int64_t;  // the type returned from dds_time::to_ns()
using dds_guid = eprosima::fastrtps::rtps::GUID_t;
using dds_guid_prefix = eprosima::fastrtps::rtps::GuidPrefix_t;
using dds_entity_id = eprosima::fastrtps::rtps::EntityId_t;
typedef int dds_domain_id;
typedef uint64_t dds_sequence_number;  // sample_identity.sequence_number()


}  // namespace realdds
