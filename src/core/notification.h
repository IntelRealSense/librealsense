// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/h/rs_types.h>
#include <rsutils/concurrency/concurrency.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <memory>
#include <string>


namespace librealsense {


using notifications_callback_ptr = std::shared_ptr< rs2_notifications_callback >;


struct notification
{
    notification( rs2_notification_category category, int type, rs2_log_severity severity, std::string const & description )
        : category( category )
        , type( type )
        , severity( severity )
        , description( description )
    {
        timestamp = std::chrono::duration< double, std::milli >( std::chrono::system_clock::now().time_since_epoch() )
                        .count();
        LOG_INFO( description );
    }

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

    void set_callback( notifications_callback_ptr callback );
    notifications_callback_ptr get_callback() const;
    void raise_notification( const notification );

private:
    notifications_callback_ptr _callback;
    std::mutex _callback_mutex;
    dispatcher _dispatcher;
};


}  // namespace librealsense