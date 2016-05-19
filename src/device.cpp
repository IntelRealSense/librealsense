// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "sync.h"

#include <algorithm>
#include <sstream>
#include <iostream>

using namespace rsimpl;

rs_device::rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info) : device(device), config(info), capturing(false),
    depth(config, RS_STREAM_DEPTH), color(config, RS_STREAM_COLOR), infrared(config, RS_STREAM_INFRARED), infrared2(config, RS_STREAM_INFRARED2), fisheye(config, RS_STREAM_FISHEYE),
    points(depth), rect_color(color), color_to_depth(color, depth), depth_to_color(depth, color), depth_to_rect_color(depth, rect_color), infrared2_to_depth(infrared2,depth), depth_to_infrared2(depth,infrared2)
{
    streams[RS_STREAM_DEPTH    ] = native_streams[RS_STREAM_DEPTH]     = &depth;
    streams[RS_STREAM_COLOR    ] = native_streams[RS_STREAM_COLOR]     = &color;
    streams[RS_STREAM_INFRARED ] = native_streams[RS_STREAM_INFRARED]  = &infrared;
    streams[RS_STREAM_INFRARED2] = native_streams[RS_STREAM_INFRARED2] = &infrared2;
    streams[RS_STREAM_FISHEYE  ] = native_streams[RS_STREAM_FISHEYE]   = &fisheye;
    streams[RS_STREAM_POINTS]                                          = &points;
    streams[RS_STREAM_RECTIFIED_COLOR]                                 = &rect_color;
    streams[RS_STREAM_COLOR_ALIGNED_TO_DEPTH]                          = &color_to_depth;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_COLOR]                          = &depth_to_color;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR]                = &depth_to_rect_color;
    streams[RS_STREAM_INFRARED2_ALIGNED_TO_DEPTH]                      = &infrared2_to_depth;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_INFRARED2]                      = &depth_to_infrared2;
}

rs_device::~rs_device()
{

}

bool rs_device::supports_option(rs_option option) const 
{ 
    if(uvc::is_pu_control(option)) return true;
    for(auto & o : config.info.options) if(o.option == option) return true;
    return false; 
}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {true, width, height, format, fps};
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!config.info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    config.requests[stream] = config.info.presets[stream][preset];
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {};
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

bool rs_device::supports_events_proc(rs_channel channel) const
{
    bool bRes = true;
    //TODO Evgeni
    //throw std::runtime_error("function not implemented");
    return bRes;
}

void rs_device::enable_events_proc(rs_channel channel)
{
    if (data_acquisition_active) throw std::runtime_error("channel cannot be reconfigured after having called rs_start_device()");
    
    bool bsuccess = false;

    // Use the first available channel
    for (int i = 0; i < RS_CHANNEL_NATIVE_COUNT; i++)
        if (!config.data_requests[i].enabled)
        {
            config.data_requests[i] = { true, channel };         
            bsuccess = true;
            break;
        }

    if (!bsuccess)
        throw std::runtime_error("cannot add request for the specified data channel");
}

void rs_device::disable_events_proc(rs_channel channel)
{
    if (data_acquisition_active) throw std::runtime_error("channel cannot be reconfigured after having called rs_start_device()");

    for (int i = 0; i < RS_STREAM_NATIVE_COUNT; i++)
        if ((config.data_requests[i].enabled) && (config.data_requests[i].channel == channel))
        {
            config.data_requests[i].enabled = false;
            break;
        }       
}

