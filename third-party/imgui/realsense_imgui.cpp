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

    if (!display_format)
        display_format = "%.0f";

    float v_f = (float)*v;
    bool value_changed = SliderFloat(label, &v_f, (float)v_min, (float)v_max, display_format, 1.0f);

    *v = (int)v_f;

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

IMGUI_API bool ImGui::SliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags, bool render_bg)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = g.Style;

    const bool is_non_linear = fabsf(power - 1.0f) > 0.0001f;
    const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;


    if (!render_bg)
    {
        // Draw frame
        RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    }


    const float grab_padding = 2.0f;
    const float slider_sz = is_horizontal ? (frame_bb.GetWidth() - grab_padding * 2.0f) : (frame_bb.GetHeight() - grab_padding * 2.0f);
    float grab_sz;
    if (decimal_precision > 0)
        grab_sz = ImMin(style.GrabMinSize, slider_sz);
    else
        grab_sz = ImMin(ImMax(1.0f * (slider_sz / (v_max - v_min + 1.0f)), style.GrabMinSize), slider_sz);  // Integer sliders, if possible have the grab size represent 1 unit
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = (is_horizontal ? frame_bb.Min.x : frame_bb.Min.y) + grab_padding + grab_sz * 0.5f;
    const float slider_usable_pos_max = (is_horizontal ? frame_bb.Max.x : frame_bb.Max.y) - grab_padding - grab_sz * 0.5f;

    // For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
    float linear_zero_pos = 0.0f;   // 0.0->1.0f
    if (v_min * v_max < 0.0f)
    {
        // Different sign
        const float linear_dist_min_to_0 = powf(fabsf(0.0f - v_min), 1.0f / power);
        const float linear_dist_max_to_0 = powf(fabsf(v_max - 0.0f), 1.0f / power);
        linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0);
    }
    else
    {
        // Same sign
        linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
    }

    // Process clicking on the slider
    bool value_changed = false;
    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            const float mouse_abs_pos = is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
            float normalized_pos = ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f);
            if (!is_horizontal)
                normalized_pos = 1.0f - normalized_pos;

            float new_value;
            if (is_non_linear)
            {
                // Account for logarithmic scale on both sides of the zero
                if (normalized_pos < linear_zero_pos)
                {
                    // Negative: rescale to the negative range before powering
                    float a = 1.0f - (normalized_pos / linear_zero_pos);
                    a = powf(a, power);
                    new_value = ImLerp(ImMin(v_max, 0.0f), v_min, a);
                }
                else
                {
                    // Positive: rescale to the positive range before powering
                    float a;
                    if (fabsf(linear_zero_pos - 1.0f) > 1.e-6f)
                        a = (normalized_pos - linear_zero_pos) / (1.0f - linear_zero_pos);
                    else
                        a = normalized_pos;
                    a = powf(a, power);
                    new_value = ImLerp(ImMax(v_min, 0.0f), v_max, a);
                }
            }
            else
            {
                // Linear slider
                new_value = ImLerp(v_min, v_max, normalized_pos);
            }

            // Round past decimal precision, verify that it remains within min/max range
            new_value = RoundScalar(new_value, decimal_precision);
            if (new_value > v_max)  new_value = v_max;
            if (new_value < v_min)  new_value = v_min;

            if (*v != new_value)
            {
                *v = new_value;
                value_changed = true;
            }
        }
        else
        {
            ClearActiveID();
        }
    }

    // Calculate slider grab positioning
    float grab_t;
    if (is_non_linear)
    {
        float v_clamped = ImClamp(*v, v_min, v_max);
        if (v_clamped < 0.0f)
        {
            const float f = 1.0f - (v_clamped - v_min) / (ImMin(0.0f, v_max) - v_min);
            grab_t = (1.0f - powf(f, 1.0f / power)) * linear_zero_pos;
        }
        else
        {
            const float f = (v_clamped - ImMax(0.0f, v_min)) / (v_max - ImMax(0.0f, v_min));
            grab_t = linear_zero_pos + powf(f, 1.0f / power) * (1.0f - linear_zero_pos);
        }
    }
    else
    {
        // Linear slider
        grab_t = (ImClamp(*v, v_min, v_max) - v_min) / (v_max - v_min);
    }

    // Draw
    if (!is_horizontal)
        grab_t = 1.0f - grab_t;
    const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    ImRect grab_bb;
    if (is_horizontal)
        grab_bb = ImRect(ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
    else
        grab_bb = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));

    if (render_bg)
    {
        auto bb = frame_bb;
        ImRect fill_br = frame_bb;
        auto slider_height = bb.Max.y - bb.Min.y;
        auto slider_width = bb.Max.x - bb.Min.x;
        ImVec2 graber_size = {};
        float width = (grab_bb.Max.x - grab_bb.Min.x);
        float height = (grab_bb.Max.y - grab_bb.Min.y);
        float radius = 1.0;
        if (is_horizontal)
        {
            bb.Min.y = bb.Min.y + (slider_height / 3);
            bb.Max.y = bb.Max.y - (slider_height / 3);
            if (bb.Max.y - bb.Min.y < 1.0f)
            {
                bb.Min.y -= 0.5;
                bb.Max.y += 0.5;
            }
            float grab_paddingl = 2.0f;
            //Horizontal fills from left to right
            fill_br.Min = bb.Min;
            fill_br.Max = ImVec2(ImLerp(bb.Min.x, bb.Max.x - grab_paddingl, *v / 100), bb.Max.y);
            graber_size = { grab_bb.Max.x - (width / 2.0f) , grab_bb.Max.y - (height / 2.0f) };
            radius = height / 2.5f;
        }
        else
        {
            bb.Min.x = bb.Min.x + (slider_width / 3);
            bb.Max.x = bb.Max.x - (slider_width / 3);
            if (bb.Max.x - bb.Min.x < 1.0f)
            {
                bb.Min.x -= 0.5;
                bb.Max.x += 0.5;
            }
            //Vertical fills from down upwards
            fill_br.Min = bb.Min;
            fill_br.Min.y = grab_bb.Min.y;
            fill_br.Max = bb.Max;
            graber_size = { grab_bb.Max.x - (width / 2.0f) , grab_bb.Max.y - (height / 2.0f) };
            radius = width;// / 2.5;
        }
        // Draw frame
        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), false, 10.0f);
        RenderFrame(fill_br.Min, fill_br.Max, ImGui::ColorConvertFloat4ToU32({ 0, 112.f / 255, 197.f / 255, 1 }), false, 10.0f);
        window->DrawList->AddCircleFilled(graber_size, radius, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), 16);
    }
    else
    {
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
    }

    return value_changed;
}

