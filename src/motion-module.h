// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef MOTION_MODULE_H
#define MOTION_MODULE_H

#include <mutex>
#include <bitset>

#include "device.h"

namespace rsimpl
{
    namespace motion_module
    {
        enum motion_module_errors
        {
            imu_not_responding      = 0,
            illegal_configuration   = 1,
            ts_fifo_overflow        = 2,
            imu_fifo_overflow       = 3,
            watchdog_reset          = 4,
            crc_error               = 5,
            missing_data_from_imu   = 6,
            bootloader_failure      = 7,
            eeprom_error            = 8,
            bist_check_failed       = 9,
            uctrl_not_responding    = 10,
            cx3_fifo_overflow       = 11,
            general_cx3_failure     = 12,
            cx3_usb_failure         = 13,
            reserved_1              = 14,
            reserved_2              = 15,
            max_mm_error
        };

        //RealSense DS41t SaS rev 0_60 specifications [deg/sec]
        enum class mm_gyro_range : uint8_t
        {
            gyro_range_default  = 0,
            gyro_range_1000     = 1,
            gyro_range_2000     = 2
        };

        enum class mm_gyro_bandwidth : uint8_t
        {
            gyro_bw_default = 0,
            gyro_bw_200hz   = 1
        };

        enum class mm_accel_range : uint8_t
        {
            accel_range_default = 0,
            accel_range_4g      = 1,
            accel_range_8g      = 2,
            accel_range_16g     = 3
        };

        enum class mm_accel_bandwidth : uint8_t
        {
            accel_bw_default    = 0,
            accel_bw_125hz      = 1,
            accel_bw_250hz      = 2
        };

        struct motion_event_status
        {
            unsigned reserved_0             : 1;
            unsigned pwr_mode_change_done   : 1;
            unsigned cx3_packet_number      : 4;     /* bits [2..5] packet counter number*/
            unsigned reserved_6_15          : 10;
        };

        struct motion_event
        {
            std::bitset<16>         error_state;
            motion_event_status     status;
            unsigned short          imu_entries_num;
            unsigned short          non_imu_entries_num;
            unsigned long           timestamp;
            rs_motion_data          imu_packets[4];
            rs_timestamp_data       non_imu_packets[8];
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

        #define FW_IMAGE_PACKET_PAYLOAD_LEN (128)

        struct fw_image_packet {
            uint8_t op_code;
            uint32_t address;
            uint16_t length;
            uint8_t dummy;
            uint8_t data[FW_IMAGE_PACKET_PAYLOAD_LEN];
        };
#pragma pack(pop)

        void config(uvc::device & device, uint8_t gyro_bw, uint8_t gyro_range, uint8_t accel_bw, uint8_t accel_range, uint32_t time_seed);

        enum mm_request : uint8_t
        {
            mm_output_undefined = 0,
            mm_video_output     = 1,
            mm_events_output    = 2
        };

        inline const char* get_mm_request_name(mm_request request) {
            switch (request) {
            case mm_output_undefined:   return "undefined";
            case mm_video_output:       return "video";
            case mm_events_output:      return "motion_tracking";
            default: return  std::string(to_string() << "unresolved request id: " << request).c_str();
            }
        }

        enum mm_state : uint8_t
        {
            mm_idle         = 0,    // Initial
            mm_streaming    = 1,    // FishEye video only
            mm_eventing     = 2,    // Motion data only
            mm_full_load    = 3     // Motion dat + FishEye streaming
        };

        inline const char* get_mm_state_name(mm_state state) {
            switch (state) {
            case mm_idle:           return "idle";
            case mm_streaming:      return "video";
            case mm_eventing:       return "motion";
            case mm_full_load:      return "video+motion";
            default: return  std::string(to_string() << "unresolved mm state id: " << state).c_str();
            }
        }

        struct motion_module_wraparound
        {
            motion_module_wraparound()
                : timestamp_wraparound(1, std::numeric_limits<uint32_t>::max()), frame_counter_wraparound(0, 0xfff)
            {}
            wraparound_mechanism<unsigned long long> timestamp_wraparound;
            wraparound_mechanism<unsigned long long> frame_counter_wraparound;
        };

