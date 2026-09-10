// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include "control-section.h"

#include <realsense_imgui.h>


namespace rs2
{
    control_section & control_section::add_section( std::string title, std::string label )
    {
        _sections.push_back( std::unique_ptr< control_section >(
            new control_section( std::move( title ), std::move( label ) ) ) );
        return *_sections.back();
    }

    void control_section::draw( control_draw_context & ctx )
    {
        if( gap_above )
            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 5 );
        if( toggle )
        {
            ImVec2 const pos = ImGui::GetCursorPos();
            auto draw_toggle = toggle;
            ctx.draw_later.push_back( [draw_toggle, pos]() { draw_toggle( pos ); } );
        }

        if( ! ImGui::TreeNode( _label.c_str() ) )
            return;

        // A failed write to the camera propagates out of on_close; the tree still has to close
        struct tree_closer { ~tree_closer() { ImGui::TreePop(); } } close_tree;

        bool const changed_before = ctx.changed;
        ctx.changed = false;

        if( on_open )
            on_open();
        for( auto & control : _controls )
            if( control->drawable() )
                control->draw( ctx );
        if( on_close )
            on_close( ctx );

        ctx.changed = ctx.changed || changed_before;

        if( content )
            content( ctx );

        for( auto & section : _sections )
            section->draw( ctx );
    }
}
