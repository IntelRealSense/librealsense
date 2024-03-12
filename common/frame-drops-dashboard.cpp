// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "frame-drops-dashboard.h"

using namespace rs2;
using namespace rsutils::string;

void frame_drops_dashboard::process_frame( rs2::frame f )
{
    write_shared_data(
        [&]()
        {
            double ts = glfwGetTime();
            if( method == 1 )
                ts = f.get_timestamp() / 1000.f;
            auto it = stream_to_time.find( f.get_profile().unique_id() );
            if( it != stream_to_time.end() )
            {
                auto last = stream_to_time[f.get_profile().unique_id()];

                double fps = (double)f.get_profile().fps();

                if( f.supports_frame_metadata( RS2_FRAME_METADATA_ACTUAL_FPS ) )
                    fps = f.get_frame_metadata( RS2_FRAME_METADATA_ACTUAL_FPS ) / 1000.;

                if( 1000. * ( ts - last ) > 1.5 * ( 1000. / fps ) )
                {
                    drops++;
                }
            }

            counter++;

            if( ts - last_time > 1.f )
            {
                if( drops_history.size() > 100 )
                    drops_history.pop_front();
                drops_history.push_back( drops );
                *total = counter;
                *frame_drop_count = drops;
                drops = 0;
                last_time = ts;
                counter = 0;
            }

            stream_to_time[f.get_profile().unique_id()] = ts;
        } );
}

void frame_drops_dashboard::draw( ux_window & win, rect r )
{
    auto hist = read_shared_data< std::deque< int > >( [&]() { return drops_history; } );
    for( int i = 0; i < hist.size(); i++ )
    {
        add_point_n_vector( (float)i, (float)hist[i] );
    }
    r.h -= ImGui::GetTextLineHeightWithSpacing() + 10;

    draw_dashboard( win, r );

    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetTextLineHeightWithSpacing() * 2 );
    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 3 );
    ImGui::Text( "%s", "Measurement Metric:" );
    ImGui::SameLine();
    ImGui::SetCursorPosY( ImGui::GetCursorPosY() - 3 );

    ImGui::SetCursorPosX( 11.5f * win.get_font_size() );

    std::vector< const char * > methods;
    methods.push_back( "Viewer Processing Rate" );
    methods.push_back( "Camera Timestamp Rate" );

    ImGui::PushItemWidth( -1.f );
    if( ImGui::Combo( "##fps_method", &method, methods.data(), (int)( methods.size() ) ) )
    {
        clear( false );
    }
    ImGui::PopItemWidth();
}

int frame_drops_dashboard::get_height() const
{
    return (int)( 10 * ImGui::GetTextLineHeight() + ImGui::GetTextLineHeightWithSpacing() );
}

void frame_drops_dashboard::clear( bool full )
{
    write_shared_data(
        [&]()
        {
            stream_to_time.clear();
            last_time = 0;
            *total = 0;
            *frame_drop_count = 0;
            if( full )
            {
                drops_history.clear();
                for( int i = 0; i < 100; i++ )
                    drops_history.push_back( 0 );
            }
        } );
}
