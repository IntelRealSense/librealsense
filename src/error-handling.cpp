#include "error-handling.h"

#include <memory>


namespace rsimpl2
{
    polling_error_handler::polling_error_handler(unsigned int poll_intervals_ms, std::unique_ptr<option> option,
        std::shared_ptr <notifications_proccessor> proccessor, std::unique_ptr<notification_decoder> decoder)
        :_poll_intervals_ms(poll_intervals_ms),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
        {
            polling(cancellable_timer);
        }), 
        _option(std::move(option)),
        _notifications_proccessor(proccessor),
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
             auto val = 0;
             try
             {
                 val = static_cast<int>(_option->query());

                 if (val != 0)
                 {
                     auto notification = _decoder->decode(val);
                     auto strong = _notifications_proccessor.lock();
                     if (strong)strong->raise_notification(notification);
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