// Create text input in place of a slider (when CTRL+Clicking on slider)
IMGUI_API  bool ImGui::InputScalarAsWidgetReplacement(const ImRect& aabb, const char* label, ImGuiDataType data_type, void* data_ptr, ImGuiID id, int decimal_precision)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    // Track whether the item is being used for text input
    bool start_text_input = false;

    // First, determine if we should start text input (e.g., CTRL+Click on the slider)
    if (ImGui::IsItemFocused() || (ImGui::IsItemHovered() && g.IO.MouseClicked[0]))
    {
        SetActiveID(id, window);
        FocusWindow(window);

        if (ImGui::IsItemFocused() || g.IO.KeyCtrl)
        {
            start_text_input = true;
        }
    }

    // If we are in text input mode or if the item is actively being edited, use InputText
    if (start_text_input || (g.ActiveId == id && g.ActiveIdSource == ImGuiInputSource_Keyboard))
    {
        bool text_value_changed = false;

        // Use InputScalar to handle scalar input (e.g., float, int)
        switch (data_type)
        {
        case ImGuiDataType_Float:
            text_value_changed = ImGui::InputScalar(label, data_type, data_ptr, NULL, NULL, ".3f");
            break;
        case ImGuiDataType_S32:
            text_value_changed = ImGui::InputScalar(label, data_type, data_ptr, NULL, NULL, 0);
            break;
            // Add other cases if you need support for other scalar types
        default:
            break;
        }

        if (text_value_changed)
        {
            return true; // If the value changed, return true
        }
    }
    else
    {
        // If not in text input mode, render the slider as usual
        ImGui::SliderFloat(label, (float*)data_ptr, 0.0f, 6.0f);
    }

    return false;
}

bool ImGui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* display_format, float power, bool render_bg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
    const ImRect bb(frame_bb.Min,ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x + frame_bb.Max .x: frame_bb.Max.x, frame_bb.Max.y));

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    const bool hovered = IsHovered(frame_bb, id);
    if (hovered)
        SetHoveredID(id);

    if (!display_format)
        display_format = "%.3f";
    int decimal_precision = ParseFormatPrecision(display_format, 3);

    if (hovered && g.IO.MouseClicked[0])
    {
        SetActiveID(id, window);
        FocusWindow(window);
    }

    // Actual slider behavior + render grab
    bool value_changed = SliderBehavior(frame_bb, id, v, v_min, v_max, power, decimal_precision, ImGuiSliderFlags_Vertical, render_bg);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v);
    RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL);
    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}
