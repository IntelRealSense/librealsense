// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense/rs2.hpp>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <map>
#include <set>
#include <array>
#include "example.hpp"

namespace ImGui {
/*
    tabLabels: name of all tabs involved
    tabSize: number of elements
    tabIndex: holds the current active tab
    tabOrder: optional array of integers from 0 to tabSize-1 that maps the tab label order. If one of the numbers is replaced by -1 the tab label is not visible (closed). It can be read/modified at runtime.

    // USAGE EXAMPLE
    static const char* tabNames[] = {"First tab","Second tab","Third tab"};
    static int tabOrder[] = {0,1,2};
    static int tabSelected = 0;
    const bool tabChanged = ImGui::TabLabels(tabNames,sizeof(tabNames)/sizeof(tabNames[0]),tabSelected,tabOrder);
    ImGui::Text("\nTab Page For Tab: \"%s\" here.\n",tabNames[tabSelected]);
*/

inline IMGUI_API bool TabLabels(const char **tabLabels, int tabSize, int &tabIndex, int *tabOrder=NULL) {
    ImGuiStyle& style = ImGui::GetStyle();

    const ImVec2 itemSpacing =  style.ItemSpacing;
    const ImVec4 color =        style.Colors[ImGuiCol_Button];
    const ImVec4 colorActive =  style.Colors[ImGuiCol_ButtonActive];
    const ImVec4 colorHover =   style.Colors[ImGuiCol_ButtonHovered];
    const ImVec4 colorText =    style.Colors[ImGuiCol_Text];
    style.ItemSpacing.x =       1;
    style.ItemSpacing.y =       1;
    const ImVec4 colorSelectedTab = ImVec4(color.x,color.y,color.z,color.w*0.5f);
    const ImVec4 colorSelectedTabHovered = ImVec4(colorHover.x,colorHover.y,colorHover.z,colorHover.w*0.5f);
    const ImVec4 colorSelectedTabText = ImVec4(colorText.x*0.8f,colorText.y*0.8f,colorText.z*0.8f,colorText.w*0.8f);

    if (tabSize>0 && (tabIndex<0 || tabIndex>=tabSize)) {
        if (!tabOrder)  tabIndex = 0;
        else tabIndex = -1;
    }

    float windowWidth = 0.f,sumX=0.f;
    windowWidth = ImGui::GetWindowWidth() - style.WindowPadding.x - (ImGui::GetScrollMaxY()>0 ? style.ScrollbarSize : 0.f);

    static int draggingTabIndex = -1;int draggingTabTargetIndex = -1;   // These are indices inside tabOrder
    static ImVec2 draggingtabSize(0,0);
    static ImVec2 draggingTabOffset(0,0);

    const bool isMMBreleased = ImGui::IsMouseReleased(2);
    const bool isMouseDragging = ImGui::IsMouseDragging(0,2.f);
    int justClosedTabIndex = -1,newtabIndex = tabIndex;


    bool selection_changed = false;bool noButtonDrawn = true;
    for (int j = 0,i; j < tabSize; j++)
    {
        i = tabOrder ? tabOrder[j] : j;
        if (i==-1) continue;

        if (sumX > 0.f) {
            sumX+=style.ItemSpacing.x;   // Maybe we can skip it if we use SameLine(0,0) below
            sumX+=ImGui::CalcTextSize(tabLabels[i]).x+2.f*style.FramePadding.x;
            if (sumX>windowWidth) sumX = 0.f;
            else ImGui::SameLine();
        }

        if (i != tabIndex) {
            // Push the style
            style.Colors[ImGuiCol_Button] =         colorSelectedTab;
            style.Colors[ImGuiCol_ButtonActive] =   colorSelectedTab;
            style.Colors[ImGuiCol_ButtonHovered] =  colorSelectedTabHovered;
            style.Colors[ImGuiCol_Text] =           colorSelectedTabText;
        }
        // Draw the button
        ImGui::PushID(i);   // otherwise two tabs with the same name would clash.
        if (ImGui::Button(tabLabels[i]))   {selection_changed = (tabIndex!=i);newtabIndex = i;}
        ImGui::PopID();
        if (i != tabIndex) {
            // Reset the style
            style.Colors[ImGuiCol_Button] =         color;
            style.Colors[ImGuiCol_ButtonActive] =   colorActive;
            style.Colors[ImGuiCol_ButtonHovered] =  colorHover;
            style.Colors[ImGuiCol_Text] =           colorText;
        }
        noButtonDrawn = false;

        if (sumX==0.f) sumX = style.WindowPadding.x + ImGui::GetItemRectSize().x; // First element of a line

        if (ImGui::IsItemHoveredRect()) {
            if (tabOrder)  {
                // tab reordering
                if (isMouseDragging) {
                    if (draggingTabIndex==-1) {
                        draggingTabIndex = j;
                        draggingtabSize = ImGui::GetItemRectSize();
                        const ImVec2& mp = ImGui::GetIO().MousePos;
                        const ImVec2 draggingTabCursorPos = ImGui::GetCursorPos();
                        draggingTabOffset=ImVec2(
                                    mp.x+draggingtabSize.x*0.5f-sumX+ImGui::GetScrollX(),
                                    mp.y+draggingtabSize.y*0.5f-draggingTabCursorPos.y+ImGui::GetScrollY()
                                    );

                    }
                }
                else if (draggingTabIndex>=0 && draggingTabIndex<tabSize && draggingTabIndex!=j){
                    draggingTabTargetIndex = j; // For some odd reasons this seems to get called only when draggingTabIndex < i ! (Probably during mouse dragging ImGui owns the mouse someway and sometimes ImGui::IsItemHovered() is not getting called)
                }
            }
        }

    }

    tabIndex = newtabIndex;

    // Draw tab label while mouse drags it
    if (draggingTabIndex>=0 && draggingTabIndex<tabSize) {
        const ImVec2& mp = ImGui::GetIO().MousePos;
        const ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 start(wp.x+mp.x-draggingTabOffset.x-draggingtabSize.x*0.5f,wp.y+mp.y-draggingTabOffset.y-draggingtabSize.y*0.5f);
        const ImVec2 end(start.x+draggingtabSize.x,start.y+draggingtabSize.y);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float draggedBtnAlpha = 0.65f;
        const ImVec4& btnColor = style.Colors[ImGuiCol_Button];
        drawList->AddRectFilled(start,end,ImColor(btnColor.x,btnColor.y,btnColor.z,btnColor.w*draggedBtnAlpha),style.FrameRounding);
        start.x+=style.FramePadding.x;start.y+=style.FramePadding.y;
        const ImVec4& txtColor = style.Colors[ImGuiCol_Text];
        drawList->AddText(start,ImColor(txtColor.x,txtColor.y,txtColor.z,txtColor.w*draggedBtnAlpha),tabLabels[tabOrder[draggingTabIndex]]);

        ImGui::SetMouseCursor(ImGuiMouseCursor_Move);
    }

    // Drop tab label
    if (draggingTabTargetIndex!=-1) {
        // swap draggingTabIndex and draggingTabTargetIndex in tabOrder
        const int tmp = tabOrder[draggingTabTargetIndex];
        tabOrder[draggingTabTargetIndex] = tabOrder[draggingTabIndex];
        tabOrder[draggingTabIndex] = tmp;
        //fprintf(stderr,"%d %d\n",draggingTabIndex,draggingTabTargetIndex);
        draggingTabTargetIndex = draggingTabIndex = -1;
    }

    // Reset draggingTabIndex if necessary
    if (!isMouseDragging) draggingTabIndex = -1;

    // Change selected tab when user closes the selected tab
    if (tabIndex == justClosedTabIndex && tabIndex>=0)    {
        tabIndex = -1;
        for (int j = 0,i; j < tabSize; j++) {
            i = tabOrder ? tabOrder[j] : j;
            if (i==-1) continue;
            tabIndex = i;
            break;
        }
    }

    // Restore the style
    style.Colors[ImGuiCol_Button] =         color;
    style.Colors[ImGuiCol_ButtonActive] =   colorActive;
    style.Colors[ImGuiCol_ButtonHovered] =  colorHover;
    style.Colors[ImGuiCol_Text] =           colorText;
    style.ItemSpacing =                     itemSpacing;

    return selection_changed;
}
} // namespace ImGui

