// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "option-model.h"
#include <librealsense2/rs_advanced_mode.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include "device-model.h"
#include "subdevice-model.h"

namespace rs2
{
    option_model create_option_model(rs2_option opt,
        const std::string& opt_base_label,
        subdevice_model* model,
        std::shared_ptr<options> options,
        bool* options_invalidated,
        std::string& error_message)
    {
        option_model option = {};

        std::stringstream ss;

        ss << opt_base_label << "/" << options->get_option_name(opt);
        option.id = ss.str();
        option.opt = opt;
        option.endpoint = options;
        option.label = options->get_option_name(opt) + std::string("##") + ss.str();
        option.invalidate_flag = options_invalidated;
        option.dev = model;
        option.range = { 0, 1, 0, 0 };
        option.value = 0;

        option.supported = options->supports(opt);
        if (option.supported)
        {
            try
            {
                option.range = options->get_option_range(opt);
                option.read_only = options->is_option_read_only(opt);
                option.value = options->get_option(opt);
            }
            catch (const error& e)
            {
                error_message = error_to_string(e);
            }
        }
        return option;
    }
}

using namespace rs2;

std::string option_model::adjust_description(const std::string& str_in, const std::string& to_be_replaced, const std::string& to_replace)
{
    std::string adjusted_string(str_in);
    auto pos = adjusted_string.find(to_be_replaced);
    adjusted_string.replace(pos, to_be_replaced.size(), to_replace);
    return adjusted_string;
}

bool option_model::draw( std::string & error_message,
                         notifications_model & model,
                         bool new_line,
                         bool use_option_name )
{
    auto res = false;
    if( endpoint->supports( opt ) )
    {
        // The option's rendering model supports an alternative option title derived from its
        // description rather than name. This is applied to the Holes Filling as its display must
        // conform with the names used by a 3rd-party tools for consistency.
        if( opt == RS2_OPTION_HOLES_FILL )
            use_option_name = false;

        std::string desc_str( endpoint->get_option_description( opt ) );

        // Device D405 is for short range, therefore, its units are in cm - for better UX
        bool use_cm_units = false;
        std::string device_pid = dev->dev.get_info( RS2_CAMERA_INFO_PRODUCT_ID );
        if( device_pid == "0B5B"
            && val_in_range(
                opt,
                { RS2_OPTION_MIN_DISTANCE, RS2_OPTION_MAX_DISTANCE, RS2_OPTION_DEPTH_UNITS } ) )
        {
            use_cm_units = true;
            desc_str = adjust_description( desc_str, "meters", "cm" );
        }

        auto desc = desc_str.c_str();

        // remain option to append to the current line
        if( ! new_line )
            ImGui::SameLine();

        if( is_enum() )
        {
            res = draw_combobox( model, error_message, desc, new_line, use_option_name );
        }
        else
        {
            if( is_checkbox() )
            {
                res = draw_checkbox( model, error_message, desc );
            }
            else
            {
                res = draw_slider( model, error_message, desc, use_cm_units );
            }
        }

        if( ! read_only && opt == RS2_OPTION_ENABLE_AUTO_EXPOSURE && dev->auto_exposure_enabled
            && dev->s->is< roi_sensor >() && dev->streaming )
        {
            ImGui::SameLine( 0, 10 );
            std::string button_label = label;
            auto index = label.find_last_of( '#' );
            if( index != std::string::npos )
            {
                button_label = label.substr( index + 1 );
            }

            ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, { 1.f, 1.f, 1.f, 1.f } );
            if( ! dev->roi_checked )
            {
                std::string caption = rsutils::string::from() << "Set ROI##" << button_label;
                if( ImGui::Button( caption.c_str(), { 55, 0 } ) )
                {
                    dev->roi_checked = true;
                }
            }
            else
            {
                std::string caption = rsutils::string::from() << "Cancel##" << button_label;
                if( ImGui::Button( caption.c_str(), { 55, 0 } ) )
                {
                    dev->roi_checked = false;
                }
            }
            ImGui::PopStyleColor();

            if( ImGui::IsItemHovered() )
                ImGui::SetTooltip( "Select custom region of interest for the auto-exposure "
                                   "algorithm\nClick the button, then draw a rect on the frame" );
        }
    }

    return res;
}

