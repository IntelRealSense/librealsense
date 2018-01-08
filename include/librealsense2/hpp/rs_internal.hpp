// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_INTERNAL_HPP
#define LIBREALSENSE_RS2_INTERNAL_HPP

#include "rs_types.hpp"
#include "rs_device.hpp"
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
                     const std::string& section = "")
        {
            rs2_error* e = nullptr;
            _context = std::shared_ptr<rs2_context>(
                rs2_create_mock_context(RS2_API_VERSION, filename.c_str(), section.c_str(), &e),
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
    class bypass_sensor : public sensor
    {
    public:
        bypass_sensor(std::shared_ptr<rs2_sensor> s)
            : rs2::sensor(s)
        {
            rs2_error* e = nullptr;
            if (rs2_is_sensor_extendable_to(_sensor.get(), RS2_EXTENSION_BYPASS_SENSOR, &e) == 0 && !e)
            {
                _sensor = nullptr;
            }
            rs2::error::handle(e);
        }

        void add_video_stream(rs2_stream type, int index,
            int uid, int width, int height, int bpp, rs2_format fmt, rs2_intrinsics intrinsics)
        {
            rs2_error* e = nullptr;
            rs2_bypass_add_video_stream(_sensor.get(), 
                type, index, uid, width, height, bpp, fmt, intrinsics, &e);
            error::handle(e);
        }
    };

    class bypass_device : public device
    {
        std::shared_ptr<rs2_device> create_device_ptr()
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_bypass_device(&e),
                rs2_delete_device);
            error::handle(e);
            return dev;
        }

    public:
        bypass_device()
            : device(create_device_ptr())
        {

        }

        bypass_sensor add_sensor(std::string name)
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_sensor> sensor(
                rs2_bypass_add_sensor(_dev.get(), name.c_str(), &e),
                rs2_delete_sensor);
            error::handle(e);

            return bypass_sensor(sensor);
            
        }

        void on_video_frame(int sensor, 
            void* pixels, void(*deleter)(void*),
            int stride, int bpp,
            rs2_time_t timestamp, rs2_timestamp_domain domain,
            int frame_number, stream_profile profile)
        {
            rs2_error* e = nullptr;
            rs2_bypass_on_video_frame(_dev.get(), sensor, 
                pixels, deleter, stride, bpp, timestamp, domain, frame_number, profile.get(), &e);
            error::handle(e);
        }
    };

}
#endif // LIBREALSENSE_RS2_INTERNAL_HPP
