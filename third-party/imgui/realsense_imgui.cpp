/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

//Overloading imgui functions to accommodate realsense GUI
#include <realsense_imgui.h>


static void ClosePopup(ImGuiID id);

bool ImGui::SeekSlider(const char* label, int* v, const char* display_format) {
    return SliderInt(label, v, 0, 100, display_format, true);
}

// center_y_ratio: 0.0f top of last item, 0.5f vertical center of last item, 1.0f bottom of last item.
void ImGui::SetScrollHere(float center_y_ratio)
{
    ImGuiWindow* window = GetCurrentWindow();
    float target_y = window->DC.CursorPosPrevLine.y + (window->DC.PrevLineTextBaseOffset * center_y_ratio) + (GImGui->Style.ItemSpacing.y * (center_y_ratio - 0.5f) * 2.0f); // Precisely aim above, in the middle or below the last line.
    SetScrollFromPosY(target_y - window->Pos.y, center_y_ratio);
}
static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
    // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    if (!*p)
        return false;
    if (out_text)
        *out_text = p;
    return true;
}
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char** items = (const char**)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}
// NB: This is an internal helper. The user-facing IsItemHovered() is using data emitted from ItemAdd(), with a slightly different logic.
bool ImGui::IsHovered(const ImRect& bb, ImGuiID id, bool flatten_childs)
{
    ImGuiContext& g = *GImGui;
    if (g.HoveredId == 0 || g.HoveredId == id || g.HoveredIdAllowOverlap)
    {
        ImGuiWindow* window = GetCurrentWindowRead();
        if (g.HoveredWindow == window || (flatten_childs && g.HoveredWindow == window->RootWindow))
            if ((g.ActiveId == 0 || g.ActiveId == id || g.ActiveIdAllowOverlap) && IsMouseHoveringRect(bb.Min, bb.Max))
                if (IsWindowContentHoverable(g.HoveredWindow))
                    return true;
    }
    return false;
}

// Provide GUI slider with values according to interval steps 
bool ImGui::SliderIntWithSteps(const char* label, int* v, int v_min, int v_max, int v_step, const char* display_format)
{
    if (!display_format)
        display_format = "%d";

    int tmp_val = *v;
    bool value_changed = ImGui::SliderInt(label, &tmp_val, v_min, v_max, display_format);

    // Round the actual slider value to the cloasest bound interval
    if (v_step > 1)
        tmp_val -= (tmp_val - v_min) % v_step;
    *v = tmp_val;

    return value_changed;
}

// Parse display precision back from the display format string
int ImGui::ParseFormatPrecision(const char* fmt, int default_precision)
{
    int precision = default_precision;
    while ((fmt = strchr(fmt, '%')) != NULL)
    {
        fmt++;
        if (fmt[0] == '%') { fmt++; continue; } // Ignore "%%"
        while (*fmt >= '0' && *fmt <= '9')
            fmt++;
        if (*fmt == '.')
        {
            precision = atoi(fmt + 1);
            if (precision < 0 || precision > 10)
                precision = default_precision;
        }
        break;
    }
    return precision;
}
float ImGui::RoundScalar(float value, int decimal_precision)
{
    // Round past decimal precision
    // So when our value is 1.99999 with a precision of 0.001 we'll end up rounding to 2.0
    // FIXME: Investigate better rounding methods
    static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
    float min_step = (decimal_precision >= 0 && decimal_precision < 10) ? min_steps[decimal_precision] : powf(10.0f, (float)-decimal_precision);
    bool negative = value < 0.0f;
    value = fabsf(value);
    float remainder = fmodf(value, min_step);
    if (remainder <= min_step * 0.5f)
        value -= remainder;
    else
        value += (min_step - remainder);
    return negative ? -value : value;
}
static void ClosePopup(ImGuiID id)
{
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_AnyPopupLevel))
        return;
    ImGuiContext& g = *GImGui;
    ImGui::ClosePopupToLevel(g.OpenPopupStack.Size - 1,true);
}

// Render a triangle to denote expanded/collapsed state
void ImGui::RenderCollapseTriangle(ImVec2 p_min, bool is_open, float scale, bool shadow)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    const float h = g.FontSize * 1.00f;
    const float r = h * 0.40f * scale;
    ImVec2 center = ImVec2(h * 0.50f+ p_min.x, h * 0.50f * scale+ p_min.y);

    ImVec2 a, b, c;
    if (is_open)
    {
        center.y -= r * 0.25f;
        a = ImVec2(0 * r+ center.x, 1 * r+ center.y) ;
        b = ImVec2(-0.866f * r+ center.x, -0.5f * r+ center.y) ;
        c = ImVec2(0.866f * r + center.x, -0.5f * r + center.y) ;
    }
    else
    {
        a =  ImVec2(1 * r+ center.x, 0 * r+ center.y) ;
        b = ImVec2(-0.500f*r+ center.x, 0.866f*r+ center.y);
        c = ImVec2(-0.500f*r+ center.x, -0.866f*r+ center.y);
    }

    if (shadow && (window->Flags) != 0)
        window->DrawList->AddTriangleFilled(ImVec2(2+a.x, 2+a.y), ImVec2(2+b.x, 2+b.y), ImVec2(2+c.x, 2+c.y), GetColorU32(ImGuiCol_BorderShadow));
    window->DrawList->AddTriangleFilled(a, b, c, GetColorU32(ImGuiCol_Text));
}

IMGUI_API bool ImGui::CustomComboBox(const char* label, int* current_item, const char* const items[], int items_count)
{
    bool value_changed = false;

    // calculate dimensions and spacing for the combo box
    ImGuiStyle& style = ImGui::GetStyle();
    float w = ImGui::CalcItemWidth();
    float spacing = style.ItemInnerSpacing.x;
    float button_sz = ImGui::GetFrameHeight();
    ImGui::PushItemWidth(w - spacing * 2.0f - button_sz);

    // the preview value - selected item
    const char* preview_value = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : "Select an item";

    if (ImGui::BeginCombo(label, ""))
    {
        //insert combobox items
        for (int i = 0; i < items_count; i++)
        {
            const bool is_selected = (i == *current_item);
            if (ImGui::Selectable(items[i], is_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Center the text in the combo box when closed
    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 size = ImGui::GetItemRectSize();
    ImVec2 textSize = ImGui::CalcTextSize(preview_value);
    float centerX = pos.x + (size.x - textSize.x) * 0.5f - 5.0f ;
    float centerY = pos.y + (size.y - textSize.y) * 0.5f;

    // draw centered text when combo box is closed
    if (!ImGui::IsItemActive())
    {
        ImGui::GetWindowDrawList()->AddText(ImVec2(centerX, centerY), ImGui::GetColorU32(ImGuiCol_Text), preview_value);
    }

    ImGui::PopItemWidth();
    return value_changed;
}
