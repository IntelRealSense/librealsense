// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "sync.h"
#include "motion-module.h"
#include "hw-monitor.h"
#include "image.h"

#include <array>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <functional>

using namespace rsimpl;
using namespace rsimpl::motion_module;

const int NUMBER_OF_FRAMES_TO_SAMPLE = 5;

rs_device_base::rs_device_base(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info, calibration_validator validator) : device(device), config(info),
    depth(config, RS_STREAM_DEPTH, validator), color(config, RS_STREAM_COLOR, validator), infrared(config, RS_STREAM_INFRARED, validator), infrared2(config, RS_STREAM_INFRARED2, validator), fisheye(config, RS_STREAM_FISHEYE, validator),
    points(depth), rect_color(color), color_to_depth(color, depth), depth_to_color(depth, color), depth_to_rect_color(depth, rect_color), infrared2_to_depth(infrared2,depth), depth_to_infrared2(depth,infrared2),
    capturing(false), data_acquisition_active(false), max_publish_list_size(RS_USER_QUEUE_SIZE), event_queue_size(RS_MAX_EVENT_QUEUE_SIZE), events_timeout(RS_MAX_EVENT_TIME_OUT),
    usb_port_id(""), motion_module_ready(false), keep_fw_logger_alive(false), frames_drops_counter(0)
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

rs_device_base::~rs_device_base()
{
    try
    {
        if (capturing) 
            stop(RS_SOURCE_VIDEO);
        if (data_acquisition_active)
            stop(RS_SOURCE_MOTION_TRACKING);
        if (keep_fw_logger_alive)
            stop_fw_logger();
    }
    catch (...) {}
}

bool rs_device_base::supports_option(rs_option option) const 
{ 
    if(uvc::is_pu_control(option)) return true;
    for(auto & o : config.info.options) if(o.option == option) return true;
    return false; 
}

const char* rs_device_base::get_camera_info(rs_camera_info info) const
{ 
    auto it = config.info.camera_info.find(info);
    if (it == config.info.camera_info.end())
        throw std::runtime_error("selected camera info is not supported for this camera!"); 
    return it->second.c_str();
}

void rs_device_base::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = { true, width, height, format, fps, output };
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device_base::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!config.info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    config.requests[stream] = config.info.presets[stream][preset];
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

rs_motion_intrinsics rs_device_base::get_motion_intrinsics() const
{
    throw std::runtime_error("Motion intrinsic is not supported for this device");
}

rs_extrinsics rs_device_base::get_motion_extrinsics_from(rs_stream /*from*/) const
{
    throw std::runtime_error("Motion extrinsics does not supported");
}

