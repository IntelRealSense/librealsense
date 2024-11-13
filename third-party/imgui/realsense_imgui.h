/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */
#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
    class ScopePushFont
    {
    public:
        ScopePushFont(ImFont* font) { PushFont(font); }
        ~ScopePushFont() { PopFont(); }
    };
    class ScopePushStyleColor
    {
    public:
        ScopePushStyleColor(ImGuiCol idx, const ImVec4& col) { PushStyleColor(idx, col); }
        ~ScopePushStyleColor() { PopStyleColor(); }
    };
    class ScopePushStyleVar
    {
    public:
        ScopePushStyleVar(ImGuiStyleVar idx, float val) { PushStyleVar(idx, val); }
        ScopePushStyleVar(ImGuiStyleVar idx, const ImVec2& val) { PushStyleVar(idx, val); }
        ~ScopePushStyleVar() { PopStyleVar(); }
    };
    IMGUI_API bool          SliderIntWithSteps(const char* label, int* v, int v_min, int v_max, int v_step = 1);
    IMGUI_API float         RoundScalar(float value, int decimal_precision);
    IMGUI_API bool          CustomComboBox(const char* label, int* current_item, const char* const items[], int items_count);
    IMGUI_API bool          SliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags, bool render_bg);
    IMGUI_API bool          VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* display_format, float power, bool render_bg);
    IMGUI_API bool          SliderIntTofloat(const char* label, int* v, int v_min, int v_max, const char* display_format = "%.0f");

}
#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define ImGui_ScopePushFont(f) ImGui::ScopePushFont CONCAT(scope_push_font, __LINE__) (f)
#define ImGui_ScopePushStyleColor(idx, col) ImGui::ScopePushStyleColor CONCAT(scope_push_style_color, __LINE__) (idx, col)
#define ImGui_ScopePushStyleVar(idx, val) ImGui::ScopePushStyleVar CONCAT(scope_push_style_var, __LINE__) (idx, val)