void option_model::update_supported( std::string & error_message )
{
    try
    {
        supported = endpoint->supports( opt );
    }
    catch( const error & e )
    {
        error_message = error_to_string( e );
    }
}

void option_model::update_read_only_status( std::string & error_message )
{
    try
    {
        read_only = endpoint->is_option_read_only( opt );
    }
    catch( const error & e )
    {
        error_message = error_to_string( e );
    }
}

void option_model::update_all_fields( std::string & error_message, notifications_model & model )
{
    try
    {
        if( ( supported = endpoint->supports( opt ) ) )
        {
            value = endpoint->get_option( opt );
            range = endpoint->get_option_range( opt );
            read_only = endpoint->is_option_read_only( opt );
        }
    }
    catch( const error & e )
    {
        if( read_only )
        {
            model.add_notification( { rsutils::string::from()
                                          << "Could not refresh read-only option "
                                          << endpoint->get_option_name( opt ) << ": " << e.what(),
                                      RS2_LOG_SEVERITY_WARN,
                                      RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR } );
        }
        else
            error_message = error_to_string( e );
    }
}

bool option_model::is_all_integers() const
{
    return is_integer( range.min ) && is_integer( range.max ) && is_integer( range.def )
        && is_integer( range.step );
}

bool option_model::is_enum() const
{
    // We do not expect enum values to have a step that is smaller than 1,
    // and we don't want to compare a floating point value to an integer so 0.9 will do the work.
    if( range.step < 0.9f )
        return false;

    for( auto i = range.min; i <= range.max; i += range.step )
    {
        if( endpoint->get_option_value_description( opt, i ) == nullptr )
            return false;
    }
    return true;
}