void rs_device_base::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.callbacks[stream] = {};
    config.requests[stream] = {};
    for(auto & s : native_streams) s->archive.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device_base::set_stream_callback(rs_stream stream, void (*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user)
{
    config.callbacks[stream] = frame_callback_ptr(new frame_callback{ this, on_frame, user });
}

void rs_device_base::set_stream_callback(rs_stream stream, rs_frame_callback* callback)
{
    config.callbacks[stream] = frame_callback_ptr(callback);
}

void rs_device_base::enable_motion_tracking()
{
    if (data_acquisition_active) throw std::runtime_error("motion-tracking cannot be reconfigured after having called rs_start_device()");

    config.data_request.enabled = true;
}

void rs_device_base::disable_motion_tracking()
{
    if (data_acquisition_active) throw std::runtime_error("motion-tracking disabled after having called rs_start_device()");

    config.data_request.enabled = false;
}

void rs_device_base::set_motion_callback(rs_motion_callback* callback)
{
    if (data_acquisition_active) throw std::runtime_error("cannot set motion callback when motion data is active");
    
    // replace previous, if needed
    config.motion_callback = motion_callback_ptr(callback, [](rs_motion_callback* c) { c->release(); });
}

void rs_device_base::start_motion_tracking()
{
    if (data_acquisition_active) throw std::runtime_error("cannot restart data acquisition without stopping first");

    auto parser = std::make_shared<motion_module_parser>();

    // Activate data polling handler
    if (config.data_request.enabled)
    {
        // TODO -replace hard-coded value 3 which stands for fisheye subdevice   
        set_subdevice_data_channel_handler(*device, 3,
            [this, parser](const unsigned char * data, const int size) mutable
        {
            if (motion_module_ready)    //  Flush all received data before MM is fully operational 
            {
                // Parse motion data
                auto events = (*parser)(data, size);

                // Handle events by user-provided handlers
                for (auto & entry : events)
                {
                    // Handle Motion data packets
                    if (config.motion_callback)
                        for (int i = 0; i < entry.imu_entries_num; i++)
                            config.motion_callback->on_event(entry.imu_packets[i]);

                    // Handle Timestamp packets
                    if (config.timestamp_callback)
                    {
                        for (int i = 0; i < entry.non_imu_entries_num; i++)
                        {
                            auto tse = entry.non_imu_packets[i];
                            if (archive)
                                archive->on_timestamp(tse);

                            config.timestamp_callback->on_event(entry.non_imu_packets[i]);
                        }
                    }
                }
            }
        });
    }

    start_data_acquisition(*device);     // activate polling thread in the backend
    data_acquisition_active = true;
}

void rs_device_base::stop_motion_tracking()
{
    if (!data_acquisition_active) throw std::runtime_error("cannot stop data acquisition - is already stopped");
    stop_data_acquisition(*device);
    data_acquisition_active = false;
}

void rs_device_base::set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user)
{
    if (data_acquisition_active) throw std::runtime_error("cannot set motion callback when motion data is active");
    
    // replace previous, if needed
    config.motion_callback = motion_callback_ptr(new motion_events_callback(this, on_event, user), [](rs_motion_callback* c) { delete c; });
}

void rs_device_base::set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user)
{
    if (data_acquisition_active) throw std::runtime_error("cannot set timestamp callback when motion data is active");

    config.timestamp_callback = timestamp_callback_ptr(new timestamp_events_callback{ this, on_event, user }, [](rs_timestamp_callback* c) { delete c; });
}

void rs_device_base::set_timestamp_callback(rs_timestamp_callback* callback)
{
    // replace previous, if needed
    config.timestamp_callback = timestamp_callback_ptr(callback, [](rs_timestamp_callback* c) { c->release(); });
}

void rs_device_base::start(rs_source source)
{
    if (source == RS_SOURCE_MOTION_TRACKING)
    {
        if (supports(RS_CAPABILITIES_MOTION_EVENTS))
            start_motion_tracking();
        else
            throw std::runtime_error("motion-tracking is not supported by this device");
    }
    else if (source == RS_SOURCE_VIDEO)
    {
        start_video_streaming();
    }
    else if (source == RS_SOURCE_ALL)
    {
        start(RS_SOURCE_MOTION_TRACKING);
        start(RS_SOURCE_VIDEO);
    }
    else
    {
        throw std::runtime_error("unsupported streaming source!");
    }
}

void rs_device_base::stop(rs_source source)
{
    if (source == RS_SOURCE_VIDEO)
    {
        stop_video_streaming();
    }
    else if (source == RS_SOURCE_MOTION_TRACKING)
    {
        if (supports(RS_CAPABILITIES_MOTION_EVENTS))
            stop_motion_tracking();
        else
            throw std::runtime_error("motion-tracking is not supported by this device");
    }
    else if (source == RS_SOURCE_ALL)
    {
        stop(RS_SOURCE_VIDEO);
        stop(RS_SOURCE_MOTION_TRACKING);
    }
    else
    {
        throw std::runtime_error("unsupported streaming source");
    }
}

std::string hexify(unsigned char n)
{
    std::string res;

    do
    {
        res += "0123456789ABCDEF"[n % 16];
        n >>= 4;
    } while (n);

    reverse(res.begin(), res.end());

    if (res.size() == 1)
    {
        res.insert(0, "0");
    }

    return res;
}

