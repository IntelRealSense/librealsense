// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "output-model.h"
#include "device-model.h"

#include <rs-config.h>
#include "ux-window.h"

namespace rs2
{
    class frame_drops_dashboard : public stream_dashboard
    {
    public:
        frame_drops_dashboard( std::string name, int * frame_drop_count, int * total )
            : stream_dashboard( name, 30 )
            , last_time( glfwGetTime() )
            , frame_drop_count( frame_drop_count )
            , total( total )
        {
            clear( true );
        }

        void process_frame( rs2::frame f ) override;
        void draw( ux_window & win, rect r ) override;
        int get_height() const override;

        void clear( bool full ) override;

    private:
        std::map< int, double > stream_to_time;
        int drops = 0;
        double last_time;
        std::deque< int > drops_history;
        int *frame_drop_count, *total;
        int counter = 0;
        int method = 0;
    };
}