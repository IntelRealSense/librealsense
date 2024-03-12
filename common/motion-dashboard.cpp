// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "output-model.h"
#include "motion-dashboard.h"

using namespace rs2;
using namespace rsutils::string;

motion_dashboard::motion_dashboard( std::string name, enum rs2_stream stream)
    : stream_dashboard( name, 30 )
    , last_time( glfwGetTime() )
    , x_value( 0 )
    , y_value( 0 )
    , z_value( 0 )
    , n_value( 0 )
{
    dashboard_update_rate = MAX_FRAME_RATE;
    stream_type = stream;
    clear( true );
}

void motion_dashboard::process_frame( rs2::frame f )
{
    write_shared_data(
        [&]()
        {
            if( f && f.is< rs2::motion_frame >()
                && ( f.as< rs2::motion_frame >() ).get_profile().stream_type() == stream_type )
            {
                double ts = glfwGetTime();

                if( ts - last_time > dashboard_update_rate )
                {
                    rs2::motion_frame frame = f.as< rs2::motion_frame >();

                    x_value = frame.get_motion_data().x;
                    y_value = frame.get_motion_data().y;
                    z_value = frame.get_motion_data().z;
                    n_value = std::sqrt( ( x_value * x_value ) + ( y_value * y_value ) + ( z_value * z_value ) );

                    if( x_history.size() > DEQUE_SIZE )
                        x_history.pop_front();
                    if( y_history.size() > DEQUE_SIZE )
                        y_history.pop_front();
                    if( z_history.size() > DEQUE_SIZE )
                        z_history.pop_front();
                    if( n_history.size() > DEQUE_SIZE )
                        n_history.pop_front();

                    x_history.push_back( x_value );
                    y_history.push_back( y_value );
                    z_history.push_back( z_value );
                    n_history.push_back( n_value );

                    last_time = ts;
                }
            }
        } );
}

void motion_dashboard::draw( ux_window & win, rect r )
{
    r.h -= ImGui::GetTextLineHeightWithSpacing() + 10;

    if( show_x_graph )
    {
        auto x_hist = read_shared_data< std::deque< float > >( [&]() { return x_history; } );
        for( int i = 0; i < x_hist.size(); i++ )
        {
            add_point_x_axis( (float)i, x_hist[i] );
        }
    }

    if( show_y_graph )
    {
        auto y_hist = read_shared_data< std::deque< float > >( [&]() { return y_history; } );
        for( int i = 0; i < y_hist.size(); i++ )
        {
            add_point_y_axis( (float)i, y_hist[i] );
        }
    }

    if( show_z_graph )
    {
        auto z_hist = read_shared_data< std::deque< float > >( [&]() { return z_history; } );
        for( int i = 0; i < z_hist.size(); i++ )
        {
            add_point_z_axis( (float)i, z_hist[i] );
        }
    }

    if( show_n_graph )
    {
        auto n_hist = read_shared_data< std::deque< float > >( [&]() { return n_history; } );
        for( int i = 0; i < n_hist.size(); i++ )
        {
            add_point_n_vector( (float)i, n_hist[i] );
        }
    }
   
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
    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );  // -> Red
    ImGui::Checkbox( x_axis_name, &show_x_graph );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", std::string( rsutils::string::from() << "Show " << x_axis_name ).c_str() );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.00f, 1.00f, 0.00f, 1.00f ) ); // -> Green
    ImGui::Checkbox( y_axis_name, &show_y_graph );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", std::string( rsutils::string::from() << "Show " << y_axis_name ).c_str() );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.00f, 0.00f, 1.00f, 1.00f ) );  // -> Blue
    ImGui::Checkbox( z_axis_name, &show_z_graph );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", std::string( rsutils::string::from() << "Show " << z_axis_name ).c_str() );
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.00f, 1.00f, 1.00f, 1.00f ) );  // -> White
    ImGui::Checkbox( n_vector_name, &show_n_graph );
    if( ImGui::IsItemHovered() )
        ImGui::SetTooltip( "%s", "Show Normal - sqrt(x^2 + y^2 + z^2)" );
    ImGui::PopStyleColor();
}

void motion_dashboard::show_data_rate_slider()
{
    if( ( ImGui::GetContentRegionMax().x - ImGui::GetCursorPosX() - 10.f ) < 100 )
        ImGui::PushItemWidth( ImGui::GetContentRegionMax().x - ImGui::GetCursorPosX() - 10.f );
    else
        ImGui::PushItemWidth( 100 );

    ImGui::SliderFloat( "##rate", &dashboard_update_rate, MIN_FRAME_RATE, MAX_FRAME_RATE, "%.2f" );
    ImGui::GetWindowWidth();

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip( "%s", std::string( rsutils::string::from() 
                                             << "Dashboard update every " 
                                             << std::fixed 
                                             << std::setprecision(1) 
                                             << dashboard_update_rate * 1000 
                                             << " mSec"
                                            ).c_str() );
    }
}