// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "hw-monitor.h"
#include "motion_module.h"
#include <iostream>


namespace rsimpl
{
    enum CX3_GrossTete_MonitorCommand : uint32_t
    {
        IRB         = 0x01,     // Read from i2c ( 8x8 )
        IWB         = 0x02,     // Write to i2c ( 8x8 )
        GVD         = 0x03,     // Get Version and Date
        IAP_IRB     = 0x04,     // Read from IAP i2c ( 8x8 )
        IAP_IWB     = 0x05,     // Write to IAP i2c ( 8x8 )
        FRCNT       = 0x06,     // Read frame counter
        GLD         = 0x07,     // Get logger data
        GPW         = 0x08,     // Write to GPIO
        GPR         = 0x09,     // Read from GPIO
        MMPWR       = 0x0A,     // Motion module power up/down
        DSPWR       = 0x0B,     // DS4 power up/down
        EXT_TRIG    = 0x0C,     // external trigger mode
        CX3FWUPD    = 0x0D,     // FW update
        MM_ACTIVATE = 0x0E      // Motion Module activation
    };

    motion_module_control::motion_module_control(uvc::device *device): device_handle(device)
    {
        //std::cout <<  state_handler.state << std::endl;
    }

    mm_state motion_module_state::requested_state(mm_request request, bool on) const
    {
        int tmp = state;
        tmp += (int)request * (on ? 1 : -1);

        return (mm_state)tmp;
    }

    void motion_module_control::impose(mm_request request, bool on)
    {
        mm_state new_state = state_handler.requested_state(request,on);

        if (motion_module_state::valid(new_state))
            enter_state(new_state);
        else
            throw std::logic_error("ABC");
    }

    void motion_module_control::enter_state(mm_state new_state)
    {
        if (new_state == state_handler.state)
            return;

        // TODO refactor into state patters
        switch (state_handler.state)
        {
            case mm_idle:
                if (mm_streaming == new_state)
                {
                    set_control(mm_video_output, true);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                    //std::cout << "Switch from mm_idle to mm_streaming" << std::endl;
                }
                if (mm_eventing == new_state)
                {
                    set_control(mm_video_output, true);
                    set_control(mm_events_output, true);
                    //std::cout << "Switch from mm_idle to mm_eventing" << std::endl;
                }
                break;
            case mm_streaming:
                if (mm_idle == new_state)
                {
                    set_control(mm_video_output, false);
                    //std::cout << "Switch from mm_eventing to mm_idle" << std::endl;
                }
                if (mm_full_load == new_state)
                {
                    set_control(mm_events_output, true);
                    //std::cout << "Switch from mm_streaming to mm_full_load" << std::endl;
                }
                break;
            case mm_eventing:
                if (mm_idle == new_state)
                {
                    set_control(mm_events_output, false);
                    //std::cout << "Switch from mm_eventing to mm_idle" << std::endl;
                }
                break;
            default:        // void
                break;
        }

        state_handler.state = new_state;
    }

    void motion_module_control::set_control(mm_request request, bool on)
    {
        CX3_GrossTete_MonitorCommand cmd_opcode;
        switch(request)
        {
            case mm_video_output:
                cmd_opcode = CX3_GrossTete_MonitorCommand::MMPWR;
                break;
            case mm_events_output:
                cmd_opcode = CX3_GrossTete_MonitorCommand::MM_ACTIVATE;
                break;
            default:
                throw std::logic_error(to_string() << " unsupported control requested :" << (int)request << " valid range is [1,2]");
                break;
        }

        std::timed_mutex mutex;
        hw_mon::HWMonitorCommand cmd((uint8_t)cmd_opcode);
        cmd.Param1 = (on) ? 1 : 0;

        // Motion module will always use the auxillary USB handle (1) for
        rsimpl::hw_mon::perform_and_send_monitor_command(*device_handle, mutex, 1, cmd);
    }

    void motion_module_control::toggle_motion_module_power(bool on)
    {
        // Apply user request, and update motion module controls if needed
        impose(mm_video_output,on);
    }

    void motion_module_control::toggle_motion_module_events(bool on)
    {
        // Apply user request, and update motion module controls if needed
        impose(mm_events_output,on);
    }
}
