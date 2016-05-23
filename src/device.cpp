// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "sync.h"

#include <array>
#include "image.h"

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

void rs_device::set_stream_callback(rs_stream stream, void (*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
{
    config.callbacks[stream] = {this, on_frame, user};
}

int rs_device::supports_events() const
{
    bool bRes = true;
    //TODO Evgeni
    return bRes;
}

void rs_device::enable_events()
{
    if (data_acquisition_active) throw std::runtime_error("channel cannot be reconfigured after having called rs_start_device()");

    config.data_requests.enabled = true;

}

void rs_device::disable_events()
{
    if (data_acquisition_active) throw std::runtime_error("channel cannot be reconfigured after having called rs_start_device()");

    config.data_requests.enabled = false;
}

void rs_device::start_events()
{
    if (data_acquisition_active) throw std::runtime_error("cannot restart data acquisition without stopping first");

    std::vector<motion_events_callback> mo_callbacks = config.motion_callbacks;
    std::vector<timestamp_events_callback> ts_callbacks = config.timestamp_callbacks;

    motion_module_parser parser;

    // Activate data polling handler
    if (config.data_requests.enabled)
    {
        // TODO -replace hard-coded value 3 which stands for fisheye subdevice   
        set_subdevice_data_channel_handler(*device, 3,
            [mo_callbacks, ts_callbacks, parser](const unsigned char * data, const int size) mutable
        {
            // Parse motion data
            auto events = parser(data, size);

            // Handle events by user-provided handlers
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
    data_acquisition_active = false; // todo
}

void rs_device::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
{
    config.motion_callbacks.push_back({ this, on_event, user });
}

void rs_device::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
{
    config.timestamp_callbacks.push_back({ this, on_event, user });
}


void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
    auto selected_modes = config.select_modes();
    auto archive = std::make_shared<syncronizing_archive>(selected_modes, select_key_stream(selected_modes));
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

        // Copy the callbacks that apply to this stream, so that they can be captured by value
        std::vector<frame_callback> callbacks;
        std::vector<rs_stream> streams;
        for (auto & output : mode_selection.get_outputs())
        {
            callbacks.push_back(config.callbacks[output.first]);
            streams.push_back(output.first);
        }

        // Initialize the subdevice and set it to the selected mode
        set_subdevice_mode(*device, mode_selection.mode.subdevice, mode_selection.mode.native_dims.x, mode_selection.mode.native_dims.y, mode_selection.mode.pf.fourcc, mode_selection.mode.fps, 
            [mode_selection, archive, timestamp_reader, callbacks, streams](const void * frame, std::function<void()> continuation) mutable
        {
			auto now = std::chrono::system_clock::now().time_since_epoch();
			auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            frame_continuation release_and_enqueue(continuation, frame);

            // Ignore any frames which appear corrupted or invalid
            if (!timestamp_reader->validate_frame(mode_selection.mode, frame)) return;

            // Determine the timestamp for this frame
            auto timestamp = timestamp_reader->get_frame_timestamp(mode_selection.mode, frame);
            auto frame_counter = timestamp_reader->get_frame_counter(mode_selection.mode, frame);
            
            auto requires_processing = mode_selection.requires_processing();

            
            // Obtain buffers for unpacking the frame
            std::vector<byte *> dest;
			for (auto & output : mode_selection.get_outputs()) dest.push_back(archive->alloc_frame(output.first, timestamp, frame_counter, sys_time, requires_processing));

            // Unpack the frame
            if (requires_processing)
            {
                mode_selection.unpack(dest.data(), reinterpret_cast<const byte *>(frame));
            }

            // If any frame callbacks were specified, dispatch them now
            for (size_t i = 0; i < dest.size(); ++i)
            {

                {
                    archive->attach_continuation(streams[i], std::move(release_and_enqueue));
                }

                if (callbacks[i])
                {
                    auto frame_ref = archive->track_frame(streams[i]);
                    if (frame_ref)
                    {
                        callbacks[i]((rs_frame_ref*)frame_ref);
                    }
                }
                else
                {
                    // Commit the frame to the archive
                    archive->commit_frame(streams[i]);
                }
            }
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
    archive->flush();
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

rs_frameset* rs_device::wait_all_streams_safe()
{
    if (!capturing) throw std::runtime_error("Can't call wait_for_frames_safe when the device is not capturing!");
    if (!archive) throw std::runtime_error("Can't call wait_for_frames_safe when frame archive is not available!");

        return (rs_frameset*)archive->wait_for_frames_safe();
}

bool rs_device::poll_all_streams_safe(rs_frameset** frames)
{
    if (!capturing) return false;
    if (!archive) return false;

    return archive->poll_for_frames_safe((frame_archive::frameset**)frames);
}

void rs_device::release_frames(rs_frameset * frameset)
{
    archive->release_frameset((frame_archive::frameset *)frameset);
}

rs_frameset * rs_device::clone_frames(rs_frameset * frameset)
{
    auto result = archive->clone_frameset((frame_archive::frameset *)frameset);
    if (!result) throw std::runtime_error("Not enough resources to clone frameset!");
    return (rs_frameset*)result;
}


rs_frame_ref* rs_device::detach_frame(const rs_frameset* fs, rs_stream stream)
{
    auto result = archive->detach_frame_ref((frame_archive::frameset *)fs, stream);
    if (!result) throw std::runtime_error("Not enough resources to tack detached frame!");
    return (rs_frame_ref*)result;
}

void rs_device::release_frame(rs_frame_ref* ref)
{
    archive->release_frame_ref((frame_archive::frame_ref *)ref);
}

rs_frame_ref* ::rs_device::clone_frame(rs_frame_ref* frame)
{
    auto result = archive->clone_frame((frame_archive::frame_ref *)frame);
    if (!result) throw std::runtime_error("Not enough resources to clone frame!");
    return (rs_frame_ref*)result;
}

bool rs_device::supports(rs_capabilities capability) const
{
    for (auto elem: config.info.capabilities_vector)
    {
        if (elem == capability)
            return true;
    }

    return false;
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

std::vector<motion_event> motion_module_parser::operator() (const unsigned char* data, const int& data_size)
{
    const unsigned short motion_packet_size = 104; // bytes
    const unsigned short motion_packet_header_size = 8; // bytes
    const unsigned short non_imu_data_offset = 56; // bytes
    unsigned short packets = data_size / motion_packet_size;

    std::vector<motion_event> v;

    if (packets)
    {
        unsigned char *cur_packet = nullptr;        

        for (uint8_t i = 0; i < packets; i++)
        {
            motion_event event_data;

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
                parse_timestamp(&cur_packet[non_imu_data_offset],event_data.non_imu_packets[j]);
            }

            v.push_back(std::move(event_data));
        }
    }
    
    return v;
    
}

void motion_module_parser::parse_timestamp(const unsigned char * data,rs_timestamp_data &entry )
{
    // assuming msb ordering
    unsigned short  tmp     =   (data[1]<<8) | (data[0]);

    entry.source_id         =   rs_event_source(tmp&0x7);   // bits [0:2] - source_id
    entry.frame_number         =   (tmp & 0x7fff)>>3;          // bits [3-14] - frame num
    memcpy(&entry.timestamp,&data[2],sizeof(unsigned int)); // bits [16:47] - timestamp

}

rs_motion_data motion_module_parser::parse_motion(const unsigned char * data)
{
    // predefined motion devices ranges

    const static float gravity = 9.871f;
    const static float gyro_range = 2000.f;
    const static float gyro_transform_factor = (gyro_range * 3.141527f) / (360.f * 32768.f);

    const static float accel_range = 0.00195f;   // [-4..4]g
    const static float accelerator_transform_factor = accel_range * gravity;

    rs_motion_data entry;

    //entry::rs_timestamp_data =
    parse_timestamp(data, (rs_timestamp_data&)entry);

    entry.is_valid = data[1]& (0x80);          // bit[15]

    short tmp[3];
    memcpy(&tmp,&data[6],sizeof(short)*3);

    unsigned data_shift = (RS_IMU_ACCEL == entry.timestamp_data.source_id) ? 4 : 0;

    for (int i=0; i<3; i++)                     // convert axis data to physical units (m/sec^2)
    {
        entry.axes[i] = (tmp[i]>>data_shift);
        if (RS_IMU_ACCEL == entry.timestamp_data.source_id) entry.axes[i] *= accelerator_transform_factor;
        if (RS_IMU_GYRO == entry.timestamp_data.source_id) entry.axes[i] *= gyro_transform_factor;

        // TODO check and report invalid cnversion requests
    }

    return entry;
}
