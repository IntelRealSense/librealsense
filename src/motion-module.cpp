// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <mutex>
#define _USE_MATH_DEFINES
#include <math.h>

#include "hw-monitor.h"
#include "motion-module.h"

using namespace rsimpl;
using namespace motion_module;

const uint8_t MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS = 0x42;
const double IMU_UNITS_TO_MSEC = 0.00003125;

motion_module_control::motion_module_control(uvc::device *device, std::timed_mutex& usbMutex) : device_handle(device), usbMutex(usbMutex), power_state(false)
{
}

int motion_module_state::requested_state(mm_request request, bool on) const
{
    int tmp = state;
    tmp += (int)request * (on ? 1 : -1);

    return tmp;
}

void motion_module_control::impose(mm_request request, bool on)
{
    std::lock_guard<std::mutex> lock(mtx);

    auto new_state = state_handler.requested_state(request, on);

    if (motion_module_state::valid(new_state))
        enter_state((mm_state)new_state);
    else
        throw std::logic_error(to_string() << "MM invalid mode from" << state_handler.state << " to " << new_state);
}

void motion_module_control::enter_state(mm_state new_state)
{
    if (new_state == state_handler.state)
        return;

    switch (state_handler.state)
    {
    case mm_idle:
        if (mm_streaming == new_state)
        {
            // Power off before power on- Ensure that we starting from scratch
            set_control(mm_events_output, false);
            set_control(mm_video_output, false);
            set_control(mm_video_output, true);
        }
        if (mm_eventing == new_state)
        {
            //  Power off before power on- Ensure that we starting from scratch
            set_control(mm_events_output, false);
            set_control(mm_video_output, false);
            set_control(mm_video_output, true); // L -shape adapter board
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Added delay between MM power on and MM start commands to be sure that the MM will be ready until start polling events.
            set_control(mm_events_output, true);
        }
        break;
    case mm_streaming:
        if (mm_idle == new_state)
        {
            set_control(mm_events_output, false);
            set_control(mm_video_output, false);
        }
        if (mm_full_load == new_state)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Added delay between MM power on and MM start commands to be sure that the MM will be ready until start polling events.
            set_control(mm_events_output, true);
        }
        if (mm_eventing == new_state)
        {            
            throw std::logic_error(" Invalid Motion Module transition from streaming to motion tracking");
        }
        break;
    case mm_eventing:
        if (mm_idle == new_state)
        {
            set_control(mm_events_output, false);
            set_control(mm_video_output, false);  //Prevent power down
        }
        if (mm_full_load == new_state)
        {
            set_control(mm_events_output, true);
        }
        if (mm_streaming == new_state)
        {
            throw std::logic_error(" Invalid Motion Module transition from motion tracking to streaming");
        }
        break;
    case mm_full_load:
        if (mm_streaming == new_state)
        {
            set_control(mm_events_output, false);
        }
        if (mm_idle == new_state)
        {
            set_control(mm_events_output, false);
            set_control(mm_video_output, false);
            throw std::logic_error(" Invalid Motion Module transition from full to idle");
        }
        break;
    default:
        break;
    }

    state_handler.state = new_state;
}

void motion_module_control::set_control(mm_request request, bool on)
{
    adaptor_board_command cmd_opcode;
    switch (request)
    {
    case mm_video_output:
        cmd_opcode = adaptor_board_command::MMPWR;
        break;
    case mm_events_output:
        cmd_opcode = adaptor_board_command::MM_ACTIVATE;
        break;
    default:
        throw std::logic_error(to_string() << " unsupported control requested :" << (int)request << " valid range is [1,2]");
    }

    hw_monitor::hwmon_cmd cmd((uint8_t)cmd_opcode);
    cmd.Param1 = (on) ? 1 : 0;

    // Motion module will always use the auxiliary USB handle (1) for
    perform_and_send_monitor_command(*device_handle, usbMutex, cmd);
}

void motion_module_control::toggle_motion_module_power(bool on)
{
    if (power_state != on)  // prevent re-entrance
    {
        // Apply user request, and update motion module controls if needed
        impose(mm_video_output, on);
        power_state = on;
    }
}

void motion_module_control::toggle_motion_module_events(bool on)
{
    // Apply user request, and update motion module controls if needed
    impose(mm_events_output, on);
}


// Write a buffer to the IAP I2C register.
void motion_module_control::i2c_iap_write(uint16_t slave_address, uint8_t *buffer, uint16_t len)
{
    hw_monitor::hwmon_cmd cmd((int)adaptor_board_command::IAP_IWB);

    cmd.Param1 = slave_address;
    cmd.Param2 = len;

    cmd.sizeOfSendCommandData = len;
    memcpy(cmd.data, buffer, len);

    perform_and_send_monitor_command(*device_handle, usbMutex, cmd);
}

