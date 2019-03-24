// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "error-handling.h"

#include <memory>


namespace librealsense
{
    polling_error_handler::polling_error_handler(unsigned int poll_intervals_ms, std::unique_ptr<option> option,
        std::shared_ptr <notifications_processor> processor, std::unique_ptr<notification_decoder> decoder)
        :_poll_intervals_ms(poll_intervals_ms),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
        {
            polling(cancellable_timer);
        }),
        _option(std::move(option)),
        _notifications_processor(processor),
        _decoder(std::move(decoder))
    {
    }

    polling_error_handler::~polling_error_handler()
    {
        stop();
    }

    void polling_error_handler::start()
    {
        _active_object.start();
    }
    void polling_error_handler::stop()
    {
        _active_object.stop();
    }

    void polling_error_handler::polling(dispatcher::cancellable_timer cancellable_timer)
    {
         if (cancellable_timer.try_sleep(_poll_intervals_ms))
         {
             try
             {
                 auto val = static_cast<uint8_t>(_option->query());

                 if (val != 0 && !_silenced)
                 {
                     auto strong = _notifications_processor.lock();
                     if (strong) strong->raise_notification(_decoder->decode(val));

                     val = static_cast<int>(_option->query());
                     if (val != 0)
                     {
                         // Reading from last-error control is supposed to set it to zero in the firmware
                         // If this is not happening there is some issue
                         notification postcondition_failed{
                             RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR,
                             0,
                             RS2_LOG_SEVERITY_WARN,
                             "Error polling loop is not behaving as expected!\nThis can indicate an issue with camera firmware or the underlying OS..."
                         };
                         if (strong) strong->raise_notification(postcondition_failed);
                         _silenced = true;
                     }
                 }
             }
             catch (const std::exception& ex)
             {
                 LOG_ERROR("Error during polling error handler: " << ex.what());
             }
             catch (...)
             {
                 LOG_ERROR("Unknown error during polling error handler!");
             }
         }
         else
         {
             LOG_DEBUG("Notification polling loop is being shut-down");
         }
    }
}
