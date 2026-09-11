// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.
#pragma once

#include "d585s-md.h"
#include <cstring>
#include <src/frame.h>

namespace librealsense
{
// Both Mapping products carry the same capture identity in their UVC metadata.
// Linux's UVCH format may expose only the 12-byte base header; MAP1 then retains
// the source identity/time even when host deliveries were dropped.
inline bool get_mapping_capture_timing( const frame &f, uint32_t &counter, uint64_t &timestamp )
{
    auto const &data = f.additional_data;
    constexpr size_t offset = platform::uvc_header_size;
    if ( data.metadata_size >= offset + sizeof( md_occupancy ) &&
         data.metadata_size <= data.metadata_blob.size() )
    {
        md_occupancy md;
        std::memcpy( &md, data.metadata_blob.data() + offset, sizeof( md ) );
        if ( ( md.header.md_type_id == md_type::META_DATA_INTEL_OCCUPANCY_ID ||
               md.header.md_type_id == md_type::META_DATA_INTEL_POINT_CLOUD_ID ) &&
             md.header.md_size >= sizeof( md ) && md.header.md_size <= data.metadata_size - offset &&
             ( md.flags & 5u ) == 5u && md.frame_timestamp != 0 )
        {
            counter = md.frame_counter;
            timestamp = md.frame_timestamp;
            return true;
        }
    }

    auto const size = f.get_frame_data_size();
    auto const bytes = f.get_frame_data();
    if ( !bytes || size < 44 )
        return false;
    uint32_t magic, payload_size;
    uint16_t version, profile;
    std::memcpy( &magic, bytes, sizeof( magic ) );
    std::memcpy( &version, bytes + 4, sizeof( version ) );
    std::memcpy( &payload_size, bytes + 8, sizeof( payload_size ) );
    std::memcpy( &profile, bytes + 12, sizeof( profile ) );
    bool const lpcl = bytes[6] == 1 && profile == 0x0101;
    bool const og = bytes[6] == 2 && profile == 0x0201;
    if ( magic != 0x3150414d || version != 0x0100 || ( !lpcl && !og ) ||
         payload_size != static_cast<uint32_t>( size - 20 ) || ( og && size < 52 ) )
        return false;
    uint16_t width, height, stride, label_stride;
    uint32_t count;
    std::memcpy( &width, bytes + 20, sizeof( width ) );
    std::memcpy( &height, bytes + 22, sizeof( height ) );
    std::memcpy( &stride, bytes + ( lpcl ? 24 : 26 ), sizeof( stride ) );
    std::memcpy( &label_stride, bytes + 26, sizeof( label_stride ) );
    std::memcpy( &count, bytes + ( lpcl ? 28 : 48 ), sizeof( count ) );
    if ( !width || !height || count != uint32_t( width ) * height ||
         ( lpcl && ( stride != 12 || label_stride != 1 ) ) || ( og && stride != 1 ) ||
         uint64_t( count ) * ( lpcl ? 13u : 1u ) != uint64_t( size - ( lpcl ? 44 : 52 ) ) )
        return false;
    std::memcpy( &counter, bytes + ( lpcl ? 32 : 36 ), sizeof( counter ) );
    std::memcpy( &timestamp, bytes + ( lpcl ? 36 : 40 ), sizeof( timestamp ) );
    return timestamp != 0;
}
} // namespace librealsense
