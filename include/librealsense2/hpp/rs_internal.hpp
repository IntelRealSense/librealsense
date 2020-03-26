// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_INTERNAL_HPP
#define LIBREALSENSE_RS2_INTERNAL_HPP

#include "rs_types.hpp"
#include "rs_device.hpp"
#include "rs_context.hpp"
#include "../h/rs_internal.h"

namespace rs2
{
    class recording_context : public context
    {
    public:
        /**
        * create librealsense context that will try to record all operations over librealsense into a file
        * \param[in] filename string representing the name of the file to record
        */
        recording_context(const std::string& filename,
                          const std::string& section = "",
                          rs2_recording_mode mode = RS2_RECORDING_MODE_BLANK_FRAMES)
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_recording_context(RS2_API_VERSION, filename.c_str(), section.c_str(), mode, &e),
                rs2_delete_context);
            error::handle(e);
        }

        recording_context() = delete;
    };

    class mock_context : public context
    {
    public:
        /**
        * create librealsense context that given a file will respond to calls exactly as the recording did
        * if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
        * \param[in] filename string of the name of the file
        */
        mock_context(const std::string& filename,
                     const std::string& section = "",
                     const std::string& min_api_version = "0.0.0")
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_mock_context_versioned(RS2_API_VERSION, filename.c_str(), section.c_str(), min_api_version.c_str(), &e),
                rs2_delete_context);
            error::handle(e);
        }

        mock_context() = delete;
    };

    namespace internal
    {
        /**
        * \return            the time at specific time point, in live and redord contextes it will return the system time and in playback contextes it will return the recorded time
        */
        inline double get_time()
        {
            rs2_error* e = nullptr;
            auto time = rs2_get_time( &e);

            error::handle(e);

            return time;
        }
    }

    template<class T>
    class software_device_destruction_callback : public rs2_software_device_destruction_callback
    {
        T on_destruction_function;
    public:
        explicit software_device_destruction_callback(T on_destruction) : on_destruction_function(on_destruction) {}

        void on_destruction() override
        {
            on_destruction_function();
        }

        void release() override { delete this; }
    };

    class software_sensor : public sensor
    {
    public:
        /**
        * Add video stream to software sensor
        *
        * \param[in] video_stream   all the parameters that required to defind video stream
        */
        stream_profile add_video_stream(rs2_video_stream video_stream, bool is_default=false)
        {
            rs2_error* e = nullptr;

            auto profile = rs2_software_sensor_add_video_stream_ex(_sensor.get(), video_stream, is_default, &e);
            error::handle(e);

            stream_profile stream(profile);
            return stream;
        }

        /**
        * Add motion stream to software sensor
        *
        * \param[in] motion   all the parameters that required to defind motion stream
        */
        stream_profile add_motion_stream(rs2_motion_stream motion_stream, bool is_default=false)
        {
            rs2_error* e = nullptr;

            auto profile = rs2_software_sensor_add_motion_stream_ex(_sensor.get(), motion_stream, is_default, &e);
            error::handle(e);

            stream_profile stream(profile);
            return stream;
        }

        /**
        * Add pose stream to software sensor
        *
        * \param[in] pose   all the parameters that required to defind pose stream
        */
        stream_profile add_pose_stream(rs2_pose_stream pose_stream, bool is_default=false)
        {
            rs2_error* e = nullptr;

            auto profile = rs2_software_sensor_add_pose_stream_ex(_sensor.get(), pose_stream, is_default, &e);
            error::handle(e);

            stream_profile stream(profile);
            return stream;
        }

        /**
        * Inject video frame into the sensor
        *
        * \param[in] frame   all the parameters that required to define video frame
        */
        void on_video_frame(rs2_software_video_frame frame)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_on_video_frame(_sensor.get(), frame, &e);
            error::handle(e);
        }

        /**
        * Inject motion frame into the sensor
        *
        * \param[in] frame   all the parameters that required to define motion frame
        */
        void on_motion_frame(rs2_software_motion_frame frame)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_on_motion_frame(_sensor.get(), frame, &e);
            error::handle(e);
        }

        /**
        * Inject pose frame into the sensor
        *
        * \param[in] frame   all the parameters that required to define pose frame
        */
        void on_pose_frame(rs2_software_pose_frame frame)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_on_pose_frame(_sensor.get(), frame, &e);
            error::handle(e);
        }

        /**
        * Set frame metadata for the upcoming frames
        * \param[in] value metadata key to set
        * \param[in] type metadata value
        */
        void set_metadata(rs2_frame_metadata_value value, rs2_metadata_type type)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_set_metadata(_sensor.get(), value, type, &e);
            error::handle(e);
        }

        /**
        * Register read-only option that will be supported by the sensor
        *
        * \param[in] option  the option
        * \param[in] val  the initial value
        */
        void add_read_only_option(rs2_option option, float val)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_add_read_only_option(_sensor.get(), option, val, &e);
            error::handle(e);
        }

        /**
        * Update value of registered read-only option
        *
        * \param[in] option  the option
        * \param[in] val  the initial value
        */
        void set_read_only_option(rs2_option option, float val)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_update_read_only_option(_sensor.get(), option, val, &e);
            error::handle(e);
        }
        /**
        * Register option that will be supported by the sensor
        *
        * \param[in] option  the option
        * \param[in] range  range data for the option. range.def will be used as the initial value
        */
        void add_option(rs2_option option, const option_range& range, bool is_writable=true)
        {
            rs2_error* e = nullptr;
            rs2_software_sensor_add_option(_sensor.get(), option, range.min,
                range.max, range.step, range.def, is_writable, &e);
            error::handle(e);
        }

        void on_notification(rs2_software_notification notif)
        {
            rs2_error * e = nullptr;
            rs2_software_sensor_on_notification(_sensor.get(), notif, &e);
            error::handle(e);
        }
        /**
        * Sensors hold the parent device in scope via a shared_ptr. This function detaches that so that the 
        * software sensor doesn't keep the software device alive. Note that this is dangerous as it opens the
        * door to accessing freed memory if care isn't taken.
        */
        void detach()
        {
            rs2_error * e = nullptr;
            rs2_software_sensor_detach(_sensor.get(), &e);
            error::handle(e);
        }

    private:
        friend class software_device;

        software_sensor(std::shared_ptr<rs2_sensor> s)
            : rs2::sensor(s)
        {
            rs2_error* e = nullptr;
            if (rs2_is_sensor_extendable_to(_sensor.get(), RS2_EXTENSION_SOFTWARE_SENSOR, &e) == 0 && !e)
            {
                _sensor = nullptr;
            }
            rs2::error::handle(e);
        }
    };


    class software_device : public device
    {
        std::shared_ptr<rs2_device> create_device_ptr(std::function<void(rs2_device*)> deleter)
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_software_device(&e),
                deleter);
            error::handle(e);
            return dev;
        }

    public:
        software_device(std::function<void(rs2_device*)> deleter = &rs2_delete_device)
            : device(create_device_ptr(deleter))
        {
            this->set_destruction_callback([]{});
        }

        software_device(std::string name)
            : device(create_device_ptr(&rs2_delete_device))
        {
            update_info(RS2_CAMERA_INFO_NAME, name);
        }

        /**
        * Add software sensor with given name to the software device.
        *
        * \param[in] name   the name of the sensor
        */
        software_sensor add_sensor(std::string name)
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_sensor> sensor(
                rs2_software_device_add_sensor(_dev.get(), name.c_str(), &e),
                rs2_delete_sensor);
            error::handle(e);

            return software_sensor(sensor);
        }

        /**
        * Register destruction callback
        * \param[in] callback   destruction callback
        */
        template<class T>
        void set_destruction_callback(T callback) const
        {
            rs2_error* e = nullptr;
            rs2_software_device_set_destruction_callback_cpp(_dev.get(),
                new software_device_destruction_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * Add software device to existing context.
        * Any future queries on the context will return this device.
        * This operation cannot be undone (except for destroying the context)
        *
        * \param[in] ctx   context to add the device to
        */
        void add_to(context& ctx)
        {
            rs2_error* e = nullptr;
            rs2_context_add_software_device(ctx._context.get(), _dev.get(), &e);
            error::handle(e);
        }

        /**
        * Add a new camera info value, like serial number
        *
        * \param[in] info  Identifier of the camera info type
        * \param[in] val   string value to set to this camera info type
        */
        void register_info(rs2_camera_info info, const std::string& val)
        {
            rs2_error* e = nullptr;
            rs2_software_device_register_info(_dev.get(), info, val.c_str(), &e);
            error::handle(e);
        }

        /**
        * Update an existing camera info value, like serial number
        *
        * \param[in] info  Identifier of the camera info type
        * \param[in] val   string value to set to this camera info type
        */
        void update_info(rs2_camera_info info, const std::string& val)
        {
            rs2_error* e = nullptr;
            rs2_software_device_update_info(_dev.get(), info, val.c_str(), &e);
            error::handle(e);
        }

        /**
        * Set the wanted matcher type that will be used by the syncer
        * \param[in] matcher matcher type
        */
        void create_matcher(rs2_matchers matcher)
        {
            rs2_error* e = nullptr;
            rs2_software_device_create_matcher(_dev.get(), matcher, &e);
            error::handle(e);
        }
    };

}
#endif // LIBREALSENSE_RS2_INTERNAL_HPP
