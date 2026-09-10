// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include "control-model.h"

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rs2
{
    // A titled group of controls, and of nested groups. A group with nothing to show is not
    // drawn at all, its heading included.
    class control_section
    {
    public:
        // title is what the user reads; label is what ImGui gets, "##id" included
        control_section( std::string title, std::string label )
            : _title( std::move( title ) ), _label( std::move( label ) )
        {
        }

        void add( std::unique_ptr< control_model > control ) { _controls.push_back( std::move( control ) ); }
        // Sections are held by pointer: the controls inside one keep a reference to it, and a
        // vector of sections would move them out from under those references as it grows
        control_section & add_section( std::string title, std::string label );

        // Drawn deferred, at the panel's right edge, from the position this heading was drawn at
        std::function< void( ImVec2 ) > toggle;
        // Around this section's own controls: on_open sets the item width, on_close writes back
        // what they changed - the ctx.changed it reads covers those controls and nothing else
        std::function< void() > on_open;
        std::function< void( control_draw_context & ) > on_close;
        // A section with something to say beyond its controls
        std::function< void( control_draw_context & ) > content;
        // Air above the heading, as the filter blocks have always had it; the denser lists opt out
        bool gap_above = true;

        std::string const & title() const { return _title; }
        // Nothing to draw: no controls of its own, nothing to say, and no non-empty section under it
        bool empty() const;
        void draw( control_draw_context & );

    private:
        std::string _title;
        std::string _label;
        std::vector< std::unique_ptr< control_model > > _controls;
        std::vector< std::unique_ptr< control_section > > _sections;
    };
}
