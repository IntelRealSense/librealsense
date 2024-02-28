// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "motion-dashboard.h"

using namespace rs2;
using namespace rsutils::string;

motion_dashboard::motion_dashboard( std::string name )
    : stream_dashboard( name, 30 )
    , last_time( glfwGetTime() )
    , x_value( 0 )
    , y_value( 0 )
    , z_value( 0 )
    , n_value( 0 )
{
    frame_rate = MAX_FRAME_RATE;
    clear( true );
}

void motion_dashboard::draw( ux_window & win, rect r )
{
    if( accel_params[curr_accel_param_position] == "X" )
    {
        auto x_hist = read_shared_data< std::deque< float > >( [&]() { return x_history; } );
        for( int i = 0; i < x_hist.size(); i++ )
        {
            add_point( (float)i, (float)x_hist[i] );
        }
    }

    if( accel_params[curr_accel_param_position] == "Y" )
    {
        auto y_hist = read_shared_data< std::deque< float > >( [&]() { return y_history; } );
        for( int i = 0; i < y_hist.size(); i++ )
        {
            add_point( (float)i, (float)y_hist[i] );
        }
    }

    if( accel_params[curr_accel_param_position] == "Z" )
    {
        auto z_hist = read_shared_data< std::deque< float > >( [&]() { return z_history; } );
        for( int i = 0; i < z_hist.size(); i++ )
        {
            add_point( (float)i, (float)z_hist[i] );
        }
    }

    if( accel_params[curr_accel_param_position] == "N" )
    {
        auto n_hist = read_shared_data< std::deque< float > >( [&]() { return n_history; } );
        for( int i = 0; i < n_hist.size(); i++ )
        {
            add_point( (float)i, (float)n_hist[i] );
        }
    }
    r.h -= ImGui::GetTextLineHeightWithSpacing() + 10;
    draw_dashboard( win, r );

    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ImGui::GetTextLineHeightWithSpacing() * 2 );
    show_radiobuttons();

    ImGui::SameLine();

    show_data_rate_slider();
}

int motion_dashboard::get_height() const
{
    return (int)( 10 * ImGui::GetTextLineHeight() + ImGui::GetTextLineHeightWithSpacing() );
}

void motion_dashboard::clear( bool full )
{
    write_shared_data(
        [&]()
        {
            if( full )
            {
                x_history.clear();
                for( int i = 0; i < DEQUE_SIZE; i++ )
                    x_history.push_back( 0 );

                y_history.clear();
                for( int i = 0; i < DEQUE_SIZE; i++ )
                    y_history.push_back( 0 );

                z_history.clear();
                for( int i = 0; i < DEQUE_SIZE; i++ )
                    z_history.push_back( 0 );

                n_history.clear();
                for( int i = 0; i < DEQUE_SIZE; i++ )
                    n_history.push_back( 0 );
            }
        } );
}

void motion_dashboard::show_radiobuttons()
{
    ImGui::PushStyleColor( ImGuiCol_Text, from_rgba( 233, 0, 0, 255, true ) );
    ImGui::RadioButton( "X", &curr_accel_param_position, 0 );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", "Show accel X" );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, from_rgba( 0, 255, 0, 255, true ) );
    ImGui::RadioButton( "Y", &curr_accel_param_position, 1 );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", "Show accel Y" );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, from_rgba( 85, 89, 245, 255, true ) );
    ImGui::RadioButton( "Z", &curr_accel_param_position, 2 );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", "Show accel Z" );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, from_rgba( 255, 255, 255, 255, true ) );
    ImGui::RadioButton( "N", &curr_accel_param_position, 3 );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", "Show Normal" );
    ImGui::PopStyleColor();
}

void motion_dashboard::show_data_rate_slider()
{
    ImGui::PushItemWidth( 100 );
    ImGui::SliderFloat( "##rate", &frame_rate, MIN_FRAME_RATE, MAX_FRAME_RATE, "%.2f" );
    ImGui::GetWindowWidth();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip( "%s", std::string( rsutils::string::from() 
                                             << "Frame rate " 
                                             << std::fixed 
                                             << std::setprecision(1) 
                                             << frame_rate * 1000 
                                             << " mSec"
                                            ).c_str() );
    }
}