// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once


#include <librealsense2/hpp/rs_types.hpp>
#include <src/frame-archive.h>


namespace librealsense
{
    class option;
    class frame_holder;
    class archive_interface;

    class LRS_EXTENSION_API frame_source
    {
    public:
        //using stream_uid = std::pair< rs2_stream, int >; // Stream type and index
        using archive_id = std::pair< rs2_stream, rs2_extension >;

        frame_source( uint32_t max_publish_list_size = 16 );

        void init( std::shared_ptr< metadata_parser_map > metadata_parsers );

        callback_invocation_holder begin_callback( archive_id id );

        void reset();

        std::shared_ptr< option > get_published_size_option();

        frame_interface * alloc_frame( archive_id id,
                                       size_t size,
                                       frame_additional_data && additional_data,
                                       bool requires_memory );

        void set_callback( rs2_frame_callback_sptr callback );
        rs2_frame_callback_sptr get_callback() const;

        void invoke_callback( frame_holder frame ) const;

        void flush() const;

        virtual ~frame_source() { flush(); }

        void set_sensor( const std::weak_ptr< sensor_interface > & s );

        template<class T>
        void add_extension( rs2_extension ex )
        {
            archive_id special_index = { RS2_STREAM_COUNT, ex };
            
            std::lock_guard< std::recursive_mutex > lock( _mutex );

            _archive[special_index] = std::make_shared< frame_archive< T > >( &_max_publish_list_size, _metadata_parsers );
        }

        void set_max_publish_list_size( int qsize ) { _max_publish_list_size = qsize; }

        static rs2_extension stream_to_frame_types( rs2_stream stream );

    private:
        friend class syncer_process_unit;

        void create_archive( archive_id id );

        mutable std::recursive_mutex _mutex;

        std::map< archive_id, std::shared_ptr< archive_interface > > _archive;
        std::vector< rs2_extension > _supported_extensions;

        std::atomic< uint32_t > _max_publish_list_size;
        rs2_frame_callback_sptr _callback;
        std::shared_ptr< metadata_parser_map > _metadata_parsers;
        std::weak_ptr< sensor_interface > _sensor;
    };
}
