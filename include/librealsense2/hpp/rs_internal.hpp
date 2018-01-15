// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_INTERNAL_HPP
#define LIBREALSENSE_RS2_INTERNAL_HPP

#include "rs_types.hpp"
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

}
#endif // LIBREALSENSE_RS2_INTERNAL_HPP