        struct motion_module_parser
        {
            motion_module_parser()
                : mm_data_wraparound(RS_EVENT_SOURCE_COUNT)
            {}

            std::vector<motion_event> operator() (const unsigned char* data, const int& data_size);
            void parse_timestamp(const unsigned char* data, rs_timestamp_data &);
            rs_motion_data parse_motion(const unsigned char* data);

            std::vector<motion_module_wraparound> mm_data_wraparound;
        };

        class motion_module_state
        {
        public:
            motion_module_state() : state(mm_idle) {};
            mm_state state;
            int requested_state(mm_request, bool on) const;
            static bool valid(int check_state) { return ((check_state >= mm_idle) && (check_state <= mm_full_load)); }
        private:
            motion_module_state(const motion_module_state&);
        };

        class motion_module_control
        {
        public:

            motion_module_control(uvc::device *device, std::timed_mutex& usbMutex);

            void toggle_motion_module_power(bool on);
            void toggle_motion_module_events(bool on);

            void switch_to_iap();
            void switch_to_operational();

            void firmware_upgrade(void *data, int size);

        private:
            motion_module_control(const motion_module_control&);
            motion_module_state state_handler;
            uvc::device* device_handle;
            std::mutex mtx;
            std::timed_mutex& usbMutex;
            bool    power_state;

            void i2c_iap_write(uint16_t slave_address, uint8_t *buffer, uint16_t len);

            void write_firmware(uint8_t *data, int size);

            void impose(mm_request request, bool on);
            void enter_state(mm_state new_state);
            void set_control(mm_request request, bool on);

        };

        enum class i2c_register : uint32_t {
            REG_UCTRL_CFG = 0x00,
            REG_INT_ID_CONFIG = 0x04,
            REG_INT_TYPE_CONFIG = 0x08,
            REG_EEPROM_CMD = 0x0C,
            REG_EEPROM_DATA = 0xD0,
            REG_TIMER_PRESCALER = 0x14,
            REG_TIMER_RESET_VALUE = 0x18,
            REG_GYRO_BYPASS_CMD1 = 0x1C,
            REG_GYRO_BYPASS_CMD2 = 0x20,
            REG_GYRO_BYPASS_RDOUT1 = 0x24,
            REG_GYRO_BYPASS_RDOUT2 = 0x28,
            REG_ACC_BYPASS_CMD1 = 0x2C,
            REG_ACC_BYPASS_CMD2 = 0x30,
            REG_ACC_BYPASS_RDOUT1 = 0x34,
            REG_ACC_BYPASS_RDOUT2 = 0x38,
            REG_REQ_LTR = 0x3C,
            REG_STATUS_REG = 0x40,
            REG_FIFO_RD = 0x44,
            REG_FIRST_INT_IGNORE = 0x48,
            REG_IAP_REG = 0x4C,
            REG_INT_ENABLE = 0x50,
            REG_CURR_PWR_STATE = 0x54,
            REG_NEXT_PWR_STATE = 0x58,
            REG_ACC_BW = 0x5C,
            REG_GYRO_BW = 0x60,
            REG_ACC_RANGE = 0x64,
            REG_GYRO_RANGE = 0x68,
            REG_IMG_VER = 0x6C,
            REG_ERROR = 0x70,
            REG_JUMP_TO_APP = 0x77
        };

        enum class power_states : uint32_t {
            PWR_STATE_DNR = 0x00,
            PWR_STATE_INIT = 0x02,
            PWR_STATE_ACTIVE = 0x03,
            PWR_STATE_PAUSE = 0x04,
            PWR_STATE_IAP = 0x05
        };

        enum class adaptor_board_command : uint32_t
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
            MM_SNB      = 0x10,     // Get the serial number
            MM_TRB      = 0x11,     // Get the extrinsics and intrinsics data
            MM_ACTIVATE = 0x0E      // Motion Module activation
        };
    }   // namespace motion_module
}

#endif // MOTION_MODULE_H
