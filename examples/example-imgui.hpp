// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <cmath>
#include <map>

#include "example.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>


//////////////////////////////
// ImGui Helpers            //
//////////////////////////////

//slider for ImGui
class slider {
public:
    slider(const char* name, int seq_id, float init_value, float min_value, float max_value, ImVec2 position, ImVec2 size) :
        _name(name), _seq_id(seq_id), _value(init_value), _min_value(min_value), _max_value(max_value), _position(position), _size(size) {}

    void virtual show()=0;

public:
    const char* _name;
    int _seq_id;
    float _value;
    float _max_value;
    float _min_value;
    ImVec2 _position;
    ImVec2 _size;

};

class hdr_slider : public slider {
public:
    hdr_slider(const char* name, int seq_id, float init_value, rs2::sensor& sensor,
        rs2_option option, rs2::option_range range, ImVec2 position, ImVec2 size) : slider(name, seq_id, init_value, range.min, range.max, position, size),
        _sensor(sensor), _option(option), _range(range){}

    void show() override 
    {
        ImGui::SetNextWindowSize(_size);
        ImGui::SetNextWindowPos(_position);
        //concate the name given with seq_id in order to make a unique name (uniqeness is needed for Begin())
        std::string name_id = std::string(_name) + std::to_string(_seq_id);
        ImGui::Begin(name_id.c_str(), nullptr, _sliders_flags);
        ImGui::Text("%s",_name);
        bool is_changed =
            ImGui::SliderFloat("", &_value, _min_value, _max_value, "%.3f", 5.0f, false); //5.0f for logarithmic scale 
        if (is_changed) {
            _sensor.set_option(RS2_OPTION_SEQUENCE_ID, float(_seq_id));
            _sensor.set_option(_option, _value);
        }
        ImGui::End();
    }

public:
    rs2::sensor& _sensor;
    rs2_option _option;
    rs2::option_range _range;
    //flags for the sliders
    const static int _sliders_flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoBringToFrontOnFocus;
};

//text box for ImGui
class text_box {
public:
    text_box(const char* name, ImVec2 position, ImVec2 size) : _name(name), _position(position), _size(size) {}

    void show(const char* text)
    {
        ImGui::SetNextWindowSize(_size);
        ImGui::SetNextWindowPos(_position);
        ImGui::Begin(_name, nullptr, _text_box_flags);
        ImGui::Text("%s",text);

        ImGui::End();
    }
    void remove_title_bar() {
        _text_box_flags |= ImGuiWindowFlags_NoTitleBar;
    }

public:
    const char* _name;
    ImVec2 _position;
    ImVec2 _size;
    // flags for displaying text box
    int _text_box_flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_AlwaysUseWindowPadding
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_AlwaysAutoResize;
};


class hdr_widgets {
public:
    // c'tor that creats all 4 sliders and text boxes
    // needed to init in an init list because no default c'tor for sliders and they are allocated inside hdr_widgets
    hdr_widgets(rs2::depth_sensor& depth_sensor):
        _exposure_slider_seq_1("Exposure", 1, 8000,
            depth_sensor, RS2_OPTION_EXPOSURE, depth_sensor.get_option_range(RS2_OPTION_EXPOSURE), { 130, 180 }, { 350, 40 }),
        _exposure_slider_seq_2("Exposure", 2, 18,
            depth_sensor, RS2_OPTION_EXPOSURE, depth_sensor.get_option_range(RS2_OPTION_EXPOSURE), { 390, 180 }, { 350, 40 }),
        _gain_slider_seq_1("Gain", 1, 25,
            depth_sensor, RS2_OPTION_GAIN, depth_sensor.get_option_range(RS2_OPTION_GAIN), { 130, 220 }, { 350, 40 }),
        _gain_slider_seq_2("Gain", 2, 16,
            depth_sensor, RS2_OPTION_GAIN, depth_sensor.get_option_range(RS2_OPTION_GAIN), { 390, 220 }, { 350, 40 }),
        _text_box_hdr_explain("HDR Tutorial", { 120, 20 }, { 1000, 140 }),
        _text_box_first_frame("frame 1", { 200, 150 }, { 170, 40 }),
        _text_box_second_frame("frame 2", { 460, 150 }, { 170, 40 }),
        _text_box_hdr_frame("hdr", { 850, 280 }, { 170, 40 })
    {
        // init frames map
        //for initilize only - an empty frame with its properties
        rs2::frame frame;

        //set each frame with its properties:
        //  { tile's x coordinate, tiles's y coordinate, tile's width (in tiles), tile's height (in tiles), priority (default value=0) }, (x=0,y=0) <-> left bottom corner
        //priority sets the order of drawing frame when two frames share part of the same tile, 
        //meaning if there are two frames: frame1 with priority=-1 and frame2 with priority=0, both with { 0,0,1,1 } as property,
        //frame2 will be drawn on top of frame1
        _frames_map[IR1] = frame_and_tile_property(frame, { 0,0,1,1,Priority::high });
        _frames_map[IR2] = frame_and_tile_property(frame, { 1,0,1,1,Priority::high });
        _frames_map[DEPTH1] = frame_and_tile_property(frame,{ 0,1,1,1,Priority::high });
        _frames_map[DEPTH2] = frame_and_tile_property(frame, { 1,1,1,1,Priority::high });
        _frames_map[HDR] = frame_and_tile_property(frame, { 2,0,2,2,Priority::high });
    }

