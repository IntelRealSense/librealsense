// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef MOTION_MODULE_H
#define MOTION_MODULE_H


namespace rsimpl
{
    enum mm_request : uint8_t
    {
        mm_output_undefined = 0,
        mm_video_output     = 1,
        mm_events_output    = 2
    };

    enum mm_state : uint8_t
    {
        mm_undefined    = 0,
        mm_idle         = 1,    // Initial
        mm_streaming    = 2,    // FishEye video only
        mm_eventing     = 3,    // Motion data only
        mm_full_load    = 4     // Motion dat + FishEye streaming
    };

	struct motion_module_parser
	{
		std::vector<motion_event> operator() (const unsigned char* data, const int& data_size);
		void parse_timestamp(const unsigned char* data, rs_timestamp_data &);
		rs_motion_data parse_motion(const unsigned char* data);
	};

    class motion_module_state
    {
    public:
        motion_module_state() : state(mm_idle){};
        mm_state state;
        mm_state requested_state(mm_request, bool on) const;
        static bool valid(mm_state check_state) { return ((check_state >= mm_idle) && (check_state <= mm_full_load)); }
    private:
        motion_module_state(const motion_module_state&);
    };

    class motion_module_control
    {
    public :

        motion_module_control(uvc::device *device);//: device_handle(device){};

        void toggle_motion_module_power(bool on);
        void toggle_motion_module_events(bool on);

    private:
        motion_module_control(void);
        motion_module_control(const motion_module_control&);
        motion_module_state state_handler;
        uvc::device* device_handle;

        void impose(mm_request request, bool on);
        void enter_state(mm_state new_state);
        void set_control(mm_request request, bool on);
    };

}

#endif // MOTION_MODULE_H
