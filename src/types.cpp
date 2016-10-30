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
#include <numeric>

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
        CASE(BROWN_CONRADY)
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
        CASE(FISHEYE_ENABLE_AUTO_EXPOSURE)
        CASE(FISHEYE_AUTO_EXPOSURE_MODE)
        CASE(FISHEYE_AUTO_EXPOSURE_ANTIFLICKER_RATE)
        CASE(FISHEYE_AUTO_EXPOSURE_PIXEL_SAMPLE_RATE)
        CASE(FISHEYE_AUTO_EXPOSURE_SKIP_FRAMES)
        CASE(FRAMES_QUEUE_SIZE)
        CASE(HARDWARE_LOGGER_ENABLED)
        CASE(TOTAL_FRAME_DROPS)
        CASE(RS4XX_PROJECTOR_MODE)
        CASE(RS4XX_PROJECTOR_PWR)
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
                for(size_t j=0; j<outputs.size(); ++j) out[j] += out_stride[j];
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

    // find_valid_combination is used to find the set of supported profiles that satisfies the cameras set of constraints.
    // this is done using BFS search over the posibility space.
    // the algorithm:
    // start with initial combination of streams requests- the input requests, can be empty or partially filled by user
    // insert initial combination to a queue data structure (dequeu for performance)
    // loop until queue is empty - at each iteration pop from queue the next set of requests.
    // for each one of the next stream request posibilties create new items by adding them to current item and pushing them back to queue.
    // once there is a item that all its stream requsts are filled 
    // and validated to satisfies all interstream constraints
    // copy it to requests parameter and return true.
    bool device_config::find_valid_combination( stream_request(&requests)[RS_STREAM_NATIVE_COUNT], std::vector<stream_request> stream_requests[RS_STREAM_NATIVE_COUNT]) const
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

            //check if the found combination satisfies all interstream constraints
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
                //check that the specific request doen't contradict with the original user request
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
        //if deque is empty and no matching combination found, then  return false
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

        //find a stream profiles combination that satisfies all interstream constraints
        return find_valid_combination(requests, stream_requests);
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

    /// Convert orientation angles stored in rodrigues conventions to rotation matrix
    /// for details: http://mesh.brown.edu/en193s08-2003/notes/en193s08-rots.pdf
    float3x3 calc_rodrigues_matrix(const std::vector<double> rot)
    {
        float3x3 rot_mat{};

        double theta = sqrt(std::inner_product(rot.begin(), rot.end(), rot.begin(), 0.0));
        double r1 = rot[0], r2 = rot[1], r3 = rot[2];
        if (theta <= sqrt(DBL_EPSILON)) // identityMatrix
        {
            rot_mat(0,0) = rot_mat(1, 1) = rot_mat(2, 2) = 1.0;
            rot_mat(0,1) = rot_mat(0, 2) = rot_mat(1, 0) = rot_mat(1, 2) = rot_mat(2, 0) = rot_mat(2, 1) = 0.0;
        }
        else
        {
            r1 /= theta;
            r2 /= theta;
            r3 /= theta;

            double c = cos(theta);
            double s = sin(theta);
            double g = 1 - c;

            rot_mat(0, 0) = float(c + g * r1 * r1);
            rot_mat(0, 1) = float(g * r1 * r2 - s * r3);
            rot_mat(0, 2) = float(g * r1 * r3 + s * r2);
            rot_mat(1, 0) = float(g * r2 * r1 + s * r3);
            rot_mat(1, 1) = float(c + g * r2 * r2);
            rot_mat(1, 2) = float(g * r2 * r3 - s * r1);
            rot_mat(2, 0) = float(g * r3 * r1 - s * r2);
            rot_mat(2, 1) = float(g * r3 * r2 + s * r1);
            rot_mat(2, 2) = float(c + g * r3 * r3);
        }

        return rot_mat;
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


#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

    static const uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    /// Calculate CRC code for arbitrary characters buffer
    uint32_t calc_crc32(uint8_t *buf, size_t bufsize)
    {
        uint32_t oldcrc32 = 0xFFFFFFFF;
        for (; bufsize; --bufsize, ++buf)
            oldcrc32 = UPDC32(*buf, oldcrc32);
        return ~oldcrc32;
    }

}