void rs_device_base::start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex)
{
    if (keep_fw_logger_alive)
        throw std::logic_error("FW logger already started");

    keep_fw_logger_alive = true;
    fw_logger = std::make_shared<std::thread>([this, fw_log_op_code, grab_rate_in_ms, &mutex]() {
        const int data_size = 500;
        hw_monitor::hwmon_cmd cmd((int)fw_log_op_code);
        cmd.Param1 = data_size;
        while (keep_fw_logger_alive)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(grab_rate_in_ms));
            hw_monitor::perform_and_send_monitor_command(this->get_device(), mutex, cmd);
            char data[data_size];
            memcpy(data, cmd.receivedCommandData, cmd.receivedCommandDataLength);

            std::stringstream sstr;
            sstr << "FW_Log_Data:";
            for (size_t i = 0; i < cmd.receivedCommandDataLength; ++i)
                sstr << hexify(data[i]) << " ";

            if (cmd.receivedCommandDataLength)
               LOG_INFO(sstr.str());
        }
    });
}

void rs_device_base::stop_fw_logger()
{
    if (!keep_fw_logger_alive)
        throw std::logic_error("FW logger not started");

    keep_fw_logger_alive = false;
    fw_logger->join();
}

struct drops_status
{
    bool was_initialized = false;
    unsigned long long prev_frame_counter = 0;
};