bool option_model::draw_combobox( notifications_model & model,
                                  std::string & error_message,
                                  const char * description,
                                  bool new_line,
                                  bool use_option_name )
{
    bool item_clicked = false;
    std::string txt = rsutils::string::from()
                   << ( use_option_name ? endpoint->get_option_name( opt ) : description ) << ":";

    auto pos_x = ImGui::GetCursorPosX();

    ImGui::Text( "%s", txt.c_str() );
    if( ImGui::IsItemHovered() && description )
    {
        ImGui::SetTooltip( "%s", description );
    }

    ImGui::SameLine();
    if( new_line )
        ImGui::SetCursorPosX( pos_x + 140 );

    ImGui::PushItemWidth( new_line ? -1.f : 100.f );

    std::vector< const char * > labels;
    auto selected = 0, counter = 0;
    
    for( auto i = range.min; i <= range.max; i += range.step, counter++ )
    {
        if( std::fabs( i - value ) < 0.001f )
            selected = counter;

        labels.push_back( endpoint->get_option_value_description( opt, i ) );
    }
    ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 } );

    try
    {
        int tmp_selected = selected;
        float tmp_value = value;
        ImGui::PushItemWidth( 105.f );
        if( ImGui::Combo( id.c_str(),
                          &tmp_selected,
                          labels.data(),
                          static_cast< int >( labels.size() ) ) )
        {
            tmp_value = range.min + range.step * tmp_selected;
            model.add_log( rsutils::string::from()
                           << "Setting " << opt << " to " << tmp_value << " (" << labels[tmp_selected] << ")" );
            set_option( opt, tmp_value, error_message );
            selected = tmp_selected;
            if( invalidate_flag )
                *invalidate_flag = true;
            item_clicked = true;
        }
    }
    catch( const error & e )
    {
        error_message = error_to_string( e );
    }

    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    return item_clicked;
}
bool option_model::draw_slider( notifications_model & model,
                                std::string & error_message,
                                const char * description,
                                bool use_cm_units )
{
    bool slider_clicked = false;
    std::string txt = rsutils::string::from() << endpoint->get_option_name( opt ) << ":";
    ImGui::Text( "%s", txt.c_str() );

    ImGui::SameLine();
    ImGui::SetCursorPosX( read_only ? 268.f : 245.f );
    ImGui::PushStyleColor( ImGuiCol_Text, grey );
    ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, grey );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive, { 1.f, 1.f, 1.f, 0.f } );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 1.f, 1.f, 1.f, 0.f } );
    ImGui::PushStyleColor( ImGuiCol_Button, { 1.f, 1.f, 1.f, 0.f } );
    ImGui::Button( textual_icons::question_mark, { 20, 20 } );
    ImGui::PopStyleColor( 5 );
    if( ImGui::IsItemHovered() && description )
    {
        ImGui::SetTooltip( "%s", description );
    }

    if( ! read_only )
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX( 268 );
        if( ! edit_mode )
        {
            std::string edit_id = rsutils::string::from() << textual_icons::edit << "##" << id;
            ImGui::PushStyleColor( ImGuiCol_Text, light_grey );
            ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, light_grey );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 1.f, 1.f, 1.f, 0.f } );
            ImGui::PushStyleColor( ImGuiCol_Button, { 1.f, 1.f, 1.f, 0.f } );
            if( ImGui::Button( edit_id.c_str(), { 20, 20 } ) )
            {
                if( is_all_integers() )
                    edit_value = rsutils::string::from() << (int)value;
                else
                    edit_value = rsutils::string::from() << value;
                edit_mode = true;
            }
            if( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Enter text-edit mode" );
            }
            ImGui::PopStyleColor( 4 );
        }
        else
        {
            std::string edit_id = rsutils::string::from() << textual_icons::edit << "##" << id;
            ImGui::PushStyleColor( ImGuiCol_Text, light_blue );
            ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, light_blue );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 1.f, 1.f, 1.f, 0.f } );
            ImGui::PushStyleColor( ImGuiCol_Button, { 1.f, 1.f, 1.f, 0.f } );
            if( ImGui::Button( edit_id.c_str(), { 20, 20 } ) )
            {
                edit_mode = false;
            }
            if( ImGui::IsItemHovered() )
            {
                ImGui::SetTooltip( "Exit text-edit mode" );
            }
            ImGui::PopStyleColor( 4 );
        }
    }

    ImGui::PushItemWidth( -1 );

    try
    {
        if( read_only )
        {
            ImVec2 vec{ 0, 20 };
            std::string text
                = ( value == (int)value ) ? std::to_string( (int)value ) : std::to_string( value );
            if( range.min != range.max )
            {
                ImGui::ProgressBar( ( value / ( range.max - range.min ) ), vec, text.c_str() );
            }
            else  // constant value options
            {
                auto c = ImGui::ColorConvertU32ToFloat4( ImGui::GetColorU32( ImGuiCol_FrameBg ) );
                ImGui::PushStyleColor( ImGuiCol_FrameBgActive, c );
                ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, c );
                float dummy = std::floor( value );
                if( ImGui::DragFloat( id.c_str(), &dummy, 1, 0, 0, text.c_str() ) )
                {
                    // Changing the depth units not on advanced mode is not allowed,
                    // prompt the user to switch to advanced mode for chaging it.
                    if( RS2_OPTION_DEPTH_UNITS == opt )
                    {
                        auto advanced = dev->dev.as< rs400::advanced_mode >();
                        if( advanced )
                            if( ! advanced.is_enabled() )
                                dev->draw_advanced_mode_prompt = true;
                    }
                }
                ImGui::PopStyleColor( 2 );
            }
        }
        else if( edit_mode )
        {
            std::string buff_str = edit_value;

            // lambda function used to convert meters to cm - while the number is a string
            auto convert_float_str = []( std::string float_str, float conversion_factor ) {
                if( float_str.size() == 0 )
                    return float_str;
                float number_float = std::stof( float_str );
                return std::to_string( number_float * conversion_factor );
            };

            // when cm must be used instead of meters
            if( use_cm_units )
                buff_str = convert_float_str( buff_str, 100.f );

            char buff[TEXT_BUFF_SIZE];
            memset( buff, 0, TEXT_BUFF_SIZE );
            strcpy( buff, buff_str.c_str() );

            if( ImGui::InputText( id.c_str(),
                                  buff,
                                  TEXT_BUFF_SIZE,
                                  ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                if( use_cm_units )
                {
                    buff_str = convert_float_str( std::string( buff ), 0.01f );
                    memset( buff, 0, TEXT_BUFF_SIZE );
                    strcpy( buff, buff_str.c_str() );
                }
                float new_value;
                if( ! rsutils::string::string_to_value< float >( buff, new_value ) )
                {
                    error_message = "Invalid float input!";
                }
                else if( new_value < range.min || new_value > range.max )
                {
                    float val = use_cm_units ? new_value * 100.f : new_value;
                    float min = use_cm_units ? range.min * 100.f : range.min;
                    float max = use_cm_units ? range.max * 100.f : range.max;

                    error_message = rsutils::string::from()
                                 << val << " is out of bounds [" << min << ", " << max << "]";
                }
                else
                {
                    set_option( opt, new_value, error_message );
                }
                edit_mode = false;
            }
            else if( use_cm_units )
            {
                buff_str = convert_float_str( buff_str, 0.01f );
                memset( buff, 0, TEXT_BUFF_SIZE );
                strcpy( buff, buff_str.c_str() );
            }
            edit_value = buff;
        }
        else if( is_all_integers() )
        {
            auto int_value = static_cast< int >( value );

            if( ImGui::SliderIntWithSteps( id.c_str(),
                                           &int_value,
                                           static_cast< int >( range.min ),
                                           static_cast< int >( range.max ),
                                           static_cast< int >( range.step ) ) )
            {
                // TODO: Round to step?
                slider_clicked = slider_selected( opt,
                                                  static_cast< float >( int_value ),
                                                  error_message,
                                                  model );
            }
            else
            {
                slider_clicked = slider_unselected( opt,
                                                    static_cast< float >( int_value ),
                                                    error_message,
                                                    model );
            }
        }
        else
        {
            float tmp_value = value;
            float temp_value_displayed = tmp_value;
            float min_range_displayed = range.min;
            float max_range_displayed = range.max;

            // computing the number of decimal digits taken from the step options' property
            // this will then be used to format the displayed value
            auto num_of_decimal_digits = []( float f ) {
                float f_0 = std::fabs( f - (int)f );
                std::string s = std::to_string( f_0 );
                size_t cur_len = s.length();
                // removing trailing zeros
                while( cur_len > 3 && s[cur_len - 1] == '0' )
                    cur_len--;
                return cur_len - 2;
            };
            int num_of_decimal_digits_displayed = (int)num_of_decimal_digits( range.step );

            // displaying in cm instead of meters for D405
            if( use_cm_units )
            {
                temp_value_displayed *= 100.f;
                min_range_displayed *= 100.f;
                max_range_displayed *= 100.f;
                int updated_num_of_decimal_digits_displayed = num_of_decimal_digits_displayed - 2;
                if( updated_num_of_decimal_digits_displayed > 0 )
                    num_of_decimal_digits_displayed = updated_num_of_decimal_digits_displayed;
            }

            std::stringstream formatting_ss;
            formatting_ss << "%." << num_of_decimal_digits_displayed << "f";


            if( ImGui::SliderFloat( id.c_str(),
                                    &temp_value_displayed,
                                    min_range_displayed,
                                    max_range_displayed,
                                    formatting_ss.str().c_str() ) )
            {
                tmp_value = use_cm_units ? temp_value_displayed / 100.f : temp_value_displayed;
                auto loffset = std::abs( fmod( tmp_value, range.step ) );
                auto roffset = range.step - loffset;
                if( tmp_value >= 0 )
                    tmp_value = ( loffset < roffset ) ? tmp_value - loffset : tmp_value + roffset;
                else
                    tmp_value = ( loffset < roffset ) ? tmp_value + loffset : tmp_value - roffset;
                tmp_value = ( tmp_value < range.min ) ? range.min : tmp_value;
                tmp_value = ( tmp_value > range.max ) ? range.max : tmp_value;

                slider_clicked = slider_selected( opt, tmp_value, error_message, model );
            }
            else
            {
                slider_clicked = slider_unselected( opt, tmp_value, error_message, model );
            }
        }
    }
    catch( const error & e )
    {
        error_message = error_to_string( e );
    }

    return slider_clicked;
}

