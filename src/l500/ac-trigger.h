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

        bool _is_on = false;
        std::mutex _mutex;
        bool _is_processing = false;  // Whether algo is currently running
        std::thread _worker;
        unsigned _n_retries;     // how many special frame requests we've made
        unsigned _n_cycles = 0;  // how many times we've run algo

        rs2_extrinsics _extr;
        rs2_intrinsics _intr;
        rs2_dsm_params _dsm_params;
        stream_profile_interface* _from_profile = nullptr;
        stream_profile_interface* _to_profile = nullptr;

        class retrier;
        std::shared_ptr< retrier > _retrier;
        std::shared_ptr< retrier > _recycler;
        std::shared_ptr< retrier > _next_trigger;
        rs2_calibration_status _last_status_sent;
        std::atomic_bool _own_color_stream{ false };

        class temp_check;
        double _last_temp = 0;
        std::shared_ptr< temp_check > _temp_check;

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

        /* For RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH */
        class enabler_option : public bool_option
        {
            std::shared_ptr< ac_trigger > _autocal;

        public:
            enabler_option( std::shared_ptr< ac_trigger > const & autocal );

            virtual void set( float value ) override;
            virtual const char* get_description() const override
            {
                return "Trigger Camera Accuracy Health";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
        };

        /* For RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH */
        class reset_option : public bool_option
        {
            std::shared_ptr< ac_trigger > _autocal;

        public:
            reset_option( std::shared_ptr< ac_trigger > const & autocal );

            virtual void set( float value ) override;
            virtual const char* get_description() const override
            {
                return "Reset the FW table for Camera Accuracy Health";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
        };

    public:
        ac_trigger( l500_device & dev, hw_monitor & hwm );
        ~ac_trigger();

        // Wait a certain amount of time before the next calibration happens. Can only happen if not
        // already active!
        void start( std::chrono::seconds n_seconds = std::chrono::seconds(0) );

        // Once triggered, we may want to cancel it... like when stopping the stream
        void stop();

        // If we're active, calibration is currently in progress (anywhere between asking for a
        // special frame and finishing with success/failure). No new triggers will be accepted!
        bool is_active() const { return _n_cycles > 0; }

        // Returns whether the mechanism is on: anywhere between start() and stop(). When on,
        // the end of one calibration triggers a temperature check and the next calibration.
        //
        // Note that is_active() can be true even when we're off -- trigger_calibration()
        // can manually trigger a calibration, meaning that the calibration will run its
        // course and then stop...
        //
        bool auto_calibration_is_on() const { return _is_on; }

        // Start calibration -- after this, is_active() returns true. See the note for is_on().
        void trigger_calibration( bool is_retry = false );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_intrinsics() const { return _intr; }
        rs2_dsm_params const & get_dsm_params() const { return _dsm_params; }
        stream_profile_interface * get_from_profile() const { return _from_profile; }
        stream_profile_interface * get_to_profile() const { return _to_profile; }

        using callback = std::function< void( rs2_calibration_status ) >;
        void register_callback( callback cb )
        {
            _callbacks.push_back( cb );
        }

    private:
        void set_special_frame( rs2::frameset const & );
        void set_color_frame( rs2::frame const & );

        void start_color_sensor_if_needed();
        void stop_color_sensor_if_started();

        bool is_processing() const { return _is_processing; }
        bool is_expecting_special_frame() const { return !!_retrier; }
        double read_temperature();
        void calibration_is_done();

        std::vector< callback > _callbacks;

        void call_back( rs2_calibration_status status )
        {
            _last_status_sent = status;
            for( auto && cb : _callbacks )
                cb( status );
        }

        bool check_color_depth_sync();
        void run_algo();
        void reset();
    };


}  // namespace ivcam2
}  // namespace librealsense