// Write a 32 bit value to a specific i2c slave address.


// switch the mtion module to IAP mode.
void motion_module_control::switch_to_iap()
{
    uint32_t value = -1;

    // read state.
    hw_monitor::i2c_read_reg(static_cast<int>(adaptor_board_command::IRB), *device_handle, MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (int)i2c_register::REG_CURR_PWR_STATE, sizeof(uint32_t), reinterpret_cast<byte*>(&value));

    if ((power_states)value != power_states::PWR_STATE_IAP) {
        // we are not in IAP. switch to IAP.
        hw_monitor::i2c_write_reg(static_cast<int>(adaptor_board_command::IWB), *device_handle, MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (int)i2c_register::REG_IAP_REG, 0xAE);
    }

    // retry for 10 times to be in IAP state.
    for (int retries = 0; retries < 10; retries++) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        hw_monitor::i2c_read_reg(static_cast<int>(adaptor_board_command::IRB), *device_handle, MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (int)i2c_register::REG_CURR_PWR_STATE, sizeof(uint32_t), reinterpret_cast<byte*>(&value));
        if ((power_states)value == power_states::PWR_STATE_IAP) 
            break; // we have entered IAP
    }

    if ((power_states)value != power_states::PWR_STATE_IAP)
        throw std::runtime_error("Unable to enter IAP state!");
}

void motion_module_control::switch_to_operational()
{
    uint32_t value = -1;

    // write to the REG_JUMP_TO_APP register. this should return us to operational mode if all went well.
	hw_monitor::i2c_write_reg(static_cast<int>(adaptor_board_command::IWB), *device_handle, MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (int)i2c_register::REG_JUMP_TO_APP, 0x00);

	hw_monitor::i2c_read_reg(static_cast<int>(adaptor_board_command::IRB), *device_handle, MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (int)i2c_register::REG_CURR_PWR_STATE, sizeof(uint32_t), reinterpret_cast<byte*>(&value));
        
    if ((power_states)value != power_states::PWR_STATE_INIT)
        throw std::runtime_error("Unable to leave IAP state!");
}

// Write the firmware.
void motion_module_control::write_firmware(uint8_t *data, int size)
{
    int32_t length = size;
    uint32_t image_address = 0x8002000;
    fw_image_packet packet;
    uint8_t *data_buffer = data;

    // we considered to be in a IAP state here. we loop through the data and send it in packets of 128 bytes.
    while(length > 0)
    {
        uint16_t payload_length = (length > FW_IMAGE_PACKET_PAYLOAD_LEN) ? FW_IMAGE_PACKET_PAYLOAD_LEN : length;
        uint32_t image_address_be;
        uint16_t payload_be;

        // TODO SERGEY : The firmware needs the image address and payload length as big endian. do we have any 
        // little to big endian functions in this code ? we should detect that the machine is little endian and only 
        // then do something.
        image_address_be =  (image_address >> 24)  & 0xFF;
        image_address_be |= (image_address >> 8)   & 0xFF00;
        image_address_be |= (image_address <<  8)  & 0xFF0000;
        image_address_be |= (image_address <<  24) & 0xFF000000;

        payload_be = (payload_length >> 8) & 0xFF;
        payload_be |= (payload_length << 8) & 0xFF00;

        // packet require op_code of 0x6. can't find documentation for that.
        packet.address = image_address_be;
        packet.op_code = 0x6;
        packet.length = payload_be;
        packet.dummy = 0;

        // calcuate packet Length which includes the packet size and payload.
        uint16_t packetLength = (sizeof(packet) - FW_IMAGE_PACKET_PAYLOAD_LEN + payload_length);

        // copy data to packet.
        memcpy(packet.data, data_buffer, payload_length);

        // write to IAP.
        i2c_iap_write(MOTION_MODULE_CONTROL_I2C_SLAVE_ADDRESS, (uint8_t *)&packet, packetLength);

        // go to next packet if needed.
        data_buffer += payload_length;
        length -= payload_length;
        image_address += payload_length;
    };
}

// This function responsible for the whole firmware upgrade process.
void motion_module_control::firmware_upgrade(void *data, int size)
{
    set_control(mm_events_output, false);
    // power on motion mmodule (if needed).
    toggle_motion_module_power(true);

    // swtich to IAP if needed.
    switch_to_iap();

    //write the firmware.
    write_firmware((uint8_t *)data, size);

    // write to operational mode.
    switch_to_operational();
}

void motion_module::config(uvc::device&, uint8_t, uint8_t, uint8_t, uint8_t, uint32_t)
{
    throw std::runtime_error(to_string() << __FUNCTION__ << " is not implemented");
}