namespace rs2
{
    class subdevice_model;

    template<class T>
    void push_back_if_not_exists(std::vector<T>& vec, T value)
    {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it == vec.end()) vec.push_back(value);
    }

    struct frame_metadata
    {
        std::array<std::pair<bool,rs2_metadata_t>,RS2_FRAME_METADATA_COUNT> md_attributes{};
    };

    typedef std::map<rs2_stream, rect> streams_layout;

    class option_model
    {
    public:
        void draw(std::string& error_message);
        void update_supported(std::string& error_message);
        void update_read_only(std::string& error_message);
        void update_all(std::string& error_message);

        rs2_option opt;
        option_range range;
        sensor endpoint;
        bool* invalidate_flag;
        bool supported = false;
        bool read_only = false;
        float value = 0.0f;
        std::string label = "";
        std::string id = "";
        subdevice_model* dev;

    private:
        bool is_all_integers() const;
        bool is_enum() const;
        bool is_checkbox() const;
    };

    class subdevice_model
    {
    public:
        subdevice_model(device& dev, sensor& s, std::string& error_message);
        bool is_there_common_fps() ;
        void draw_stream_selection();
        bool is_selected_combination_supported();
        std::vector<stream_profile> get_selected_profiles();
        void stop();
        void play(const std::vector<stream_profile>& profiles);
        void update(std::string& error_message);

        bool is_paused() const;
        void pause();
        void resume();

        template<typename T>
        bool get_default_selection_index(const std::vector<T>& values, const T & def, int* index)
        {
            auto max_default = values.begin();
            for (auto it = values.begin(); it != values.end(); it++)
            {

                if (*it == def)
                {
                    *index = (int)(it - values.begin());
                    return true;
                }
                if (*max_default < *it)
                {
                    max_default = it;
                }
            }
            *index = (int)(max_default - values.begin());
            return false;
        }

        sensor s;
        device dev;

        std::map<rs2_option, option_model> options_metadata;
        std::vector<std::string> resolutions;
        std::map<rs2_stream, std::vector<std::string>> fpses_per_stream;
        std::vector<std::string> shared_fpses;
        std::map<rs2_stream, std::vector<std::string>> formats;
        std::map<rs2_stream, bool> stream_enabled;

        int selected_res_id = 0;
        std::map<rs2_stream, int> selected_fps_id;
        int selected_shared_fps_id = 0;
        std::map<rs2_stream, int> selected_format_id;

        std::vector<std::pair<int, int>> res_values;
        std::map<rs2_stream, std::vector<int>> fps_values_per_stream;
        std::vector<int> shared_fps_values;
        bool show_single_fps_list = false;
        std::map<rs2_stream, std::vector<rs2_format>> format_values;

        std::vector<stream_profile> profiles;

        std::vector<std::unique_ptr<frame_queue>> queues;
        bool options_invalidated = false;
        int next_option = RS2_OPTION_COUNT;
        bool streaming = false;

        rect normalized_zoom{0, 0, 1, 1};
        rect roi_rect;
        bool auto_exposure_enabled = false;
        float depth_units = 1.f;

        bool roi_checked = false;

        std::atomic<bool> _pause;
    };

    class stream_model
    {
    public:
        stream_model();
        void upload_frame(frame&& f);
        void outline_rect(const rect& r);
        float get_stream_alpha();
        bool is_stream_visible();
        void update_ae_roi_rect(const rect& stream_rect, const mouse_info& mouse, std::string& error_message);
        void show_frame(const rect& stream_rect, const mouse_info& g, std::string& error_message);
        void show_metadata(const mouse_info& g);
        rect get_normalized_zoom(const rect& stream_rect, const mouse_info& g, bool is_middle_clicked, float zoom_val);

        rect layout;
        std::unique_ptr<texture_buffer> texture;
        float2 size;
        rect get_stream_bounds() const { return { 0, 0, size.x, size.y }; }

        rs2_stream stream;
        rs2_format format;
        std::chrono::high_resolution_clock::time_point last_frame;
        double              timestamp;
        unsigned long long  frame_number;
        rs2_timestamp_domain timestamp_domain;
        fps_calc            fps;
        rect                roi_display_rect{};
        frame_metadata      frame_md;
        bool                metadata_displayed  = false;
        bool                capturing_roi       = false;    // active modification of roi
        std::shared_ptr<subdevice_model> dev;
        float _frame_timeout = 700.0f;
        float _min_timeout = 167.0f;

        bool _mid_click = false;
        float2 _middle_pos{0, 0};
        rect _normalized_zoom{0, 0, 1, 1};
        int color_map_idx = 1;
        bool show_stream_details = false;
    };

    class device_model
    {
    public:
        device_model(){}
        void reset();
        explicit device_model(device& dev, std::string& error_message);
        bool draw_combo_box(const std::vector<std::string>& device_names, int& new_index);
        void draw_device_details(device& dev);
        std::map<rs2_stream, rect> calc_layout(float x0, float y0, float width, float height);
        void upload_frame(frame&& f);

        std::vector<std::shared_ptr<subdevice_model>> subdevices;
        std::map<rs2_stream, stream_model> streams;
        bool fullscreen = false;
        bool metadata_supported = false;
        rs2_stream selected_stream = RS2_STREAM_ANY;

    private:
        std::map<rs2_stream, rect> get_interpolated_layout(const std::map<rs2_stream, rect>& l);

        streams_layout _layout;
        streams_layout _old_layout;
        std::chrono::high_resolution_clock::time_point _transition_start_time;
    };


    struct notification_data
    {
        notification_data(std::string description,
                            double timestamp,
                            rs2_log_severity severity,
                            rs2_notification_category category);
        rs2_notification_category get_category() const;
        std::string get_description() const;
        double get_timestamp() const;
        rs2_log_severity get_severity() const;

        std::string _description;
        double _timestamp;
        rs2_log_severity _severity;
        rs2_notification_category _category;
    };

    struct notification_model
    {
        notification_model();
        notification_model(const notification_data& n);
        double get_age_in_ms();
        void draw(int w, int y, notification_model& selected);
        void set_color_scheme(float t);

        static const int MAX_LIFETIME_MS = 10000;
        int height = 60;
        int index;
        std::string message;
        double timestamp;
        rs2_log_severity severity;
        std::chrono::high_resolution_clock::time_point created_time;
        // TODO: Add more info
    };

    struct notifications_model
    {
        void add_notification(const notification_data& n);
        void draw(int w, int h, notification_model& selected);

        std::vector<notification_model> pending_notifications;
        int index = 1;
        const int MAX_SIZE = 6;
        std::mutex m;
    };
}
