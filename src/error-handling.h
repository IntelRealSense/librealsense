/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include "concurrency.h"
#include "option.h"
#include "types.h"

namespace librealsense
{
    class polling_error_handler
    {
    public:
        polling_error_handler(unsigned int poll_intervals_ms, std::shared_ptr<option> option,
            std::shared_ptr<notifications_processor> processor, std::shared_ptr<notification_decoder> decoder);
        ~polling_error_handler();

        polling_error_handler(const polling_error_handler& h);

        unsigned int get_polling_interval() const { return _poll_intervals_ms; }

        void start( unsigned int poll_intervals_ms = 0 );
        void stop();

    private:
        void polling(dispatcher::cancellable_timer cancellable_timer);

        unsigned int _poll_intervals_ms;
        bool _silenced = false;
        std::shared_ptr<option> _option;
        std::shared_ptr < active_object<> > _active_object;
        std::weak_ptr<notifications_processor> _notifications_processor;
        std::shared_ptr<notification_decoder> _decoder;
    };

}