void rs_device::start_events()
{
    if (data_acquisition_active) throw std::runtime_error("cannot restart data acquisition without stopping first");

	std::vector<motion_events_proc_callback> mo_callbacks;
	std::vector<timestamp_events_proc_callback> ts_callbacks;
    for (int i=0; i< RS_CHANNEL_COUNT; i++)
    {
        if (config.motion_callbacks[i]) mo_callbacks.push_back(config.motion_callbacks[i]);
        if (config.timestamp_callbacks[i]) ts_callbacks.push_back(config.timestamp_callbacks[i]);
    }

    motion_module_parser parser;

    // Activate the required data channels, and provide it with user-specified data handler     
    if (config.data_requests[0].enabled)
    {
        // TODO -replace hard-coded value 3 which stands for fisheye subdevice
        set_subdevice_data_channel_handler(*device, 3,
            [mo_callbacks, ts_callbacks, parser](const unsigned char * data, const int& size) mutable
        {
            // Parse motion data
            auto events = parser(data, size);

			// Hanndle events by user-provided handlers
            for (auto & entry : events)
            {		
				// Handle Motion data packets
				for (int i = 0; i < entry.imu_entries_num; i++)
				{
					for (auto & cb : mo_callbacks)
					{
						cb(entry.imu_packets[i]);
					}
				}

				// Handle Timestamp packets
				for (int i = 0; i < entry.non_imu_entries_num; i++)
				{
					for (auto & cb : ts_callbacks)
					{
						cb(entry.non_imu_packets[i]);
					}
				}
            }
        });
    }

    start_data_acquisition(*device);     // activate polling thread in the backend
    data_acquisition_active = true;
}

void rs_device::stop_events()
{
    if (!data_acquisition_active) throw std::runtime_error("cannot stop data acquisition - is already stopped");
    stop_data_acquisition(*device);
    data_acquisition_active = false;
}

void rs_device::set_motion_event_callback(rs_channel channel, void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
{
	config.motion_callbacks[channel] = { this, on_event, user };
}

void rs_device::set_timestamp_event_callback(rs_channel channel, void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
{
	config.timestamp_callbacks[channel] = { this, on_event, user };
}


void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
    auto selected_modes = config.select_modes();
    auto archive = std::make_shared<frame_archive>(selected_modes, select_key_stream(selected_modes));
    auto timestamp_reader = create_frame_timestamp_reader();

    for(auto & s : native_streams) s->archive.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(auto mode_selection : selected_modes)
    {
        // Create a stream buffer for each stream served by this subdevice mode
        for(auto & stream_mode : mode_selection.get_outputs())
        {                    
            // If this is one of the streams requested by the user, store the buffer so they can access it
            if(config.requests[stream_mode.first].enabled) native_streams[stream_mode.first]->archive = archive;
        }

        // Initialize the subdevice and set it to the selected mode
        set_subdevice_mode(*device, mode_selection.mode.subdevice, mode_selection.mode.native_dims.x, mode_selection.mode.native_dims.y, mode_selection.mode.pf.fourcc, mode_selection.mode.fps, 
            [mode_selection, archive, timestamp_reader](const void * frame) mutable
        {
            // Ignore any frames which appear corrupted or invalid
            if(!timestamp_reader->validate_frame(mode_selection.mode, frame)) return;

            // Determine the timestamp for this frame
            int timestamp = timestamp_reader->get_frame_timestamp(mode_selection.mode, frame);

            // Obtain buffers for unpacking the frame
            std::vector<byte *> dest;
            for(auto & output : mode_selection.get_outputs()) dest.push_back(archive->alloc_frame(output.first, timestamp));

            // Unpack the frame and commit it to the archive
            mode_selection.unpack(dest.data(), reinterpret_cast<const byte *>(frame));
            for(auto & output : mode_selection.get_outputs()) archive->commit_frame(output.first);
        });
    }
    
    this->archive = archive;
    on_before_start(selected_modes);
    start_streaming(*device, config.info.num_libuvc_transfer_buffers);
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
}

void rs_device::stop()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    stop_streaming(*device);
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if(!capturing) return;
    if(!archive) return;

    archive->wait_for_frames();
}

bool rs_device::poll_all_streams()
{
    if(!capturing) return false;
    if(!archive) return false;
    return archive->poll_for_frames();
}

