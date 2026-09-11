// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.
//#cmake: static!

#include "../catch.h"
#include <src/ds/d500/mapping-timing.h>
#include <src/stream.h>

using namespace librealsense;

namespace
{
template <class T, class Allocator> void put( std::vector<uint8_t, Allocator> &bytes, size_t offset, T value )
{
    std::memcpy( bytes.data() + offset, &value, sizeof( value ) );
}

std::shared_ptr<frame> mapping_frame( bool lpcl, uint32_t counter, uint64_t timestamp )
{
    auto f = std::make_shared<frame>();
    auto profile = std::make_shared<video_stream_profile>();
    profile->set_format( RS2_FORMAT_Y8 );
    profile->set_stream_type( lpcl ? RS2_STREAM_LABELED_POINT_CLOUD : RS2_STREAM_OCCUPANCY );
    f->set_stream( profile );
    auto &b = f->data;
    b.resize( lpcl ? 70 : 54 ); // two vertices/cells
    put<uint32_t>( b, 0, 0x3150414d );
    put<uint16_t>( b, 4, 0x0100 );
    b[6] = lpcl ? 1 : 2;
    put<uint32_t>( b, 8, b.size() - 20 );
    put<uint16_t>( b, 12, lpcl ? 0x0101 : 0x0201 );
    put<uint16_t>( b, 20, 2 );
    put<uint16_t>( b, 22, 1 );
    put<uint16_t>( b, 24, lpcl ? 12 : 50 );
    put<uint16_t>( b, 26, 1 );
    put<uint32_t>( b, lpcl ? 28 : 48, 2 );
    put<uint32_t>( b, lpcl ? 32 : 36, counter );
    put<uint64_t>( b, lpcl ? 36 : 40, timestamp );
    f->additional_data.metadata_size = 12; // Linux UVCH strips extension bytes
    f->additional_data.metadata_blob[0] = 148;
    return f;
}
} // namespace

TEST_CASE( "Mapping MAP1 preserves hardware FPS across dropped deliveries", "[mapping-timing]" )
{
    for ( bool lpcl : { false, true } )
    {
        auto first = mapping_frame( lpcl, 101, 6400000000ULL );
        auto next = mapping_frame( lpcl, 103, 6400066667ULL );
        uint32_t first_counter = 0, next_counter = 0;
        uint64_t first_timestamp = 0, next_timestamp = 0;
        REQUIRE( get_mapping_capture_timing( *first, first_counter, first_timestamp ) );
        REQUIRE( get_mapping_capture_timing( *next, next_counter, next_timestamp ) );
        CHECK( first_counter == 101 );
        CHECK( next_counter == 103 );
        next->additional_data.frame_number = next_counter;
        next->additional_data.last_frame_number = first_counter;
        next->additional_data.timestamp = next_timestamp * 0.001;
        next->additional_data.last_timestamp = first_timestamp * 0.001;
        CHECK( next->calc_actual_fps() == Catch::Approx( 30. ).margin( 0.001 ) );
        // The synchronous timestamp probe borrows the backend buffer, not a copy.
        frame borrowed;
        borrowed.set_data_size( next->data.size() );
        borrowed.attach_continuation( frame_continuation( []() {}, next->data.data() ) );
        uint32_t counter = 0;
        uint64_t timestamp = 0;
        REQUIRE( get_mapping_capture_timing( borrowed, counter, timestamp ) );
        CHECK( counter == 103 );
        CHECK( timestamp == 6400066667ULL );
    }
}

TEST_CASE( "Mapping validates extent, layout and missing capture time", "[mapping-timing]" )
{
    for ( bool lpcl : { false, true } )
    {
        uint32_t counter = 0;
        uint64_t timestamp = 0;
        auto good = mapping_frame( lpcl, 7, 1000000 );
        for ( size_t size = 0; size < good->data.size(); ++size )
        {
            auto short_frame = mapping_frame( lpcl, 7, 1000000 );
            short_frame->data.resize( size );
            CHECK_FALSE( get_mapping_capture_timing( *short_frame, counter, timestamp ) );
        }
        for ( size_t offset : { size_t( 0 ), size_t( 4 ), size_t( 6 ), size_t( 8 ), size_t( 12 ),
                                size_t( 20 ), size_t( 26 ) } )
        {
            auto bad = mapping_frame( lpcl, 7, 1000000 );
            bad->data[offset] ^= 1;
            CHECK_FALSE( get_mapping_capture_timing( *bad, counter, timestamp ) );
        }
        auto zero = mapping_frame( lpcl, 7, 0 );
        CHECK_FALSE( get_mapping_capture_timing( *zero, counter, timestamp ) );
    }
}

TEST_CASE( "Mapping retains validated Safety UVC timing and rejects truncated extensions",
           "[mapping-timing]" )
{
    for ( auto type : { md_type::META_DATA_INTEL_OCCUPANCY_ID, md_type::META_DATA_INTEL_POINT_CLOUD_ID } )
    {
        frame f;
        md_occupancy md{};
        md.header.md_type_id = type;
        md.header.md_size = sizeof( md );
        md.flags = 7;
        md.frame_counter = 99;
        md.frame_timestamp = 6400000000ULL;
        std::memcpy( f.additional_data.metadata_blob.data() + 12, &md, sizeof( md ) );
        f.additional_data.metadata_size = 12 + sizeof( md );
        uint32_t counter = 0;
        uint64_t timestamp = 0;
        REQUIRE( get_mapping_capture_timing( f, counter, timestamp ) );
        CHECK( counter == 99 );
        CHECK( timestamp == 6400000000ULL );
        for ( unsigned size = 0; size < 12 + sizeof( md ); ++size )
        {
            f.additional_data.metadata_size = size;
            CHECK_FALSE( get_mapping_capture_timing( f, counter, timestamp ) );
        }
        f.additional_data.metadata_size = 12 + sizeof( md );
        for ( size_t offset : { size_t( 0 ), size_t( 4 ), size_t( 12 ) } )
        {
            auto &b = f.additional_data.metadata_blob;
            b[12 + offset] ^= 4;
            CHECK_FALSE( get_mapping_capture_timing( f, counter, timestamp ) );
            b[12 + offset] ^= 4;
        }
    }
}