void rs_device_base::start_video_streaming()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");

    auto capture_start_time = std::chrono::high_resolution_clock::now();
    auto selected_modes = config.select_modes();
    auto archive = std::make_shared<syncronizing_archive>(selected_modes, select_key_stream(selected_modes), &max_publish_list_size, &event_queue_size, &events_timeout, capture_start_time);

    for(auto & s : native_streams) s->archive.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    auto timestamp_readers = create_frame_timestamp_readers();

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(auto mode_selection : selected_modes)
    {
        assert(static_cast<size_t>(mode_selection.mode.subdevice) <= timestamp_readers.size());
        auto timestamp_reader = timestamp_readers[mode_selection.mode.subdevice];

        // Create a stream buffer for each stream served by this subdevice mode
        for(auto & stream_mode : mode_selection.get_outputs())
        {                    
            // If this is one of the streams requested by the user, store the buffer so they can access it
            if(config.requests[stream_mode.first].enabled) native_streams[stream_mode.first]->archive = archive;
        }

        // Copy the callbacks that apply to this stream, so that they can be captured by value
        std::vector<rs_stream> streams;
        for (auto & output : mode_selection.get_outputs())
        {
            streams.push_back(output.first);
        }     

        auto supported_metadata_vector = std::make_shared<std::vector<rs_frame_metadata>>(config.info.supported_metadata_vector);

        auto actual_fps_calc = std::make_shared<fps_calc>(NUMBER_OF_FRAMES_TO_SAMPLE, mode_selection.get_framerate());
        std::shared_ptr<drops_status> frame_drops_status(new drops_status{});
        // Initialize the subdevice and set it to the selected mode
        set_subdevice_mode(*device, mode_selection.mode.subdevice, mode_selection.mode.native_dims.x, mode_selection.mode.native_dims.y, mode_selection.mode.pf.fourcc, mode_selection.mode.fps, 
            [this, mode_selection, archive, timestamp_reader, streams, capture_start_time, frame_drops_status, actual_fps_calc, supported_metadata_vector](const void * frame, std::function<void()> continuation) mutable
        {
            auto now = std::chrono::system_clock::now();
            auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

            frame_continuation release_and_enqueue(continuation, frame);

            // Ignore any frames which appear corrupted or invalid
            if (!timestamp_reader->validate_frame(mode_selection.mode, frame)) return;
            
            auto actual_fps = actual_fps_calc->calc_fps(now);

            // Determine the timestamp for this frame
            auto timestamp = timestamp_reader->get_frame_timestamp(mode_selection.mode, frame, actual_fps);
            auto frame_counter = timestamp_reader->get_frame_counter(mode_selection.mode, frame);
            auto recieved_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - capture_start_time).count();

            auto requires_processing = mode_selection.requires_processing();

            double exposure_value[1] = {};
            if (streams[0] == rs_stream::RS_STREAM_FISHEYE)
            {
                // fisheye exposure value is embedded in the frame data from version 1.27.2.90
                firmware_version firmware(get_camera_info(RS_CAMERA_INFO_ADAPTER_BOARD_FIRMWARE_VERSION));
                if (firmware >= firmware_version("1.27.2.90"))
                {
                    auto data = static_cast<const char*>(frame);
                    int exposure = 0; // Embedded Fisheye exposure value is in units of 0.2 mSec
                    for (int i = 4, j = 0; i < 12; ++i, ++j)
                        exposure |= ((data[i] & 0x01) << j);

                    exposure_value[0] = exposure * 0.2 * 10.;
                }
            }

            auto width = mode_selection.get_width();
            auto height = mode_selection.get_height();
            auto fps = mode_selection.get_framerate();
            std::vector<byte *> dest;

            auto stride_x = mode_selection.get_stride_x();
            auto stride_y = mode_selection.get_stride_y();
            for (auto & output : mode_selection.get_outputs())
            {
                LOG_DEBUG("FrameAccepted, RecievedAt," << recieved_time << ", FWTS," << timestamp << ", DLLTS," << recieved_time << ", Type," << rsimpl::get_string(output.first) << ",HasPair,0,F#," << frame_counter);
            }
            
            frame_drops_status->was_initialized = true;

            // Not updating prev_frame_counter when first frame arrival
            if (frame_drops_status->was_initialized)
            {
                frames_drops_counter.fetch_add(frame_counter - frame_drops_status->prev_frame_counter - 1);
                frame_drops_status->prev_frame_counter = frame_counter;
            }

            for (auto & output : mode_selection.get_outputs())
            {
                auto bpp = get_image_bpp(output.second);
                frame_archive::frame_additional_data additional_data( timestamp,
                    frame_counter,
                    sys_time,
                    width,
                    height,
                    fps,
                    stride_x,
                    stride_y,
                    bpp,
                    output.second,
                    output.first,
                    mode_selection.pad_crop,
                    supported_metadata_vector,
                    exposure_value[0],
                    actual_fps);

                // Obtain buffers for unpacking the frame
                dest.push_back(archive->alloc_frame(output.first, additional_data, requires_processing));


                if (motion_module_ready) // try to correct timestamp only if motion module is enabled
                {
                    archive->correct_timestamp(output.first);
                }
            }
            // Unpack the frame
            if (requires_processing)
            {
                mode_selection.unpack(dest.data(), reinterpret_cast<const byte *>(frame));
            }

            // If any frame callbacks were specified, dispatch them now
            for (size_t i = 0; i < dest.size(); ++i)
            {
                if (!requires_processing)
                {
                    archive->attach_continuation(streams[i], std::move(release_and_enqueue));
                }

                if (config.callbacks[streams[i]])
                {
                    auto frame_ref = archive->track_frame(streams[i]);
                    if (frame_ref)
                    {
                        frame_ref->update_frame_callback_start_ts(std::chrono::high_resolution_clock::now());
                        frame_ref->log_callback_start(capture_start_time);
                        on_before_callback(streams[i], frame_ref, archive);
                        (*config.callbacks[streams[i]])->on_frame(this, frame_ref);
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

void rs_device_base::stop_video_streaming()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    stop_streaming(*device);
    archive->flush();
    capturing = false;
}

void rs_device_base::wait_all_streams()
{
    if(!capturing) return;
    if(!archive) return;

    archive->wait_for_frames();
}

bool rs_device_base::poll_all_streams()
{
    if(!capturing) return false;
    if(!archive) return false;
    return archive->poll_for_frames();
}

void rs_device_base::release_frame(rs_frame_ref* ref)
{
    archive->release_frame_ref((frame_archive::frame_ref *)ref);
}

rs_frame_ref* ::rs_device_base::clone_frame(rs_frame_ref* frame)
{
    auto result = archive->clone_frame((frame_archive::frame_ref *)frame);
    if (!result) throw std::runtime_error("Not enough resources to clone frame!");
    return result;
}

void rs_device_base::update_device_info(rsimpl::static_device_info& info)
{
    info.options.push_back({ RS_OPTION_FRAMES_QUEUE_SIZE,     1, RS_USER_QUEUE_SIZE,      1, RS_USER_QUEUE_SIZE });
}

const char * rs_device_base::get_option_description(rs_option option) const
{
    switch (option)
    {
    case RS_OPTION_COLOR_BACKLIGHT_COMPENSATION                    : return "Enable / disable color backlight compensation";
    case RS_OPTION_COLOR_BRIGHTNESS                                : return "Color image brightness";
    case RS_OPTION_COLOR_CONTRAST                                  : return "Color image contrast";
    case RS_OPTION_COLOR_EXPOSURE                                  : return "Controls exposure time of color camera. Setting any value will disable auto exposure";
    case RS_OPTION_COLOR_GAIN                                      : return "Color image gain";
    case RS_OPTION_COLOR_GAMMA                                     : return "Color image gamma setting";
    case RS_OPTION_COLOR_HUE                                       : return "Color image hue";
    case RS_OPTION_COLOR_SATURATION                                : return "Color image saturation setting";
    case RS_OPTION_COLOR_SHARPNESS                                 : return "Color image sharpness setting";
    case RS_OPTION_COLOR_WHITE_BALANCE                             : return "Controls white balance of color image. Setting any value will disable auto white balance";
    case RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE                      : return "Enable / disable color image auto-exposure";
    case RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE                 : return "Enable / disable color image auto-white-balance";
    case RS_OPTION_F200_LASER_POWER                                : return "Power of the F200 / SR300 projector, with 0 meaning projector off";
    case RS_OPTION_F200_ACCURACY                                   : return "Set the number of patterns projected per frame. The higher the accuracy value the more patterns projected. Increasing the number of patterns help to achieve better accuracy. Note that this control is affecting the Depth FPS";
    case RS_OPTION_F200_MOTION_RANGE                               : return "Motion vs. Range trade-off, with lower values allowing for better motion sensitivity and higher values allowing for better depth range";
    case RS_OPTION_F200_FILTER_OPTION                              : return "Set the filter to apply to each depth frame. Each one of the filter is optimized per the application requirements";
    case RS_OPTION_F200_CONFIDENCE_THRESHOLD                       : return "The confidence level threshold used by the Depth algorithm pipe to set whether a pixel will get a valid range or will be marked with invalid range";
    case RS_OPTION_F200_DYNAMIC_FPS                                : return "(F200-only) Allows to reduce FPS without restarting streaming. Valid values are {2, 5, 15, 30, 60}";
    case RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE     : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER                   : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE        : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE        : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE      : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_MIN_LASER                      : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_MAX_LASER                      : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_START_LASER                    : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD                : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD                : return "Configures SR300 Depth Auto-Range setting. Should not be used directly but through set IVCAM preset method";
    case RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED                   : return "Enables / disables R200 auto-exposure. This will affect both IR and depth image.";
    case RS_OPTION_R200_LR_GAIN                                    : return "IR image gain";
    case RS_OPTION_R200_LR_EXPOSURE                                : return "This control allows manual adjustment of the exposure time value for the L/R imagers";
    case RS_OPTION_R200_EMITTER_ENABLED                            : return "Enables / disables R200 emitter";
    case RS_OPTION_R200_DEPTH_UNITS                                : return "Micrometers per increment in integer depth values, 1000 is default (mm scale). Set before streaming";
    case RS_OPTION_R200_DEPTH_CLAMP_MIN                            : return "Minimum depth in current depth units that will be output. Any values less than 'Min Depth' will be mapped to 0 during the conversion between disparity and depth. Set before streaming";
    case RS_OPTION_R200_DEPTH_CLAMP_MAX                            : return "Maximum depth in current depth units that will be output. Any values greater than 'Max Depth' will be mapped to 0 during the conversion between disparity and depth. Set before streaming";
    case RS_OPTION_R200_DISPARITY_MULTIPLIER                       : return "The disparity scale factor used when in disparity output mode. Can only be set before streaming";
    case RS_OPTION_R200_DISPARITY_SHIFT                            : return "{0 - 512}. Can only be set before streaming starts";
    case RS_OPTION_R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT     : return "(Requires LR-Auto-Exposure ON) Mean intensity set point";
    case RS_OPTION_R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT       : return "(Requires LR-Auto-Exposure ON) Bright ratio set point";
    case RS_OPTION_R200_AUTO_EXPOSURE_KP_GAIN                      : return "(Requires LR-Auto-Exposure ON) Kp Gain";
    case RS_OPTION_R200_AUTO_EXPOSURE_KP_EXPOSURE                  : return "(Requires LR-Auto-Exposure ON) Kp Exposure";
    case RS_OPTION_R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD            : return "(Requires LR-Auto-Exposure ON) Kp Dark Threshold";
    case RS_OPTION_R200_AUTO_EXPOSURE_TOP_EDGE                     : return "(Requires LR-Auto-Exposure ON) Auto-Exposure region-of-interest top edge (in pixels)";
    case RS_OPTION_R200_AUTO_EXPOSURE_BOTTOM_EDGE                  : return "(Requires LR-Auto-Exposure ON) Auto-Exposure region-of-interest bottom edge (in pixels)";
    case RS_OPTION_R200_AUTO_EXPOSURE_LEFT_EDGE                    : return "(Requires LR-Auto-Exposure ON) Auto-Exposure region-of-interest left edge (in pixels)";
    case RS_OPTION_R200_AUTO_EXPOSURE_RIGHT_EDGE                   : return "(Requires LR-Auto-Exposure ON) Auto-Exposure region-of-interest right edge (in pixels)";
    case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT    : return "Value to subtract when estimating the median of the correlation surface";
    case RS_OPTION_R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT    : return "Value to add when estimating the median of the correlation surface";
    case RS_OPTION_R200_DEPTH_CONTROL_MEDIAN_THRESHOLD             : return "A threshold by how much the winning score must beat the median";
    case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD      : return "The minimum correlation score that is considered acceptable";
    case RS_OPTION_R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD      : return "The maximum correlation score that is considered acceptable";
    case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD      : return "A parameter for determining whether the texture in the region is sufficient to justify a depth result";
    case RS_OPTION_R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD : return "A parameter for determining whether the texture in the region is sufficient to justify a depth result";
    case RS_OPTION_R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD        : return "A threshold on how much the minimum correlation score must differ from the next best score";
    case RS_OPTION_R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD           : return "Neighbor threshold value for depth calculation";
    case RS_OPTION_R200_DEPTH_CONTROL_LR_THRESHOLD                 : return "Left-Right threshold value for depth calculation";
    case RS_OPTION_FISHEYE_EXPOSURE                                : return "Fisheye image exposure time in msec";
    case RS_OPTION_FISHEYE_GAIN                                    : return "Fisheye image gain";
    case RS_OPTION_FISHEYE_STROBE                                  : return "Enables / disables fisheye strobe. When enabled this will align timestamps to common clock-domain with the motion events";
    case RS_OPTION_FISHEYE_EXTERNAL_TRIGGER                        : return "Enables / disables fisheye external trigger mode. When enabled fisheye image will be acquired in-sync with the depth image";
    case RS_OPTION_FRAMES_QUEUE_SIZE                               : return "Number of frames the user is allowed to keep per stream. Trying to hold-on to more frames will cause frame-drops.";
    case RS_OPTION_FISHEYE_ENABLE_AUTO_EXPOSURE                    : return "Enable / disable fisheye auto-exposure";
    case RS_OPTION_FISHEYE_AUTO_EXPOSURE_MODE                      : return "0 - static auto-exposure, 1 - anti-flicker auto-exposure, 2 - hybrid";
    case RS_OPTION_FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE          : return "Fisheye auto-exposure anti-flicker rate, can be 50 or 60 Hz";
    case RS_OPTION_FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE         : return "In Fisheye auto-exposure sample frame every given number of pixels";
    case RS_OPTION_FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES               : return "In Fisheye auto-exposure sample every given number of frames";
    case RS_OPTION_HARDWARE_LOGGER_ENABLED                         : return "Enables / disables fetching diagnostic information from hardware (and writting the results to log)";
    case RS_OPTION_TOTAL_FRAME_DROPS                               : return "Total number of detected frame drops from all streams";
    default: return rs_option_to_string(option);
    }
}

bool rs_device_base::supports(rs_capabilities capability) const
{
    auto found = false;
    auto version_ok = true;
    for (auto supported : config.info.capabilities_vector)
    {
        if (supported.capability == capability)
        {
            found = true;

            firmware_version firmware(get_camera_info(supported.firmware_type));
            if (!firmware.is_between(supported.from, supported.until)) // unsupported due to versioning constraint
            {
                LOG_WARNING("capability " << rs_capabilities_to_string(capability) << " requires " << rs_camera_info_to_string(supported.firmware_type) << " to be from " << supported.from << " up-to " << supported.until << ", but is " << firmware << "!");
                version_ok = false;
            }
        }
    }
    return found && version_ok;
}

bool rs_device_base::supports(rs_camera_info info_param) const
{
    auto it = config.info.camera_info.find(info_param);
    return (it != config.info.camera_info.end());
}

void rs_device_base::get_option_range(rs_option option, double & min, double & max, double & step, double & def)
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

void rs_device_base::set_options(const rs_option options[], size_t count, const double values[])
{
    for (size_t i = 0; i < count; ++i)
    {
        switch (options[i])
        {
        case  RS_OPTION_FRAMES_QUEUE_SIZE:
            max_publish_list_size = (uint32_t)values[i];
            break;
        case RS_OPTION_TOTAL_FRAME_DROPS:
            frames_drops_counter = (uint32_t)values[i];
            break;
        default:
            LOG_WARNING("Cannot set " << options[i] << " to " << values[i] << " on " << get_name());
            throw std::logic_error("Option unsupported");
            break;
        }
    }
}

void rs_device_base::get_options(const rs_option options[], size_t count, double values[])
{
    for (size_t i = 0; i < count; ++i)
    {
        switch (options[i])
        {
        case  RS_OPTION_FRAMES_QUEUE_SIZE:
            values[i] = max_publish_list_size;
            break;
        case  RS_OPTION_TOTAL_FRAME_DROPS:
            values[i] = frames_drops_counter;
            break;
        default:
            LOG_WARNING("Cannot get " << options[i] << " on " << get_name());
            throw std::logic_error("Option unsupported");
            break;
        }
    }
}

void rs_device_base::disable_auto_option(int subdevice, rs_option auto_opt)
{
    static const int reset_state = 0;
    // Probe , then deactivate
    if (uvc::get_pu_control(get_device(), subdevice, auto_opt))
        uvc::set_pu_control(get_device(), subdevice, auto_opt, reset_state);
}


const char * rs_device_base::get_usb_port_id() const
{
    std::lock_guard<std::mutex> lock(usb_port_mutex);
    if (usb_port_id == "") usb_port_id = rsimpl::uvc::get_usb_port_id(*device);
    return usb_port_id.c_str();
}
