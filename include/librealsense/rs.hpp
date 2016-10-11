// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rs.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace rs
{
    class error : public std::runtime_error
    {
        std::string function, args;
    public:
        explicit error(rs_error * err) : std::runtime_error(rs_get_error_message(err))
        {
            function = (nullptr != rs_get_failed_function(err)) ? rs_get_failed_function(err) : std::string();
            args = (nullptr != rs_get_failed_args(err)) ? rs_get_failed_args(err) : std::string();
            rs_free_error(err);
        }
        const std::string & get_failed_function() const { return function; }
        const std::string & get_failed_args() const { return args; }
        static void handle(rs_error * e) { if (e) throw error(e); }
    };

    class context;

    class device_info
    {
    public:

    private:
        friend context;
        explicit device_info(std::shared_ptr<rs_device_info> info) : _info(info) {}
        
        std::shared_ptr<rs_device_info> _info;
    };

    /*
     * context object can be used for enumerating over currently connected devices
     * and to create device object
     */
    class context
    {
    public:
        context()
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_context(RS_API_VERSION, &e),
                rs_delete_context);
            error::handle(e);
        }

        std::vector<device_info> query_devices() const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device_info_list> list(
                rs_query_devices(_context.get(), &e),
                rs_delete_device_info_list);
            error::handle(e);

            auto size = rs_get_device_list_size(list.get(), &e);
            error::handle(e);

            std::vector<device_info> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs_device_info> info(
                    rs_get_device_info(list.get(), i, &e),
                    rs_delete_device_info);
                error::handle(e);

                device_info rs_info(info);
                results.push_back(rs_info);
            }

            return results;
        }

    private:
        std::shared_ptr<rs_context> _context;
    };
};

#endif // LIBREALSENSE_RS2_HPP
