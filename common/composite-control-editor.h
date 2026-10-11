// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>
#include <imgui.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>

namespace rs2
{
    // Shared "under change" color ramp - gold (just touched) fading to
    // ImGuiCol_FrameBgHovered (about to commit). progress: 0 = just
    // touched, 1 = about to fire.
    inline ImVec4 composite_control_dirty_blend( float progress )
    {
        ImVec4 start_color( 255.f / 255.f, 210.f / 255.f, 40.f / 255.f, 90.f / 255.f );   // bright gold/yellow
        constexpr float initial_brightness = 1.2f;   // "under change" color, 20% brighter
        start_color.x = std::min( start_color.x * initial_brightness, 1.0f );
        start_color.y = std::min( start_color.y * initial_brightness, 1.0f );
        start_color.z = std::min( start_color.z * initial_brightness, 1.0f );
        const ImVec4 target_blue = ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered];
        return ImVec4(
            start_color.x + ( target_blue.x - start_color.x ) * progress,
            start_color.y + ( target_blue.y - start_color.y ) * progress,
            start_color.z + ( target_blue.z - start_color.z ) * progress,
            start_color.w + ( target_blue.w - start_color.w ) * progress );
    }

    // Reusable debounced-auto-commit editor for a composite option's struct T:
    // touch() -> fade in -> reset on retouch -> hard commit on lapse. See
    // embedded_filter_model::draw_hdrd_control_editor() for a reference usage.
    template< typename T >
    class composite_control_editor
    {
    public:
        T value{};
        bool initialized = false;

        // Named delay presets for finalize() below - public so a caller can pass one by name
        // instead of a bare literal.
        static constexpr double commit_delay = 1.7;           // seconds of quiet before auto-sending
        static constexpr double numeric_commit_delay = 0.35;  // ditto, for a plain value slider/typed number
        static constexpr double fast_commit_delay = 0.1;      // ditto, for keyboard arrow-key nudges

        // Seeds `value` from a GET the first time this is called; a no-op afterward. Returns
        // whether `value` is safe to use. `on_read`, if set, runs on the freshly-read value -
        // this generic class doesn't know T's fields, so logging is the caller's job.
        bool ensure_initialized( const std::shared_ptr< rs2::embedded_filter > & filter,
                                  rs2_composite_option_id id,
                                  std::string & error_message,
                                  const std::function< void( const T & ) > & on_read = nullptr )
        {
            if( initialized )
                return true;
            try
            {
                value = filter->get_composite_option_as< T >( id );
                initialized = true;
                if( on_read )
                    on_read( value );
            }
            catch( const std::exception & e )
            {
                error_message = e.what();
            }
            return initialized;
        }

        // Call while a field is actively being changed (every tick of a slider drag, or a
        // checkbox/radio click). Flags the group dirty and parks the deadline at +infinity so
        // nothing commits mid-edit.
        void touch()
        {
            _dirty = true;
            _commit_deadline = std::numeric_limits< double >::max();
        }

        // Call once a field's edit is finalized (slider released, or right after a checkbox/
        // radio click). Schedules auto-commit `delay_seconds` out. Default commit_delay is a
        // deliberate pause for discrete choices; numeric/fast_commit_delay are shorter, for edits/nudges.
        void finalize( double delay_seconds = commit_delay )
        {
            _commit_deadline = ImGui::GetTime() + delay_seconds;
            // try_get_progress() below needs the delay actually used, not the (possibly much
            // longer) default commit_delay - otherwise a fast_commit_delay/numeric_commit_delay
            // countdown would report itself as almost-already-committed from the very first frame.
            _active_delay = delay_seconds;
            // A discrete edit's touch()+finalize() run in the SAME frame as the draw call that
            // just set this deadline - suppress the focus-loss shortcut for that one frame so
            // it can't collapse it immediately.
            _just_finalized = true;
        }

        // Side-effect-free progress readout matching end_frame_and_maybe_commit()'s own
        // animation - lets other UI (e.g. a row-header toggle) mirror the pending-commit
        // state without duplicating the deadline math.
        bool try_get_progress( float & progress ) const
        {
            if( ! _dirty )
                return false;
            double remaining = _commit_deadline - ImGui::GetTime();
            double frac_remaining = std::min( std::max( remaining / _active_delay, 0.0 ), 1.0 );
            progress = static_cast< float >( 1.0 - frac_remaining );
            return true;
        }

        // Draws the dirty-state fill/border for [frame_min, frame_max] and sends `value` once
        // the debounce timer lapses. Delegates to the four helpers below so each concern
        // (visuals/tooltip/focus/commit) reads on its own.
        void end_frame_and_maybe_commit( const std::shared_ptr< rs2::embedded_filter > & filter,
                                          rs2_composite_option_id id,
                                          std::string & error_message,
                                          const ImVec2 & frame_min,
                                          const ImVec2 & frame_max,
                                          bool any_field_active_this_frame,
                                          const std::function< void( T & ) > & before_commit = nullptr )
        {
            draw_dirty_fill_and_border( frame_min, frame_max );
            draw_hover_tooltip( filter, id, frame_min, frame_max );
            collapse_deadline_on_focus_loss( any_field_active_this_frame );
            maybe_commit( filter, id, error_message, before_commit );
        }

    private:
        // Fades gold->blue and shrinks the border (400%->250%) over the full commit_delay
        // window while dirty; snaps to the plain idle look in one abrupt jump the instant
        // maybe_commit() actually fires.
        void draw_dirty_fill_and_border( const ImVec2 & frame_min, const ImVec2 & frame_max )
        {
            constexpr float base_border_thickness = 1.0f;
            float border_thickness = base_border_thickness;

            float progress = 0.0f;
            if( try_get_progress( progress ) )
            {
                ImVec4 blended = composite_control_dirty_blend( progress );

                ImGui::GetWindowDrawList()->AddRectFilled( frame_min, frame_max, ImGui::ColorConvertFloat4ToU32( blended ), 3.0f );
                border_thickness = base_border_thickness * ( border_start_scale + ( border_end_scale - border_start_scale ) * progress );
            }

            ImGui::GetWindowDrawList()->AddRect(
                frame_min, frame_max,
                ImGui::GetColorU32( ImGuiCol_FrameBgHovered ),
                3.0f,    // rounding
                0,       // flags
                border_thickness );
        }

        // Skipped when an inner widget is itself hovered - it already draws its own tooltip
        // (or none), and this box-wide one would otherwise cover it.
        void draw_hover_tooltip( const std::shared_ptr< rs2::embedded_filter > & filter,
                                  rs2_composite_option_id id,
                                  const ImVec2 & frame_min,
                                  const ImVec2 & frame_max )
        {
            if( ! ImGui::IsMouseHoveringRect( frame_min, frame_max ) || ImGui::IsAnyItemHovered() )
                return;
            try
            {
                ImGui::SetTooltip( "%s", filter->get_composite_option_description( id ) );
            }
            catch( const std::exception & )
            {
                // Best-effort tooltip only - a failure here shouldn't disrupt the editor.
            }
        }

        // If focus left the group entirely (not just the normal gap between fields), finish
        // the countdown now instead of making the user wait - skipped on finalize()'s own
        // frame so a fresh deadline can survive.
        void collapse_deadline_on_focus_loss( bool any_field_active_this_frame )
        {
            if( _dirty && ! any_field_active_this_frame && ImGui::IsAnyItemActive() && ! _just_finalized )
                _commit_deadline = ImGui::GetTime();
            _just_finalized = false;
        }

        // Fires once the countdown elapses quietly - checked every frame, so any fresh touch()
        // (which re-parks the deadline at +infinity) naturally defers this for as long as the
        // user keeps adjusting fields.
        void maybe_commit( const std::shared_ptr< rs2::embedded_filter > & filter,
                            rs2_composite_option_id id,
                            std::string & error_message,
                            const std::function< void( T & ) > & before_commit )
        {
            if( ! _dirty || ImGui::GetTime() < _commit_deadline )
                return;
            try
            {
                if( before_commit )
                    before_commit( value );
                filter->set_composite_option_from( id, value );
            }
            catch( const std::exception & e )
            {
                error_message = e.what();
            }
            _dirty = false;
            _commit_deadline = std::numeric_limits< double >::max();
        }

        bool _dirty = false;
        double _commit_deadline = std::numeric_limits< double >::max();
        double _active_delay = commit_delay;   // the delay finalize() last actually used - see try_get_progress()
        bool _just_finalized = false;

        static constexpr float border_start_scale = 4.0f;    // 400% of normal width, right after an edit
        static constexpr float border_end_scale = 2.5f;      // 250% of normal width, right before commit
    };
}