void rs_device::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
{
    if(uvc::is_pu_control(option))
    {
        int mn, mx, stp, df;
        uvc::get_pu_control_range(get_device(), config.info.stream_subdevices[RS_STREAM_COLOR], option, &mn, &mx, &stp, &df);
        min  = mn;
        max  = mx;
        step = stp;
        def  = df;
        return;
    }

    for(auto & o : config.info.options)
    {
        if(o.option == option)
        {
            min = o.min;
            max = o.max;
            step = o.step;
            def = o.def;
            return;
        }
    }

    throw std::logic_error("range not specified");
}

std::vector<rs_motion_event> motion_module_parser::operator() (const unsigned char* data, const int& data_size)
{
    const unsigned short motion_packet_size = 104; // bytes
    const unsigned short motion_packet_header_size = 8; // bytes
    const unsigned short imu_data_offset = motion_packet_header_size; // bytes
    const unsigned short non_imu_data_offset = 56; // bytes
    unsigned short packets = data_size / motion_packet_size;

    std::vector<rs_motion_event> v;

    if (packets)
    {
        unsigned char *cur_packet = nullptr;        

        for (uint8_t i = 0; i < packets; i++)
        {
            rs_motion_event event_data;

            cur_packet = (unsigned char*)data + (i*motion_packet_size);

            // extract packet info
            memcpy(&event_data.error_state, &cur_packet[0], sizeof(unsigned short));
            memcpy(&event_data.status,      &cur_packet[2], sizeof(unsigned short));
            memcpy(&event_data.imu_entries_num, &cur_packet[4], sizeof(unsigned short));
            memcpy(&event_data.non_imu_entries_num, &cur_packet[6], sizeof(unsigned short));

            // Parse IMU entries
            for (uint8_t j = 0; j < event_data.imu_entries_num; j++)
            {
                event_data.imu_packets[j] = parse_motion(&cur_packet[motion_packet_header_size]);
                
            }

            // Parse non-IMU entries
            for (uint8_t j = 0; j < event_data.imu_entries_num; j++)
            {
                event_data.non_imu_packets[j] = parse_timestamp(&cur_packet[non_imu_data_offset]);
            }

            v.push_back(std::move(event_data));
        }
    }
    
    return v;
    
}

rs_timestamp_data motion_module_parser::parse_timestamp(const unsigned char* data)
{
    rs_timestamp_data entry;

    // assuming msb ordering
    unsigned short  tmp     =   (data[1]<<8) | (data[0]);

    entry.source_id         =   rs_event_source(tmp&0x7);   // bits [0:2] - source_id
    entry.frame_num         =   (tmp & 0x7fff)>>3;          // bits [3-14] - frame num
    memcpy(&entry.timestamp,&data[2],sizeof(unsigned int)); // bits [16:47] - timestamp

    //std::cout << __FUNCTION__ << " TBD" << std::endl;
    
    return entry;
}

rs_motion_data motion_module_parser::parse_motion(const unsigned char* data)
{
    // predefined motion devices ranges

    const static float gravity = 9.871f;
    const static float gyro_range = 2000.f;
    const static float gyro_transform_factor = (gyro_range * 3.141527f) / (360.f * 32768.f);

    const static float accel_range = 0.00195f;   // [-4..4]g
    const static float accelerator_transform_factor = accel_range * gravity;

    rs_motion_data entry;

    entry.timestamp = parse_timestamp(data);

    entry.is_valid = data[1]& (0x80);          // bit[15]

    short tmp[3];
    memcpy(&tmp,&data[6],sizeof(short)*3);

    unsigned data_shift = (RS_IMU_ACCEL==entry.timestamp.source_id) ? 4 : 0;

    for (int i=0; i<3; i++)                     // convert axis data to physical units (m/sec^2)
    {
        entry.axes[i] = (tmp[i]>>data_shift);
        if (RS_IMU_ACCEL==entry.timestamp.source_id) entry.axes[i] *= accelerator_transform_factor;
        if (RS_IMU_GYRO==entry.timestamp.source_id) entry.axes[i] *= gyro_transform_factor;

        // TODO check and report invalid cnversion requests
    }

    return entry;
}