std::vector<motion_event> motion_module_parser::operator() (const unsigned char* data, const int& data_size)
{
    /* All sizes are in bytes*/
    const unsigned short motion_packet_header_size  = 8;
    const unsigned short imu_data_entries           = 4;
    const unsigned short imu_entry_size             = 12;
    const unsigned short non_imu_data_entries       = 8;        /* IMU SaS spec 3.3.2 */
    const unsigned short non_imu_entry_size         = 6;
    const unsigned short non_imu_data_offset        = motion_packet_header_size + (imu_data_entries * imu_entry_size);
    const unsigned short motion_packet_size         = non_imu_data_offset + (non_imu_data_entries * non_imu_entry_size);
    unsigned short packets = data_size / motion_packet_size;

    std::vector<motion_event> v;

    if (packets)
    {
        unsigned char *cur_packet = nullptr;

        for (uint8_t i = 0; i < packets; i++)
        {
            motion_event event_data = {};

            cur_packet = (unsigned char*)data + (i*motion_packet_size);

            // extract packet header
            memcpy(&event_data.error_state, &cur_packet[0], sizeof(unsigned short));
            memcpy(&event_data.status, &cur_packet[2], sizeof(unsigned short));
            memcpy(&event_data.imu_entries_num, &cur_packet[4], sizeof(unsigned short));
            memcpy(&event_data.non_imu_entries_num, &cur_packet[6], sizeof(unsigned short));

            if (event_data.error_state.any())
            {
                LOG_WARNING("Motion Event: packet-level error detected " << event_data.error_state.to_string() << " packet will be dropped");
                break;
            }

            // Validate header input
            if ((event_data.imu_entries_num <= imu_data_entries) && (event_data.non_imu_entries_num <= non_imu_data_entries))
            {
                // Parse IMU entries
                for (uint8_t j = 0; j < event_data.imu_entries_num; j++)
                {
                    event_data.imu_packets[j] = parse_motion(&cur_packet[motion_packet_header_size + j*imu_entry_size]);
                }

                // Parse non-IMU entries
                for (uint8_t j = 0; j < event_data.non_imu_entries_num; j++)
                {
                    parse_timestamp(&cur_packet[non_imu_data_offset + j*non_imu_entry_size], event_data.non_imu_packets[j]);
                }

                v.push_back(std::move(event_data));
            }
        }
    }

    return v;
}

void motion_module_parser::parse_timestamp(const unsigned char * data, rs_timestamp_data &entry)
{
    // assuming msb ordering
    unsigned short  tmp = (data[1] << 8) | (data[0]);

    entry.source_id = rs_event_source((tmp & 0x7) - 1);         // bits [0:2] - source_id
    entry.frame_number = mm_data_wraparound[entry.source_id].frame_counter_wraparound.fix((tmp & 0x7fff) >> 3); // bits [3-14] - frame num
    unsigned int timestamp;
    memcpy(&timestamp, &data[2], sizeof(unsigned int));   // bits [16:47] - timestamp
    entry.timestamp = mm_data_wraparound[entry.source_id].timestamp_wraparound.fix(timestamp) * IMU_UNITS_TO_MSEC; // Convert ticks to ms
}

rs_motion_data motion_module_parser::parse_motion(const unsigned char * data)
{
    // predefined motion devices parameters
    const static float gravity      = 9.80665f;                 // Standard Gravitation Acceleration
    const static float gyro_range   = 1000.f;                   // Preconfigured angular velocity range [-1000...1000] Deg_C/Sec
    const static float gyro_transform_factor = float((gyro_range * M_PI) / (180.f * 32767.f));

    const static float accel_range = 4.f;                       // Accelerometer is preset to [-4...+4]g range
    const static float accelerator_transform_factor = float(gravity * accel_range / 2048.f);

    rs_motion_data entry;

    parse_timestamp(data, (rs_timestamp_data&)entry);

    entry.is_valid = (data[1] >> 7);          // Isolate bit[15]

    // Read the motion tracking data for the three measured axes
    short tmp[3];
    memcpy(&tmp, &data[6], sizeof(short) * 3);

    unsigned data_shift = (RS_EVENT_IMU_ACCEL == entry.timestamp_data.source_id) ? 4 : 0;  // Acceleration data is stored in 12 MSB

    for (int i = 0; i < 3; i++)                     // convert axis data to physical units, (m/sec^2) or (rad/sec)
    {
        entry.axes[i] = float(tmp[i] >> data_shift);
        if (RS_EVENT_IMU_ACCEL == entry.timestamp_data.source_id) entry.axes[i] *= accelerator_transform_factor;
        if (RS_EVENT_IMU_GYRO == entry.timestamp_data.source_id) entry.axes[i] *= gyro_transform_factor;

        // TODO check and report invalid conversion requests
    }

    return entry;
}
