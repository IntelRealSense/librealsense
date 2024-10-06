/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */
#pragma once

#include<imgui.h>
#include<imgui_internal.h>

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
    IMGUI_API void          RenderCollapseTriangle(ImVec2 p_min, bool is_open, float scale = 1.0f, bool shadow = false);
    IMGUI_API bool          SeekSlider(const char* label, int* v, const char* display_format = "%.0f%%");
    /* Implement slider with increments other than 1. The implementation is a variation on https://github.com/ocornut/imgui/issues/1183 */
    IMGUI_API bool          SliderIntWithSteps(const char* label, int* v, int v_min, int v_max, int v_step = 1, const char* display_format = "%.3f");
    IMGUI_API bool          IsHovered(const ImRect& bb, ImGuiID id, bool flatten_childs = false);
    IMGUI_API void          SetScrollHere(float center_y_ratio);
    IMGUI_API int           ParseFormatPrecision(const char* fmt, int default_precision);
    //IMGUI_API bool          VSliderFloatadd(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* display_format, float power, bool render_bg);
    IMGUI_API bool          SliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags, bool render_bg);
    IMGUI_API float         RoundScalar(float value, int decimal_precision);
    IMGUI_API bool          Combo(const char* label, int* current_item, const char** items, int items_count, int height_in_items = -1, bool show_arrow_down = true);
    IMGUI_API bool          Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items = -1, bool show_arrow_down = true);      // separate items with \0, end item-list with \0\0
    IMGUI_API bool          Combo(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1, bool show_arrow_down = true);
}
#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define ImGui_ScopePushFont(f) ImGui::ScopePushFont CONCAT(scope_push_font, __LINE__) (f)
#define ImGui_ScopePushStyleColor(idx, col) ImGui::ScopePushStyleColor CONCAT(scope_push_style_color, __LINE__) (idx, col)
#define ImGui_ScopePushStyleVar(idx, val) ImGui::ScopePushStyleVar CONCAT(scope_push_style_var, __LINE__) (idx, val)