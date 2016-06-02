// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef MOTION_MODULE_H
#define MOTION_MODULE_H

namespace rsimpl
{
    namespace motion_module
    {
        //RealSense DS41t SaS rev 0_60 specifications
        enum class mm_gyro_range : uint8_t
        {
            gyro_range_default = 0,
            gyro_range_1000 = 1,        // Deg/sec
            gyro_range_2000 = 2
        };

        enum class mm_gyro_bandwidth : uint8_t
        {
            gyro_bw_default = 0,
            gyro_bw_200hz = 1
        };

        enum class mm_accel_range : uint8_t
        {
            accel_range_default = 0,
            accel_range_4g = 1,
            accel_range_8g = 2,
            accel_range_16g = 3
        };

        enum class mm_accel_bandwidth : uint8_t
        {
            accel_bw_default = 0,
            accel_bw_125hz = 1,
            accel_bw_250hz = 2
        };

#pragma pack(push, 1)
        struct mm_config  /* Motion module configuration data*/
        {
            uint32_t            mm_time_seed;
            mm_gyro_range       gyro_range;
            mm_gyro_bandwidth   gyro_bandwidth;
            mm_accel_range      accel_range;
            mm_accel_bandwidth  accel_bandwidth;
        };
#pragma pack(pop)

        void config(uvc::device & device, uint8_t gyro_bw, uint8_t gyro_range, uint8_t accel_bw, uint8_t accel_range, uint32_t time_seed);

        enum mm_request : uint8_t
        {
            mm_output_undefined = 0,
            mm_video_output     = 1,
            mm_events_output    = 2
        };

        enum mm_state : uint8_t
        {
            mm_idle         = 0,    // Initial
            mm_streaming    = 1,    // FishEye video only
            mm_eventing     = 2,    // Motion data only
            mm_full_load    = 3     // Motion dat + FishEye streaming
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
            motion_module_state() : state(mm_idle) {};
            mm_state state;
            mm_state requested_state(mm_request, bool on) const;
            static bool valid(mm_state check_state) { return ((check_state >= mm_idle) && (check_state <= mm_full_load)); }
        private:
            motion_module_state(const motion_module_state&);
        };

        class motion_module_control
        {
        public:

            motion_module_control(uvc::device *device);

            void toggle_motion_module_power(bool on);
            void toggle_motion_module_events(bool on);

        private:
            motion_module_control(void);
            motion_module_control(const motion_module_control&);
            motion_module_state state_handler;
            uvc::device* device_handle;
            bool    power_state;

            void impose(mm_request request, bool on);
            void enter_state(mm_state new_state);
            void set_control(mm_request request, bool on);
        };

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
    }
}

#endif // MOTION_MODULE_H
