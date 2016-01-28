// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "types.h"
#include "image.h"
#include "device.h"

#include <cstring>
#include <algorithm>
#include <array>

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
        default: assert(!is_valid(value)); return nullptr;
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
        default: assert(!is_valid(value)); return nullptr;
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
        default: assert(!is_valid(value)); return nullptr;
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
        default: assert(!is_valid(value)); return nullptr;
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
        CASE(SR300_DYNAMIC_FPS)
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
        default: assert(!is_valid(value)); return nullptr;
        }
        #undef CASE
    }

    size_t subdevice_mode_selection::get_image_size(rs_stream stream) const
    {
        return rsimpl::get_image_size(get_width(), get_height(), get_format(stream));
    }

    void subdevice_mode_selection::unpack(byte * const dest[], const byte * source) const
    {
        const int MAX_OUTPUTS = 2;
        const auto & outputs = get_outputs();        
        assert(outputs.size() <= MAX_OUTPUTS);

        // Determine input stride (and apply cropping)
        const byte * in = source;
        size_t in_stride = mode->pf->get_image_size(mode->native_dims.x, 1);
        if(pad_crop < 0) in += in_stride * -pad_crop + mode->pf->get_image_size(-pad_crop, 1);

        // Determine output stride (and apply padding)
        byte * out[MAX_OUTPUTS];
        size_t out_stride[MAX_OUTPUTS];
        for(size_t i=0; i<outputs.size(); ++i)
        {
            out[i] = dest[i];
            out_stride[i] = rsimpl::get_image_size(get_width(), 1, outputs[i].second);
            if(pad_crop > 0) out[i] += out_stride[i] * pad_crop + rsimpl::get_image_size(pad_crop, 1, outputs[i].second);
        }

        // Unpack (potentially a subrect of) the source image into (potentially a subrect of) the destination buffers
        const int unpack_width = std::min(mode->native_intrinsics.width, get_width()), unpack_height = std::min(mode->native_intrinsics.height, get_height());
        if(mode->native_dims.x == get_width())
        {
            // If not strided, unpack as though it were a single long row
            unpacker->unpack(out, in, unpack_width * unpack_height);
        }
        else
        {
            // Otherwise unpack one row at a time
            assert(mode->pf->plane_count == 1); // Can't unpack planar formats row-by-row (at least not with the current architecture, would need to pass multiple source ptrs to unpack)
            for(int i=0; i<unpack_height; ++i)
            {
                unpacker->unpack(out, in, unpack_width);
                for(size_t i=0; i<outputs.size(); ++i) out[i] += out_stride[i];
                in += in_stride;
            }
        }
    }

    ////////////////////////
    // static_device_info //
    ////////////////////////

    static_device_info::static_device_info()
    {
        for(auto & s : stream_subdevices) s = -1;
        for(auto & s : presets) for(auto & p : s) p = stream_request();
        for(auto & p : stream_poses)
        {
            p = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        }
    }

    subdevice_mode_selection static_device_info::select_mode(const stream_request (& requests)[RS_STREAM_NATIVE_COUNT], int subdevice_index) const
    {
        // Determine if the user has requested any streams which are supplied by this subdevice
        bool any_stream_requested = false;
        std::array<bool, RS_STREAM_NATIVE_COUNT> stream_requested = {};
        for(int j = 0; j < RS_STREAM_NATIVE_COUNT; ++j)
        {
            if(requests[j].enabled && stream_subdevices[j] == subdevice_index)
            {
                stream_requested[j] = true;
                any_stream_requested = true;
            }
        }

        // If no streams were requested, skip to the next subdevice
        if(!any_stream_requested) return subdevice_mode_selection(nullptr,0,nullptr);

        // Look for an appropriate mode
        for(auto & subdevice_mode : subdevice_modes)
        {
            // Skip modes that apply to other subdevices
            if(subdevice_mode.subdevice != subdevice_index) continue;

            for(auto pad_crop : subdevice_mode.pad_crop_options)
            {
                for(auto & unpacker : subdevice_mode.pf->unpackers)
                {
                    auto selection = subdevice_mode_selection(&subdevice_mode, pad_crop, &unpacker);

                    // Determine if this mode satisfies the requirements on our requested streams
                    auto stream_unsatisfied = stream_requested;
                    for(auto & output : unpacker.outputs)
                    {
                        const auto & req = requests[output.first];
                        if(req.enabled && (req.width == 0 || req.width == selection.get_width())
                                       && (req.height == 0 || req.height == selection.get_height())
                                       && (req.format == RS_FORMAT_ANY || req.format == selection.get_format(output.first))
                                       && (req.fps == 0 || req.fps == subdevice_mode.fps))
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

    std::vector<subdevice_mode_selection> static_device_info::select_modes(const stream_request (&reqs)[RS_STREAM_NATIVE_COUNT]) const
    {
        // Make a mutable copy of our array
        stream_request requests[RS_STREAM_NATIVE_COUNT];
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i) requests[i] = reqs[i];

        // Check and modify requests to enforce all interstream constraints
        for(auto & rule : interstream_rules)
        {
            auto & a = requests[rule.a], & b = requests[rule.b]; auto f = rule.field;
            if(a.enabled && b.enabled)
            {
                // Check for incompatibility if both values specified
                if(a.*f != 0 && b.*f != 0 && a.*f + rule.delta != b.*f && a.*f + rule.delta2 != b.*f)
                {
                    throw std::runtime_error(to_string() << "requested " << rule.a << " and " << rule.b << " settings are incompatible");
                }

                // If only one value is specified, modify the other request to match
                if(a.*f != 0 && b.*f == 0) b.*f = a.*f + rule.delta;
                if(a.*f == 0 && b.*f != 0) a.*f = b.*f - rule.delta;
            }
        }

        // Select subdevice modes needed to satisfy our requests
        int num_subdevices = 0;
        for(auto & mode : subdevice_modes) num_subdevices = std::max(num_subdevices, mode.subdevice+1);
        std::vector<subdevice_mode_selection> selected_modes;
        for(int i = 0; i < num_subdevices; ++i)
        {
            auto selection = select_mode(requests, i);
            if(selection.mode) selected_modes.push_back(selection);
        }
        return selected_modes;
    }

    ///////////////////
    // stream_buffer //
    ///////////////////

    /*stream_buffer::stream_buffer(subdevice_mode_selection selection, rs_stream stream)
        : selection(selection), frames({std::vector<byte>(selection.get_image_size(stream))}), last_frame_number() {}

    void stream_buffer::swap_back(int frame_number) 
    {
        if(frames.get_count() == 0) last_frame_number = frame_number;
        frames.get_back().timestamp = frame_number;
        frames.get_back().delta = frame_number - last_frame_number;
        last_frame_number = frame_number;     
        frames.swap_back();
    }*/

    ///////////////////
    // frame_archive //
    ///////////////////

    frame_archive::frame_archive(const std::vector<subdevice_mode_selection> & selection)
    {
        // Store the mode selection that pertains to each native stream
        for(auto & mode : selection)
        {
            for(auto & o : mode.unpacker->outputs)
            {
                modes[o.first] = mode;
            }
        }

        // Determine which stream will act as the key stream
        if(is_stream_enabled(RS_STREAM_DEPTH)) key_stream = RS_STREAM_DEPTH;
        else if(is_stream_enabled(RS_STREAM_INFRARED)) key_stream = RS_STREAM_INFRARED;
        else if(is_stream_enabled(RS_STREAM_INFRARED2)) key_stream = RS_STREAM_INFRARED2;
        else if(is_stream_enabled(RS_STREAM_COLOR)) key_stream = RS_STREAM_COLOR;
        else throw std::runtime_error("must enable at least one stream");

        // Enumerate all streams we need to keep synchronized with the key stream
        for(auto s : {RS_STREAM_INFRARED, RS_STREAM_INFRARED2, RS_STREAM_COLOR})
        {
            if(is_stream_enabled(s) && s != key_stream) other_streams.push_back(s);
        }

        // Allocate an empty image for each stream, and move it to the frontbuffer
        // This allows us to assume that get_frame_data/get_frame_timestamp always return valid data
        alloc_frame(key_stream, 0);
        frontbuffer[key_stream] = std::move(backbuffer[key_stream]);
        for(auto s : other_streams)
        {
            alloc_frame(s, 0);
            frontbuffer[s] = std::move(backbuffer[s]);
        }
    }

    // NOTE: Must be called with mutex acquired
    void frame_archive::dequeue_frame(rs_stream stream)
    {
        if(!frontbuffer[stream].data.empty()) freelist.push_back(std::move(frontbuffer[stream]));
        frontbuffer[stream] = std::move(frames[stream].front());
        frames[stream].erase(begin(frames[stream]));
    }
    void frame_archive::discard_frame(rs_stream stream)
    {
        freelist.push_back(std::move(frames[stream].front()));
        frames[stream].erase(begin(frames[stream]));    
    }

    void frame_archive::wait_for_frames()
    {
        // TODO: Implement a timeout in case the device hangs
        // TODO: Implement a user-specifiable timeout for how long to wait?

        // Wait until at least one frame is available on the key stream, and dequeue it
        std::unique_lock<std::mutex> lock(mutex);
        if(frames[key_stream].empty()) cv.wait(lock);
        dequeue_frame(key_stream);

        // Dequeue from other streams if the new frame is closer to the timestamp of the key stream than the old frame
        for(auto s : other_streams)
        {
            if(!frames[s].empty() && abs(frames[s].front().timestamp - frontbuffer[key_stream].timestamp) <= abs(frontbuffer[s].timestamp - frontbuffer[key_stream].timestamp))
            {
                dequeue_frame(s);
            }
        }
    }

    bool frame_archive::poll_for_frames()
    {
        // Check if least one frame is available on the key stream, and dequeue it
        std::unique_lock<std::mutex> lock(mutex);
        if(frames[key_stream].empty()) return false;
        dequeue_frame(key_stream);

        // Dequeue from other streams if the new frame is closer to the timestamp of the key stream than the old frame
        for(auto s : other_streams)
        {
            if(!frames[s].empty() && abs(frames[s].front().timestamp - frontbuffer[key_stream].timestamp) <= abs(frontbuffer[s].timestamp - frontbuffer[key_stream].timestamp))
            {
                dequeue_frame(s);
            }
        }
        return true;
    }

    const byte * frame_archive::get_frame_data(rs_stream stream) const { return frontbuffer[stream].data.data(); }
    int frame_archive::get_frame_timestamp(rs_stream stream) const { return frontbuffer[stream].timestamp; }

    byte * frame_archive::alloc_frame(rs_stream stream, int timestamp) 
    { 
        const size_t size = modes[stream].get_image_size(stream);

        {
            std::lock_guard<std::mutex> guard(mutex);

            // Attempt to obtain a buffer of the appropriate size from the freelist
            for(auto it = begin(freelist); it != end(freelist); ++it)
            {
                if(it->data.size() == size)
                {
                    backbuffer[stream] = std::move(*it);
                    freelist.erase(it);
                    break;
                }
            }

            // Discard buffers that have been in the freelist for longer than 1s
            for(auto it = begin(freelist); it != end(freelist); )
            {
                if(timestamp > it->timestamp + 1000) it = freelist.erase(it);
                else ++it;
            }
        }

        backbuffer[stream].data.resize(size); // TODO: Allow users to provide a custom allocator for frame buffers
        backbuffer[stream].timestamp = timestamp;
        return backbuffer[stream].data.data();
    }

    void frame_archive::cull_frames()
    {
        // Never keep more than four frames around in any given stream, regardless of timestamps
        for(auto s : {RS_STREAM_DEPTH, RS_STREAM_COLOR, RS_STREAM_INFRARED, RS_STREAM_INFRARED2})
        {
            while(frames[s].size() > 4)
            {
                discard_frame(s);
            }
        }

        // Cannot do any culling unless at least one frame is enqueued for each enabled stream
        if(frames[key_stream].empty()) return;
        for(auto s : other_streams) if(frames[s].empty()) return;

        // We can discard frames from the key stream if we have at least two and the latter is closer to the most recent frame of all other streams than the former
        while(true)
        {
            if(frames[key_stream].size() < 2) break;
            const int t0 = frames[key_stream][0].timestamp, t1 = frames[key_stream][1].timestamp;

            bool valid_to_skip = true;
            for(auto s : other_streams)
            {
                if(abs(t0 - frames[s].back().timestamp) < abs(t1 - frames[s].back().timestamp))
                {
                    valid_to_skip = false;
                    break;
                }
            }
            if(!valid_to_skip) break;

            discard_frame(key_stream);
        }

        // We can discard frames for other streams if we have at least two and the latter is closer to the next key stream frame than the former
        for(auto s : other_streams)
        {
            while(true)
            {
                if(frames[s].size() < 2) break;
                const int t0 = frames[s][0].timestamp, t1 = frames[s][1].timestamp;

                if(abs(t0 - frames[key_stream].front().timestamp) < abs(t1 - frames[key_stream].front().timestamp)) break;
                discard_frame(s);
            }
        }
    }

    void frame_archive::commit_frame(rs_stream stream) 
    {
        std::unique_lock<std::mutex> lock(mutex);
        frames[stream].push_back(std::move(backbuffer[stream]));
        cull_frames();
        lock.unlock();
        if(!frames[key_stream].empty()) cv.notify_one();
    }
}
