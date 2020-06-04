// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "option.h"
#include <mutex>
#include <thread>


namespace librealsense {

    class l500_device;

namespace ivcam2 {


    /*
        This is the triggering code for depth-to-RGB-calibration
    */
    class ac_trigger : public std::enable_shared_from_this< ac_trigger >
    {
        rs2::frameset _sf;
        rs2::frame _cf, _pcf;  // Keep the last and previous frame!

        float _dsm_x_scale;  // registers read when we get a special frame
        float _dsm_y_scale;
        float _dsm_x_offset;
        float _dsm_y_offset;

        hw_monitor & _hwm;
        l500_device & _dev;

        std::mutex _mutex;
        std::atomic_bool _is_processing;
        std::thread _worker;
        unsigned _n_retries;    // how many tries for a special frame we made
        unsigned _n_cycles;     // how many times AC tried to run

        rs2_extrinsics _extr;
        rs2_intrinsics _intr;
        rs2_dsm_params _dsm_params;
        stream_profile_interface* _from_profile;
        stream_profile_interface* _to_profile;

        class retrier;
        std::shared_ptr< retrier > _retrier;
        std::shared_ptr< retrier > _recycler;
        std::shared_ptr< retrier > _next_trigger;
        rs2_calibration_status _last_status_sent;

    public:
        /* Depth frame processing: detect special frames
        */
        class depth_processing_block : public generic_processing_block
        {
            std::shared_ptr< ac_trigger > _autocal;

        public:
            depth_processing_block( std::shared_ptr< ac_trigger > autocal );

            rs2::frame process_frame( const rs2::frame_source& source, const rs2::frame& f ) override;

        private:
            bool should_process( const rs2::frame& frame ) override;
            rs2::frame prepare_output( const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results ) override;
        };

        /* Color frame processing: pass color frames to the trigger
        */
        class color_processing_block : public generic_processing_block
        {
            std::shared_ptr< ac_trigger > _autocal;

        public:
            color_processing_block( std::shared_ptr< ac_trigger > autocal );

            rs2::frame process_frame( const rs2::frame_source& source, const rs2::frame& f ) override;

        private:
            bool should_process( const rs2::frame& frame ) override;
        };

        /* For RS2_OPTION_CAMERA_ACCURACY_HEALTH_ENABLED */
        class enabler_option : public bool_option
        {
            std::shared_ptr< ac_trigger > _autocal;

        public:
            enabler_option( std::shared_ptr< ac_trigger > const & autocal );

            virtual void set( float value ) override;
            virtual const char* get_description() const override
            {
                return "Enable to ask FW for a special frame for auto calibration";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
        };

    public:
        ac_trigger( l500_device & dev, hw_monitor & hwm );
        ~ac_trigger();

        // Wait a certain amount of time before the next AC happens. Can only happen if not already
        // active!
        void trigger_next_ac( unsigned n_seconds );

        // Once triggered, we may want to cancel it... like when stopping the stream
        void cancel_next_ac();

        // If we're active, no new triggers will be accepted until we reach a final state
        // (successful or failed)
        bool is_active() const { return _n_cycles > 0; }

        // Start AC activity -- after this, is_active() returns true
        void trigger_special_frame( bool is_retry = false );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_intrinsics() const { return _intr; }
        rs2_dsm_params const & get_dsm_params() const { return _dsm_params; }
        stream_profile_interface * get_from_profile() const { return _from_profile; }
        stream_profile_interface * get_to_profile() const { return _to_profile; }

        void set_special_frame( rs2::frameset const& );
        void set_color_frame( rs2::frame const& );

        using callback = std::function< void( rs2_calibration_status ) >;
        void register_callback( callback cb )
        {
            _callbacks.push_back( cb );
        }

    private:
        bool is_processing() const { return _is_processing; }
        bool is_expecting_special_frame() const { return !!_retrier; }

        std::vector< callback > _callbacks;

        void call_back( rs2_calibration_status status )
        {
            _last_status_sent = status;
            for( auto && cb : _callbacks )
                cb( status );
        }

        bool check_color_depth_sync();
        void start();
        void reset();
    };


}  // namespace ivcam2
}  // namespace librealsense

