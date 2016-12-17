// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "types.h"
#include "image.h"
#include "device.h"

#include <cstring>
#include <algorithm>
#include <array>
#include <deque>
#include <algorithm>
#include <iomanip>

#define unknown "UNKNOWN" 

namespace rsimpl
{
    const char * get_string(rs_stream value)
    {
        #define CASE(X) case RS_STREAM_##X: return #X;
        switch(value)
        {
        CASE(DEPTH)
        CASE(COLOR)
        CASE(INFRARED)
        CASE(INFRARED2)
        CASE(POINTS)
        CASE(RECTIFIED_COLOR)
        CASE(COLOR_ALIGNED_TO_DEPTH)
        CASE(DEPTH_ALIGNED_TO_COLOR)
        CASE(DEPTH_ALIGNED_TO_RECTIFIED_COLOR)
        CASE(INFRARED2_ALIGNED_TO_DEPTH)
        CASE(DEPTH_ALIGNED_TO_INFRARED2)
        CASE(FISHEYE)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_format value)
    {
        #define CASE(X) case RS_FORMAT_##X: return #X;
        switch(value)
        {
        CASE(ANY)
        CASE(Z16)
        CASE(DISPARITY16)
        CASE(XYZ32F)
        CASE(YUYV)
        CASE(RGB8)
        CASE(BGR8)
        CASE(RGBA8)
        CASE(BGRA8)
        CASE(Y8)
        CASE(Y16)
        CASE(RAW10)
        CASE(RAW16)
        CASE(RAW8)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_preset value)
    {
        #define CASE(X) case RS_PRESET_##X: return #X;
        switch(value)
        {
        CASE(BEST_QUALITY)
        CASE(LARGEST_IMAGE)
        CASE(HIGHEST_FRAMERATE)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_distortion value)
    {
        #define CASE(X) case RS_DISTORTION_##X: return #X;
        switch(value)
        {
        CASE(NONE)
        CASE(MODIFIED_BROWN_CONRADY)
        CASE(INVERSE_BROWN_CONRADY)
        CASE(FTHETA)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_option value)
    {
        #define CASE(X) case RS_OPTION_##X: return #X;
        switch(value)
        {
        CASE(COLOR_BACKLIGHT_COMPENSATION)
        CASE(COLOR_BRIGHTNESS)
        CASE(COLOR_CONTRAST)
        CASE(COLOR_EXPOSURE)
        CASE(COLOR_GAIN)
        CASE(COLOR_GAMMA)
        CASE(COLOR_HUE)
        CASE(COLOR_SATURATION)
        CASE(COLOR_SHARPNESS)
        CASE(COLOR_WHITE_BALANCE)
        CASE(COLOR_ENABLE_AUTO_EXPOSURE)
        CASE(COLOR_ENABLE_AUTO_WHITE_BALANCE)
        CASE(F200_LASER_POWER)
        CASE(F200_ACCURACY)
        CASE(F200_MOTION_RANGE)
        CASE(F200_FILTER_OPTION)
        CASE(F200_CONFIDENCE_THRESHOLD)
        CASE(F200_DYNAMIC_FPS)
        CASE(SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE) 
        CASE(SR300_AUTO_RANGE_ENABLE_LASER)               
        CASE(SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE)    
        CASE(SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE)    
        CASE(SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE)  
        CASE(SR300_AUTO_RANGE_MIN_LASER)                  
        CASE(SR300_AUTO_RANGE_MAX_LASER)                  
        CASE(SR300_AUTO_RANGE_START_LASER)                
        CASE(SR300_AUTO_RANGE_UPPER_THRESHOLD) 
        CASE(SR300_AUTO_RANGE_LOWER_THRESHOLD)
        CASE(R200_LR_AUTO_EXPOSURE_ENABLED)
        CASE(R200_LR_GAIN)
        CASE(R200_LR_EXPOSURE)
        CASE(R200_EMITTER_ENABLED)
        CASE(R200_DEPTH_UNITS)
        CASE(R200_DEPTH_CLAMP_MIN)
        CASE(R200_DEPTH_CLAMP_MAX)
        CASE(R200_DISPARITY_MULTIPLIER)
        CASE(R200_DISPARITY_SHIFT)
        CASE(R200_AUTO_EXPOSURE_MEAN_INTENSITY_SET_POINT)
        CASE(R200_AUTO_EXPOSURE_BRIGHT_RATIO_SET_POINT)  
        CASE(R200_AUTO_EXPOSURE_KP_GAIN)                 
        CASE(R200_AUTO_EXPOSURE_KP_EXPOSURE)             
        CASE(R200_AUTO_EXPOSURE_KP_DARK_THRESHOLD)       
        CASE(R200_AUTO_EXPOSURE_TOP_EDGE)       
        CASE(R200_AUTO_EXPOSURE_BOTTOM_EDGE)    
        CASE(R200_AUTO_EXPOSURE_LEFT_EDGE)      
        CASE(R200_AUTO_EXPOSURE_RIGHT_EDGE)     
        CASE(R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_DECREMENT)   
        CASE(R200_DEPTH_CONTROL_ESTIMATE_MEDIAN_INCREMENT)   
        CASE(R200_DEPTH_CONTROL_MEDIAN_THRESHOLD)            
        CASE(R200_DEPTH_CONTROL_SCORE_MINIMUM_THRESHOLD)     
        CASE(R200_DEPTH_CONTROL_SCORE_MAXIMUM_THRESHOLD)     
        CASE(R200_DEPTH_CONTROL_TEXTURE_COUNT_THRESHOLD)     
        CASE(R200_DEPTH_CONTROL_TEXTURE_DIFFERENCE_THRESHOLD)
        CASE(R200_DEPTH_CONTROL_SECOND_PEAK_THRESHOLD)       
        CASE(R200_DEPTH_CONTROL_NEIGHBOR_THRESHOLD)
        CASE(R200_DEPTH_CONTROL_LR_THRESHOLD)
        CASE(FISHEYE_EXPOSURE)
        CASE(FISHEYE_GAIN)
        CASE(FISHEYE_STROBE)
        CASE(FISHEYE_EXTERNAL_TRIGGER)
        CASE(FRAMES_QUEUE_SIZE)
        CASE(TOTAL_FRAME_DROPS)
        CASE(FISHEYE_ENABLE_AUTO_EXPOSURE)
        CASE(FISHEYE_AUTO_EXPOSURE_MODE)
        CASE(FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE)
        CASE(FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE)
        CASE(FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES)
        CASE(HARDWARE_LOGGER_ENABLED)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_source value)
    {
        #define CASE(X) case RS_SOURCE_##X: return #X;
        switch(value)
        {
        CASE(VIDEO)
        CASE(MOTION_TRACKING)
        CASE(ALL)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }
    
    const char * get_string(rs_capabilities value)
    {
        #define CASE(X) case RS_CAPABILITIES_##X: return #X;
        switch(value)
        {
        CASE(DEPTH)
        CASE(COLOR)
        CASE(INFRARED)
        CASE(INFRARED2)
        CASE(FISH_EYE)
        CASE(MOTION_EVENTS)
        CASE(MOTION_MODULE_FW_UPDATE)
        CASE(ADAPTER_BOARD)
        CASE(ENUMERATION)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_event_source value)
    {
        #define CASE(X) case RS_EVENT_##X: return #X;
        switch(value)
        {
        CASE(IMU_ACCEL)
        CASE(IMU_GYRO)
        CASE(IMU_DEPTH_CAM)
        CASE(IMU_MOTION_CAM)
        CASE(G0_SYNC)
        CASE(G1_SYNC)
        CASE(G2_SYNC)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_blob_type value)
    {
        #define CASE(X) case RS_BLOB_TYPE_##X: return #X;
        switch(value)
        {
        CASE(MOTION_MODULE_FIRMWARE_UPDATE)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_camera_info value)
    {
        #define CASE(X) case RS_CAMERA_INFO_##X: return #X;
        switch(value)
        {
        CASE(DEVICE_NAME)
        CASE(DEVICE_SERIAL_NUMBER)
        CASE(CAMERA_FIRMWARE_VERSION)
        CASE(ADAPTER_BOARD_FIRMWARE_VERSION)
        CASE(MOTION_MODULE_FIRMWARE_VERSION)
        CASE(IMAGER_MODEL_NUMBER)
        CASE(CAMERA_TYPE)
        CASE(OEM_ID)
        CASE(MODULE_VERSION)
        CASE(BUILD_DATE)
        CASE(CALIBRATION_DATE)
        CASE(PROGRAM_DATE)
        CASE(FOCUS_ALIGNMENT_DATE)
        CASE(FOCUS_VALUE)
        CASE(CONTENT_VERSION)
        CASE(ISP_FW_VERSION)
        CASE(LENS_TYPE)
        CASE(LENS_COATING__TYPE)
        CASE(NOMINAL_BASELINE)
        CASE(3RD_LENS_TYPE)
        CASE(3RD_LENS_COATING_TYPE)
        CASE(3RD_NOMINAL_BASELINE)
        CASE(EMITTER_TYPE)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_frame_metadata value)
    {
        #define CASE(X) case RS_FRAME_METADATA_##X: return #X;
        switch (value)
        {
        CASE(ACTUAL_EXPOSURE)
        CASE(ACTUAL_FPS)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    const char * get_string(rs_timestamp_domain value)
    {
        #define CASE(X) case RS_TIMESTAMP_DOMAIN_##X: return #X;
        switch (value)
        {
        CASE(CAMERA)
        CASE(MICROCONTROLLER)
        default: assert(!is_valid(value)); return unknown;
        }
        #undef CASE
    }

    size_t subdevice_mode_selection::get_image_size(rs_stream stream) const
    {
        return rsimpl::get_image_size(get_width(), get_height(), get_format(stream));
    }

    void subdevice_mode_selection::set_output_buffer_format(const rs_output_buffer_format in_output_format)
    {
        output_format = in_output_format;
    }

    void subdevice_mode_selection::unpack(byte * const dest[], const byte * source) const
    {
        const int MAX_OUTPUTS = 2;
        const auto & outputs = get_outputs();        
        assert(outputs.size() <= MAX_OUTPUTS);

        // Determine input stride (and apply cropping)
        const byte * in = source;
        size_t in_stride = mode.pf.get_image_size(mode.native_dims.x, 1);
        if(pad_crop < 0) in += in_stride * -pad_crop + mode.pf.get_image_size(-pad_crop, 1);

        // Determine output stride (and apply padding)
        byte * out[MAX_OUTPUTS];
        size_t out_stride[MAX_OUTPUTS] = { 0 };
        for(size_t i=0; i<outputs.size(); ++i)
        {
            out[i] = dest[i];
            out_stride[i] = rsimpl::get_image_size(get_width(), 1, outputs[i].second);
            if(pad_crop > 0) out[i] += out_stride[i] * pad_crop + rsimpl::get_image_size(pad_crop, 1, outputs[i].second);
        }

        // Unpack (potentially a subrect of) the source image into (potentially a subrect of) the destination buffers
        const int unpack_width = get_unpacked_width(), unpack_height = get_unpacked_height();
        if(mode.native_dims.x == get_width())
        {
            // If not strided, unpack as though it were a single long row
            mode.pf.unpackers[unpacker_index].unpack(out, in, unpack_width * unpack_height);
        }
        else
        {
            
            // Otherwise unpack one row at a time
            assert(mode.pf.plane_count == 1); // Can't unpack planar formats row-by-row (at least not with the current architecture, would need to pass multiple source ptrs to unpack)
            for(int i=0; i<unpack_height; ++i)
            {
                mode.pf.unpackers[unpacker_index].unpack(out, in, unpack_width);
                for(size_t i=0; i<outputs.size(); ++i) out[i] += out_stride[i];
                in += in_stride;
            }
        }
    }

    int subdevice_mode_selection::get_unpacked_width() const
    {
        return std::min(mode.native_intrinsics.width, get_width());
    }

    int subdevice_mode_selection::get_unpacked_height() const
    {
        return std::min(mode.native_intrinsics.height, get_height());
    }

    ////////////////////////
    // static_device_info //
    ////////////////////////

    bool stream_request::contradict(stream_request req) const
    {
        if (((format != RS_FORMAT_ANY && format != req.format) ||
            (width != 0 && width != req.width) ||
            (height != 0 && height != req.height) ||
            (fps != 0 && fps != req.fps) ||
            (output_format != req.output_format)))
            return true;
        return false;
    }

    bool stream_request::is_filled() const
    {
        return width != 0 && height != 0 && format != RS_FORMAT_ANY && fps != 0;
    }

    static_device_info::static_device_info() : num_libuvc_transfer_buffers(1), nominal_depth_scale(0.001f)
    {
        for(auto & s : stream_subdevices) s = -1;
        for(auto & s : data_subdevices) s = -1;
        for(auto & s : presets) for(auto & p : s) p = stream_request();
        for(auto & p : stream_poses)
        {
            p = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        }
    }

    // search_request_params are used to find first request that satisfies cameras set of constraints
    // each search_request_params represents requests for each stream type + index of current stream type under examination
    struct search_request_params
    {
        stream_request requests[RS_STREAM_NATIVE_COUNT];
        int stream;
        search_request_params(stream_request in_requests[RS_STREAM_NATIVE_COUNT], int i)
            : stream(i)
        {
            for (auto i = 0; i<RS_STREAM_NATIVE_COUNT; i++)
            {
                requests[i] = in_requests[i];
            }
        }
    };

    bool device_config::all_requests_filled(const stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const
    {
        for (auto i = 0; i<RS_STREAM_NATIVE_COUNT; i++)
        {
            if (requests[i].enabled &&
                (requests[i].height == 0 ||
                requests[i].width == 0 ||
                requests[i].format == RS_FORMAT_ANY ||
                requests[i].fps == 0))
                return false;
        }
        return true;
    }

    // find_good_requests_combination is used to find requests that satisfy cameras set of constraints.
    // this is done using BFS search over the posibility space.
    // the algorithm:
    // start with initial combination of streams requests- the input requests, can be empty or partially filled by user
    // insert initial combination to a queue data structure (dequeu for performance)
    // loop until queue is empty - at each iteration pop from queue the next set of requests.
    // for each one of the next stream request posibilties create new items by adding them to current item and pushing them back to queue.
    // once there is a item that all its stream requsts are filled 
    // and validated to satisfies all interstream constraints
    // copy it to requests parameter and return true.
    bool device_config::find_good_requests_combination( stream_request(&requests)[RS_STREAM_NATIVE_COUNT], std::vector<stream_request> stream_requests[RS_STREAM_NATIVE_COUNT]) const
    {
        std::deque<search_request_params> calls;
  
        // initial parameter is the input requests 
        // and its stream index is 0 (depth)
        search_request_params p = { requests, 0 };
        calls.push_back(p);

        while (!calls.empty())
        {
            //pop one item
            p = calls.front();
            calls.pop_front();

            //check if found combination that satisfies all interstream constraints
            if (all_requests_filled(p.requests) && validate_requests(p.requests))
            {
                for (auto i = 0; i < RS_STREAM_NATIVE_COUNT; i++)
                {
                    requests[i] = p.requests[i];
                }
                return true;
            }

            //if this stream is not enabled or already filled move to next item 
            if (!requests[p.stream].enabled || requests[p.stream].is_filled()) 
            {
                // push the new requests parameter with stream =  stream + 1
                search_request_params new_p = { p.requests, p.stream + 1 };
                calls.push_back(new_p);
                continue;
            }

            //now need to go over all posibilities for the next stream
            for (size_t i = 0; i < stream_requests[p.stream].size(); i++)
            {

                //check that this spasific request is not contradicts the original user request
                if (!requests[p.stream].contradict(stream_requests[p.stream][i]))
                {
                    //add to request the next option from possible requests
                    p.requests[p.stream] = stream_requests[p.stream][i];

                    //if after adding the next stream request if it doesn't satisfies all interstream constraints
                    //do not insert it to queue
                    if (validate_requests(p.requests))
                    { 
                        // push the new requests parameter with stream =  stream + 1
                        search_request_params new_p = { p.requests, p.stream + 1 };
                        calls.push_back(new_p);
                    }
                }
            }

        }
        //if deque is empty and no good requests combination found return false
        return false;
    }

    bool device_config::fill_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT]) const
    {
        //did the user filled all requests?
        if(all_requests_filled(requests))
        {
            return true;
        }

        //If the user did not fill all requests, we need to fill the missing requests

        std::vector<stream_request> stream_requests[RS_STREAM_NATIVE_COUNT];
        //Get all requests posibilities in order to find the requests that satisfies interstream constraints
        get_all_possible_requestes(stream_requests);

        //find stream requests combination that satisfies all interstream constraints
        return find_good_requests_combination(requests, stream_requests);
    }

    void device_config::get_all_possible_requestes(std::vector<stream_request>(&stream_requests)[RS_STREAM_NATIVE_COUNT]) const
    {
        for (size_t i = 0; i < info.subdevice_modes.size(); i++)
        {
            stream_request request;
            auto mode = info.subdevice_modes[i];

            for (auto pad_crop : mode.pad_crop_options)
            {
                for (auto & unpacker : mode.pf.unpackers)
                {
                    auto selection = subdevice_mode_selection(mode, pad_crop, int(&unpacker - mode.pf.unpackers.data()));

                    request.enabled = true;
                    request.fps = selection.get_framerate();
                    request.height = selection.get_height();
                    request.width = selection.get_width();
                    auto outputs = selection.get_outputs();

                    for (auto output : outputs)
                    {
                        request.format = output.second;
                        for (auto output_format = static_cast<int>(RS_OUTPUT_BUFFER_FORMAT_CONTINUOUS); output_format < static_cast<int>(RS_OUTPUT_BUFFER_FORMAT_COUNT); output_format++)
                        {
                            request.output_format = static_cast<rs_output_buffer_format>(output_format);
                            stream_requests[output.first].push_back(request);
                        }
                    }
                }
            }
        }
    }

    subdevice_mode_selection device_config::select_mode(const  stream_request(&requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        auto any_stream_requested = false;
        std::array<bool, RS_STREAM_NATIVE_COUNT> stream_requested = {};
        for(int j = 0; j < RS_STREAM_NATIVE_COUNT; ++j)
        {
            if(requests[j].enabled && info.stream_subdevices[j] == subdevice_index)
            {
                stream_requested[j] = true;
                any_stream_requested = true;
            }
        }

        // If no streams were requested, skip to the next subdevice
        if(!any_stream_requested) return subdevice_mode_selection();

        // Look for an appropriate mode
        for(auto & subdevice_mode : info.subdevice_modes)
        {
            // Skip modes that apply to other subdevices
            if(subdevice_mode.subdevice != subdevice_index) continue;

           
            for(auto pad_crop : subdevice_mode.pad_crop_options)
            {
                for(auto & unpacker : subdevice_mode.pf.unpackers)
                {
                    auto selection = subdevice_mode_selection(subdevice_mode, pad_crop, (int)(&unpacker - subdevice_mode.pf.unpackers.data()));

                    // Determine if this mode satisfies the requirements on our requested streams
                    auto stream_unsatisfied = stream_requested;
                    for(auto & output : unpacker.outputs)
                    {
                        const auto & req = requests[output.first];
                        
                        selection.set_output_buffer_format(req.output_format);
                        if(req.enabled && (req.width == selection.get_width() )
                                       && (req.height == selection.get_height())
                                       && (req.format == selection.get_format(output.first))
                                       && (req.fps == subdevice_mode.fps))
                        {
                            stream_unsatisfied[output.first] = false;
                        }
                    }

                    // If any requested streams are still unsatisfied, skip to the next mode
                    if(std::any_of(begin(stream_unsatisfied), end(stream_unsatisfied), [](bool b) { return b; })) continue;
                    return selection;
                }
            }
        }

        // If we did not find an appropriate mode, report an error
        std::ostringstream ss;
        ss << "uvc subdevice " << subdevice_index << " cannot provide";
        bool first = true;
        for(int j = 0; j < RS_STREAM_NATIVE_COUNT; ++j)
        {
            if(!stream_requested[j]) continue;
            ss << (first ? " " : " and ");
            ss << requests[j].width << 'x' << requests[j].height << ':' << get_string(requests[j].format);
            ss << '@' << requests[j].fps << "Hz " << get_string((rs_stream)j);
            first = false;
        }
        throw std::runtime_error(ss.str());
    }

    std::vector<subdevice_mode_selection> device_config::select_modes(const stream_request (&reqs)[RS_STREAM_NATIVE_COUNT]) const
    {
        // Make a mutable copy of our array
        stream_request requests[RS_STREAM_NATIVE_COUNT];
        for (int i = 0; i<RS_STREAM_NATIVE_COUNT; ++i) requests[i] = reqs[i];

        //Validate that user requests satisfy all interstream constraints 
        validate_requests(requests, true);

        //Fill the requests that user did not fill
        fill_requests(requests);

        // Select subdevice modes needed to satisfy our requests
        int num_subdevices = 0;
        for(auto & mode : info.subdevice_modes) num_subdevices = std::max(num_subdevices, mode.subdevice+1);
        std::vector<subdevice_mode_selection> selected_modes;
        for(int i = 0; i < num_subdevices; ++i)
        {
            auto selection = select_mode(requests, i);
            if(selection.mode.pf.fourcc) selected_modes.push_back(selection);
        }
        return selected_modes;
    }

    bool device_config::validate_requests(stream_request(&requests)[RS_STREAM_NATIVE_COUNT], bool throw_exception) const
    {
        // Check and modify requests to enforce all interstream constraints

        for (auto & rule : info.interstream_rules)
        {
            auto & a = requests[rule.a], &b = requests[rule.b]; auto f = rule.field;
            if (a.enabled && b.enabled)
            {
                bool compat = true;
                std::stringstream error_message;

                if (rule.same_format)
                {
                    if ((a.format != RS_FORMAT_ANY) && (b.format != RS_FORMAT_ANY) && (a.format != b.format))
                    {
                        if (throw_exception) error_message << rule.a << " format (" << rs_format_to_string(a.format) << ") must be equal to " << rule.b << " format (" << rs_format_to_string(b.format) << ")!";
                        compat = false;
                    }
                }
                else if((a.*f != 0) && (b.*f != 0))
                {
                    if ((rule.bigger == RS_STREAM_COUNT) && (!rule.divides && !rule.divides2))
                    {
                        // Check for incompatibility if both values specified
                        if ((a.*f + rule.delta != b.*f) && (a.*f + rule.delta2 != b.*f))
                        {
                            if (throw_exception) error_message << " " << rule.b << " value " << b.*f << " must be equal to either " << (a.*f + rule.delta) << " or " << (a.*f + rule.delta2) << "!";
                            compat = false;
                        }
                    }
                    else
                    {
                        if (((rule.bigger == rule.a) && (a.*f < b.*f)) || ((rule.bigger == rule.b) && (b.*f < a.*f)))
                        {
                            if (throw_exception) error_message << " " << rule.a << " value " << a.*f << " must be " << ((rule.bigger == rule.a) ? "bigger" : "smaller") << " then " << rule.b << " value " << b.*f << "!";
                            compat = false;
                        }
                        if ((rule.divides &&  (a.*f % b.*f)) || (rule.divides2 && (b.*f % a.*f)))
                        {
                            if (throw_exception) error_message << " " << rule.a << " value " << a.*f << " must " << (rule.divides ? "be divided by" : "divide") << rule.b << " value " << b.*f << "!";
                            compat = false;
                        }
                    }
                }
                if (!compat)
                {
                    if (throw_exception)
                        throw std::runtime_error(to_string() << "requested settings for " << rule.a << " and " << rule.b << " are incompatible!" << error_message.str());
                    return false;
                }
            }
        }
        return true;
    }

    std::string firmware_version::to_string() const
    {
        if (is_any) return "any";

        std::stringstream s;
        s << std::setfill('0') << std::setw(2) << m_major << "." 
            << std::setfill('0') << std::setw(2) << m_minor << "." 
            << std::setfill('0') << std::setw(2) << m_patch << "." 
            << std::setfill('0') << std::setw(2) << m_build;
        return s.str();
    }

    std::vector<std::string> firmware_version::split(const std::string& str)
    {
        std::vector<std::string> result;
        auto e = str.end();
        auto i = str.begin();
        while (i != e){
            i = find_if_not(i, e, [](char c) { return c == '.'; });
            if (i == e) break;
            auto j = find(i, e, '.');
            result.emplace_back(i, j);
            i = j;
        }
        return result;
    }

    int firmware_version::parse_part(const std::string& name, int part)
    {
        return atoi(split(name)[part].c_str());
    }

    calibration_validator::calibration_validator(std::function<bool(rs_stream, rs_stream)> extrinsic_validator, std::function<bool(rs_stream)> intrinsic_validator)
        : extrinsic_validator(extrinsic_validator), intrinsic_validator(intrinsic_validator)
    {
    }

    calibration_validator::calibration_validator()
        : extrinsic_validator([](rs_stream, rs_stream) { return true; }), intrinsic_validator([](rs_stream) { return true; })
    {
    }

    bool calibration_validator::validate_extrinsics(rs_stream from_stream, rs_stream to_stream) const
    {
        return extrinsic_validator(from_stream, to_stream);
    }
    bool calibration_validator::validate_intrinsics(rs_stream stream) const
    {
        return intrinsic_validator(stream);
    }

}
