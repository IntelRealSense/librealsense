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

        rs2_digital_gain _digital_gain;
        int _receiver_gain;
        double _temp;

        std::weak_ptr< hw_monitor > _hwm;
        l500_device & _dev;

        bool _is_on = false;
        std::mutex _mutex;
        std::atomic_bool _is_processing {false};  // Whether algo is currently running
        std::thread _worker;
        unsigned _n_retries;     // how many special frame requests we've made
        unsigned _n_cycles = 0;  // how many times we've run algo

        rs2_extrinsics _extr;
        rs2_intrinsics _raw_intr;
        rs2_intrinsics _thermal_intr;
        rs2_dsm_params _dsm_params;
        stream_profile_interface* _from_profile = nullptr;
        stream_profile_interface* _to_profile = nullptr;
        std::vector< uint16_t > _last_yuy_data;

        class retrier;
        std::shared_ptr< retrier > _retrier;
        std::shared_ptr< retrier > _recycler;
        rs2_calibration_status _last_status_sent;

        class next_trigger;
        std::shared_ptr< next_trigger > _next_trigger;

        class temp_check;
        double _last_temp = 0;
        std::shared_ptr< temp_check > _temp_check;

        bool _need_to_wait_for_color_sensor_stability = false;
        std::chrono::high_resolution_clock::time_point _rgb_sensor_start;

    public:
        /* Depth frame processing: detect special frames
        */
        class depth_processing_block : public generic_processing_block
        {
            std::weak_ptr< ac_trigger > _autocal;

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
            std::weak_ptr< ac_trigger > _autocal;

        public:
            color_processing_block( std::shared_ptr< ac_trigger > autocal );

            rs2::frame process_frame( const rs2::frame_source& source, const rs2::frame& f ) override;

        private:
            bool should_process( const rs2::frame& frame ) override;
        };

        /* For RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH */
        class enabler_option : public float_option
        {
            typedef float_option super;

            std::weak_ptr< ac_trigger > _autocal;

        public:
            enabler_option( std::shared_ptr< ac_trigger > const & autocal );

            bool is_auto() const { return (_value == _opt_range.max); }
            bool is_manual() const { return ! is_auto(); }

            virtual void set( float value ) override;
            virtual const char* get_description() const override
            {
                return "Trigger Camera Accuracy Health (off, run now, auto)";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
        };

        /* For RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH */
        class reset_option : public bool_option
        {
            std::weak_ptr< ac_trigger > _autocal;

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

        enum class calibration_type
        {
            MANUAL,
            AUTO
        };

    private:
        calibration_type _calibration_type;

    public:
        ac_trigger( l500_device & dev, std::shared_ptr<hw_monitor> hwm );
        ~ac_trigger();

        // Called when depth sensor start. Triggers a calibration in a few seconds if auto
        // calibration is turned on.
        void start();

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
        void trigger_calibration( calibration_type type );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_raw_intrinsics() const { return _raw_intr; }
        rs2_intrinsics const & get_thermal_intrinsics() const { return _thermal_intr; }
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
        void schedule_next_calibration();
        void schedule_next_time_trigger( std::chrono::seconds n_seconds = std::chrono::seconds( 0 ) );
        void schedule_next_temp_trigger();
        void cancel_current_calibration();
        void set_not_active();
        void trigger_retry();
        void trigger_special_frame();
        void check_conditions();
        void _start();


        std::vector< callback > _callbacks;

        void call_back( rs2_calibration_status status );

        bool check_color_depth_sync();
        void run_algo();
        void reset();

        class ac_logger;
        ac_logger & get_ac_logger();

    };


}  // namespace ivcam2
}  // namespace librealsense

