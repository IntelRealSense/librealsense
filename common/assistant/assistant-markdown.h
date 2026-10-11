// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#pragma once

#include "assistant-image-cache.h"
#include <string>

namespace rs2
{
    class ux_window;

    namespace assistant_detail
    {
        // Renders one assistant reply's markdown (CommonMark + GFM tables/strikethrough, via the
        // vendored imgui_md/md4c) as flowing ImGui content: headings get a bigger font, bold text a
        // bold font, fenced/inline code a monospace font, links are styled/clickable, and images/gifs
        // are fetched via `images` and drawn inline. imgui_md keeps no parse tree of its own, so this
        // re-parses `text` every call - same cost model as before.
        void draw_markdown_body(ux_window& win, const std::string& text, float wrap_width,
            assistant::assistant_image_cache& images, const assistant::invoke_fn& invoke);
    }
}