    //show the features of the ImGui we have created
    //we need slider 2 to be showen before slider 1 (otherwise slider 1 padding is covering slider 2)
    void render_widgets() {

        //start a new frame of ImGui
        ImGui_ImplGlfw_NewFrame(1);

        _exposure_slider_seq_2.show();
        _exposure_slider_seq_1.show();
        _gain_slider_seq_2.show();
        _gain_slider_seq_1.show();

        _text_box_first_frame.remove_title_bar();
        _text_box_first_frame.show("Sequence 1");

        _text_box_second_frame.remove_title_bar();
        _text_box_second_frame.show("Sequence 2");

        _text_box_hdr_frame.remove_title_bar();
        _text_box_hdr_frame.show("HDR Stream");
        _text_box_hdr_explain.show("This demo provides a quick overview of the High Dynamic Range (HDR) feature.\nThe HDR configures and operates on sequences of two frames configurations, for which separate exposure and gain values are defined.\nBoth configurations are streamed and the HDR feature uses both frames in order to provide the best depth image.\nChange the values of the sliders to see the impact on the HDR Depth Image.");

        //render the ImGui features: sliders and text
        ImGui::Render();

    }

    // return a reference to frames map
    frames_mosaic& get_frames_map() {
        return _frames_map;
    }

    void update_frames_map(const rs2::video_frame& infrared_frame, const rs2::frame& depth_frame, 
        const rs2::frame& hdr_frame, rs2_metadata_type hdr_seq_id, rs2_metadata_type hdr_seq_size) {

        // frame index in frames_map are according to hdr_seq_id and hdr_seq_size
        int infrared_index = int(hdr_seq_id);
        int depth_index = int(hdr_seq_id + hdr_seq_size);
        int hdr_index = int(hdr_seq_id + hdr_seq_size + 1);

        //work-around, 'get_frame_metadata' sometimes (after changing exposure or gain values) sets hdr_seq_size to 0 even though it 2 for few frames
        //so we update the frames only if hdr_seq_size > 0. (hdr_seq_size==0 <-> frame is invalid)
        if (hdr_seq_size > 0) {
            _frames_map[infrared_index].first = infrared_frame;
            _frames_map[depth_index].first = depth_frame;
            _frames_map[hdr_index].first = hdr_frame; //HDR shall be after IR1/2 & DEPTH1/2
        }
    }

public:

    frames_mosaic _frames_map;

    hdr_slider _exposure_slider_seq_1;
    hdr_slider _gain_slider_seq_1;
    hdr_slider _exposure_slider_seq_2;
    hdr_slider _gain_slider_seq_2;

    text_box _text_box_hdr_explain;
    text_box _text_box_first_frame;
    text_box _text_box_second_frame;
    text_box _text_box_hdr_frame;

    enum frame_id { IR1, IR2, DEPTH1, DEPTH2, HDR };

};

