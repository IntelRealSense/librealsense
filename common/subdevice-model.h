// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "rendering.h"
#include "ux-window.h"
#include "rs-config.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include "opengl3.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <map>
#include <set>
#include <array>
#include <unordered_map>

#include "../third-party/json.hpp"
#include "objects-in-frame.h"
#include "processing-block-model.h"

#include "realsense-ui-advanced-mode.h"
#include "fw-update-helper.h"
#include "updates-model.h"
#include "calibration-model.h"
#include <rsutils/time/periodic-timer.h>
#include <rsutils/number/stabilized-value.h>
#include "option-model.h"

namespace rs2
{
    std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec);

    bool restore_processing_block(const char* name,
        std::shared_ptr<rs2::processing_block> pb, bool enable = true);

    std::string get_device_sensor_name(subdevice_model* sub);

    class frame_queues
    {
    public:
        frame_queue& at(int id)
        {
            std::lock_guard<std::mutex> lock(_lookup_mutex);
            return _queues[id];
        }

        template<class T>
        void foreach(T action)
        {
            std::lock_guard<std::mutex> lock(_lookup_mutex);
            for (auto&& kvp : _queues)
                action(kvp.second);
        }

    private:
        std::unordered_map<int, frame_queue> _queues;
        std::mutex _lookup_mutex;
    };

    // Preserve user selections in UI
    struct subdevice_ui_selection
    {
        int selected_res_id = 0;
        std::map<int, int> selected_res_id_map; // used for depth and ir mixed resolutions
        bool is_multiple_resolutions = false; // used for depth and ir mixed resolutions
        int selected_shared_fps_id = 0;
        std::map<int, int> selected_fps_id;
        std::map<int, int> selected_format_id;
    };

    class subdevice_model
    {
    public:
        static void populate_options(std::map<int, option_model>& opt_container,
            const std::string& opt_base_label,
            subdevice_model* model,
            std::shared_ptr<options> options,
            bool* options_invalidated,
            std::string& error_message);

        subdevice_model(device& dev, std::shared_ptr<sensor> s, std::shared_ptr< atomic_objects_in_frame > objects, std::string& error_message, viewer_model& viewer, bool new_device_connected = true);
        ~subdevice_model();

        bool is_there_common_fps();
        bool supports_on_chip_calib();
        bool draw_stream_selection(std::string& error_message);
        bool is_selected_combination_supported();
        std::vector<stream_profile> get_selected_profiles(bool enforce_inter_stream_policies = true);
        std::vector<stream_profile> get_supported_profiles();
        void stop(std::shared_ptr<notifications_model> not_model);
        void play(const std::vector<stream_profile>& profiles, viewer_model& viewer, std::shared_ptr<rs2::asynchronous_syncer>);
        bool is_synchronized_frame(viewer_model& viewer, const frame& f);
        void update(std::string& error_message, notifications_model& model);
        void draw_options(const std::vector<rs2_option>& drawing_order,
            bool update_read_only_options, std::string& error_message,
            notifications_model& model);
        uint64_t num_supported_non_default_options() const;
        bool draw_option(rs2_option opt, bool update_read_only_options,
            std::string& error_message, notifications_model& model)
        {
            if (options_metadata.find(opt) != options_metadata.end())
                return options_metadata[opt].draw_option(update_read_only_options, streaming, error_message, model);
            return false;
        }

        bool is_paused() const;
        void pause();
        void resume();

        bool can_enable_zero_order();
        void verify_zero_order_conditions();

        void update_ui(std::vector<stream_profile> profiles_vec);
        void get_sorted_profiles(std::vector<stream_profile>& profiles);

        template<typename T, typename V>
        bool check_profile(stream_profile p, T cond, std::map<V, std::map<int, stream_profile>>& profiles_map,
            std::vector<stream_profile>& results, V key, int num_streams, stream_profile& def_p);

        void restore_ui_selection() { ui = last_valid_ui; }
        void store_ui_selection() { last_valid_ui = ui; }

        void get_depth_ir_mismatch_resolutions_ids(int& depth_res_id, int& ir1_res_id, int& ir2_res_id) const;

        template<typename T>
        bool get_default_selection_index(const std::vector<T>& values, const T& def, int* index)
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

        viewer_model& viewer;
        std::function<void()> on_frame = [] {};

        std::ofstream _fout;

        std::shared_ptr<sensor> s;
        device dev;
        std::shared_ptr< atomic_objects_in_frame > detected_objects;

        std::map<int, option_model> options_metadata;
        std::vector<std::string> resolutions;
        std::map<int, std::vector<std::string>> fpses_per_stream;
        std::vector<std::string> shared_fpses;
        std::map<int, std::vector<std::string>> formats;
        std::map<int, bool> stream_enabled;
        std::map<int, bool> prev_stream_enabled;
        std::map<int, std::string> stream_display_names;

        subdevice_ui_selection ui;
        subdevice_ui_selection last_valid_ui;

        std::vector<std::pair<int, int>> res_values;
        std::map<int, std::vector<std::pair<int, int>>> profile_id_to_res; // used for depth and ir mixed resolutions
        std::map<int, std::vector<int>> fps_values_per_stream;
        std::vector<int> shared_fps_values;
        bool show_single_fps_list = false;
        std::map<int, std::vector<rs2_format>> format_values;

        std::vector<stream_profile> profiles;

        frame_queues queues;
        std::mutex _queue_lock;
        bool _options_invalidated = false;
        int next_option = 0;
        std::vector<rs2_option> supported_options;
        bool streaming = false;
        std::map<int, bool> streaming_map; // used for depth and ir mixed resolutions
        bool allow_change_resolution_while_streaming = false;
        bool allow_change_fps_while_streaming = false;
        rect normalized_zoom{ 0, 0, 1, 1 };
        rect roi_rect;
        bool auto_exposure_enabled = false;
        float depth_units = 1.f;
        float stereo_baseline = -1.f;

        bool roi_checked = false;

        std::atomic<bool> _pause;
        std::atomic<bool> _is_being_recorded{ false };

        bool draw_streams_selector = true;
        bool draw_fps_selector = true;
        bool draw_advanced_mode_prompt = false;

        region_of_interest algo_roi;
        bool show_algo_roi = false;
        float roi_percentage;

        std::shared_ptr<rs2::colorizer> depth_colorizer;
        std::shared_ptr<rs2::yuy_decoder> yuy2rgb;
        std::shared_ptr<rs2::y411_decoder> y411;
        std::shared_ptr<processing_block_model> zero_order_artifact_fix;

        std::vector<std::shared_ptr<processing_block_model>> post_processing;
        bool post_processing_enabled = true;
        std::vector<std::shared_ptr<processing_block_model>> const_effects;

        bool uvmapping_calib_full = false;

    private:
        bool draw_resolutions(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1);
        bool draw_fps(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1);
        bool draw_streams_and_formats(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1);
        bool draw_res_stream_formats(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1);
        bool draw_resolutions_combo_box_multiple_resolutions(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1,
            int stream_type_id, int depth_res_id);
        bool draw_formats_combo_box_multiple_resolutions(std::string& error_message, std::string& label, std::function<void()> streaming_tooltip, float col0, float col1,
            int stream_type_id);
        bool is_multiple_resolutions_supported() const { return false; }
        std::pair<int, int> get_max_resolution(rs2_stream stream) const;
        void sort_resolutions(std::vector<std::pair<int, int>>& resolutions) const;
        bool is_ir_calibration_profile() const;

        // used in method get_max_resolution per stream
        std::map<rs2_stream, std::vector<std::pair<int, int>>> resolutions_per_stream;

        const float SHORT_RANGE_MIN_DISTANCE = 0.05f; // 5 cm
        const float SHORT_RANGE_MAX_DISTANCE = 4.0f;  // 4 meters
    };
}
