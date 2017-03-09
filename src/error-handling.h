#pragma once

#include "concurrency.h"
#include "option.h"
#include "types.h"

namespace rsimpl2
{
    class polling_error_handler
    {
    public:
        polling_error_handler(unsigned int poll_intervals_ms, std::unique_ptr<option> option,
            std::shared_ptr<notifications_proccessor> proccessor, std::unique_ptr<notification_decoder> decoder);
        ~polling_error_handler();

        void start();
        void stop();

    private:
        void polling(dispatcher::cancellable_timer cancellable_timer);

        unsigned int _poll_intervals_ms;
        std::unique_ptr<option> _option;
        active_object<> _active_object;
        std::weak_ptr<notifications_proccessor> _notifications_proccessor;
        std::unique_ptr<notification_decoder> _decoder;
    };

}
