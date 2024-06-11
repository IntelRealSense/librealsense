// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/hpp/rs_types.hpp>
#include <rsutils/concurrency/concurrency.h>
#include <memory>
#include <string>


namespace librealsense {


struct notification
{
    notification( rs2_notification_category category,
                  int type,
                  rs2_log_severity severity,
                  std::string const & description );

    rs2_notification_category category;
    int type;
    rs2_log_severity severity;
    std::string description;
    double timestamp;
    std::string serialized_data;
};


class notification_decoder
{
public:
    virtual ~notification_decoder() = default;
    virtual notification decode( int value ) = 0;
};


class notifications_processor
{
public:
    notifications_processor();
    ~notifications_processor();

    void set_callback( rs2_notifications_callback_sptr callback );
    rs2_notifications_callback_sptr get_callback() const;
    void raise_notification( const notification );

private:
    rs2_notifications_callback_sptr _callback;
    std::mutex _callback_mutex;
    dispatcher _dispatcher;
};


}  // namespace librealsense