bool option_model::is_checkbox() const
{
    return range.max == 1.0f && range.min == 0.0f && range.step == 1.0f;
}

bool option_model::draw_checkbox( notifications_model & model,
                                  std::string & error_message,
                                  const char * description )
{
    bool checkbox_was_clicked = false;

    auto bool_value = value > 0.0f;
    if( ImGui::Checkbox( label.c_str(), &bool_value ) )
    {
        checkbox_was_clicked = true;
        model.add_log( rsutils::string::from() << "Setting " << opt << " to " << ( bool_value ? "1.0" : "0.0" ) << " ("
                                               << ( bool_value ? "ON" : "OFF" ) << ")" );

        set_option( opt, bool_value ? 1.f : 0.f, error_message );
        if (invalidate_flag)
            *invalidate_flag = true;
    }
    if( ImGui::IsItemHovered() && description )
    {
        ImGui::SetTooltip( "%s", description );
    }
    return checkbox_was_clicked;
}

bool option_model::slider_selected( rs2_option opt,
                                    float value,
                                    std::string & error_message,
                                    notifications_model & model )
{
    bool res = false;
    auto option_was_set = set_option( opt, value, error_message, std::chrono::milliseconds( 200 ) );
    if( option_was_set )
    {
        have_unset_value = false;
        if (invalidate_flag)
            *invalidate_flag = true;
        model.add_log( rsutils::string::from() << "Setting " << opt << " to " << value );
        res = true;
    }
    else
    {
        have_unset_value = true;
        unset_value = value;
    }
    return res;
}
bool option_model::slider_unselected( rs2_option opt,
                                      float value,
                                      std::string & error_message,
                                      notifications_model & model )
{
    bool res = false;
    // Slider unselected, if last value was ignored, set with last value if the value was
    // changed.
    if( have_unset_value )
    {
        if( value != unset_value )
        {
            auto set_ok
                = set_option( opt, unset_value, error_message, std::chrono::milliseconds( 100 ) );
            if( set_ok )
            {
                model.add_log( rsutils::string::from() << "Setting " << opt << " to " << unset_value );
                if (invalidate_flag)
                    *invalidate_flag = true;
                have_unset_value = false;
                res = true;
            }
        }
        else
            have_unset_value = false;
    }
    return res;
}

bool option_model::draw_option(bool update_read_only_options,
    bool is_streaming,
    std::string& error_message, notifications_model& model)
{
    if (update_read_only_options)
    {
        update_supported(error_message);
        if (supported && is_streaming)
        {
            update_read_only_status(error_message);
            if (read_only)
            {
                update_all_fields(error_message, model);
            }
        }
    }
    if (custom_draw_method)
        return custom_draw_method(*this, error_message, model);
    else
        return draw(error_message, model);
}

bool option_model::set_option(rs2_option opt,
    float req_value,
    std::string& error_message,
    std::chrono::steady_clock::duration ignore_period)
{
    // Only set the value if `ignore_period` time past since last set_option() call for this option
    if (last_set_stopwatch.get_elapsed() < ignore_period)
        return false;

    try
    {
        last_set_stopwatch.reset();
        endpoint->set_option(opt, req_value);
    }
    catch (const error& e)
    {
        error_message = error_to_string(e);
    }

    // Only update the cached value once set_option is done! That way, if it doesn't change
    // anything...
    try
    {
        value = endpoint->get_option(opt);
    }
    catch (...)
    {
    }

    return true;
}
