// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "processing-blocks-factory.h"

#include <vector>
#include <unordered_set>
#include <memory>

namespace librealsense
{
    class formats_converter
    {
    public:
        void register_processing_block( const std::vector< stream_profile > & from,
                                        const std::vector< stream_profile > & to,
                                        std::function< std::shared_ptr< processing_block >( void ) > generate_func );
        void register_processing_block( const processing_block_factory & pbf );
        void register_processing_block( const std::vector< processing_block_factory > & pbfs );

        stream_profiles get_all_possible_target_profiles( const stream_profiles & from_profiles );
        void prepare_to_convert( stream_profiles target_profiles );

        stream_profiles get_active_source_profiles() const;
        std::vector< std::shared_ptr< processing_block > > get_active_converters() const;

        void set_frames_callback( frame_callback_ptr callback );
        frame_callback_ptr get_frames_callback() const { return _converted_frames_callback; }
        void convert_frame( frame_holder & f );

    protected:
        void clear_active_cache();
        void update_source_data( const stream_profiles & target_profiles );
        void cache_target_profiles( const stream_profiles & target_profiles );

        std::shared_ptr< stream_profile_interface > clone_profile( const std::shared_ptr< stream_profile_interface > & profile ) const;
        bool is_profile_in_list( const std::shared_ptr< stream_profile_interface > & profile, const stream_profiles & profiles ) const;

        std::pair< std::shared_ptr< processing_block_factory >, stream_profiles >
            find_pbf_matching_most_profiles( const stream_profiles & profiles );

        std::shared_ptr<stream_profile_interface> filter_frame_by_requests( const frame_interface * f );

        std::vector< std::shared_ptr< processing_block_factory > > _pb_factories;
        std::unordered_map< processing_block_factory *, stream_profiles > _pbf_supported_profiles;
        std::unordered_map< stream_profile, stream_profiles > _target_to_source_profiles_map;

        std::unordered_map< std::shared_ptr< stream_profile_interface >,
                            std::unordered_set< std::shared_ptr< processing_block > > > _source_profile_to_converters;
        std::unordered_map< rs2_format, stream_profiles > _format_to_target_profiles;

        frame_callback_ptr _converted_frames_callback = nullptr;
    };
}
