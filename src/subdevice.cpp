// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "sync.h"
#include "motion-module.h"
#include "image.h"
#include <array>

using namespace rsimpl;

std::vector<uvc::stream_profile> uvc_endpoint::get_stream_profiles()
{
    power on(this);
    return _device->get_profiles();
}

std::shared_ptr<streaming_lock> uvc_endpoint::configure(
    const std::vector<stream_request>& requests)
{
    //std::lock_guard<std::mutex> lock(_configure_lock);
    std::shared_ptr<uvc_streaming_lock> streaming(new uvc_streaming_lock(this));
    power on(this);

    auto mapping = resolve_requests(requests);

    for (auto& mode : mapping)
    {
        std::weak_ptr<uvc_streaming_lock> stream_ptr(streaming);
        _device->play(mode.profile, [stream_ptr, mode](uvc::stream_profile p, uvc::frame_object f)
        {
            auto&& unpacker = *mode.unpacker;

            auto stream = stream_ptr.lock();
            if (stream)
            {
                auto now = std::chrono::system_clock::now().time_since_epoch();
                auto sys_time = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

                frame_continuation release_and_enqueue([]() {}, f.pixels);

                // Ignore any frames which appear corrupted or invalid
                //if (!timestamp_reader->validate_frame(mode_selection.mode, frame)) return;

                // Determine the timestamp for this frame
                //auto timestamp = timestamp_reader->get_frame_timestamp(mode_selection.mode, frame);
                //auto frame_counter = timestamp_reader->get_frame_counter(mode_selection.mode, frame);
                //auto received_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - capture_start_time).count();

                auto requires_processing = unpacker.requires_processing;

                auto width = mode.profile.width;
                auto height = mode.profile.height;
                auto fps = mode.profile.fps;

                std::vector<byte *> dest;
                std::vector<rs_frame_ref*> refs;

                //auto stride_x = mode_selection.get_stride_x();
                //auto stride_y = mode_selection.get_stride_y();
                /*for (auto&& output : unpacker.outputs)
                {
                    LOG_DEBUG("FrameAccepted, RecievedAt," << received_time
                        << ", FWTS," << timestamp << ", DLLTS," << received_time << ", Type," << rsimpl::get_string(output.first) << ",HasPair,0,F#," << frame_counter);
                }*/

                //frame_drops_status->was_initialized = true;

                // Not updating prev_frame_counter when first frame arrival
                /*if (frame_drops_status->was_initialized)
                {
                    frames_drops_counter.fetch_add(int(frame_counter - frame_drops_status->prev_frame_counter - 1));
                    frame_drops_status->prev_frame_counter = frame_counter;
                }*/

                for (auto&& output : unpacker.outputs)
                {
                    auto bpp = get_image_bpp(output.second);
                    frame_additional_data additional_data(0,
                        0,
                        sys_time,
                        width,
                        height,
                        fps,
                        width,
                        bpp,
                        output.second,
                        output.first);

                    auto frame_ref = stream->alloc_frame(width * height * bpp / 8, additional_data);
                    if (frame_ref)
                    {
                        refs.push_back(frame_ref);
                        dest.push_back(const_cast<byte*>(frame_ref->get_frame_data()));
                    }

                    // Obtain buffers for unpacking the frame
                    //dest.push_back(archive->alloc_frame(output.first, additional_data, requires_processing));
                }

                // Unpack the frame
                //if (requires_processing)
                {
                    unpacker.unpack(dest.data(), reinterpret_cast<const byte *>(f.pixels), width * height);
                }

                // If any frame callbacks were specified, dispatch them now
                for (auto&& pref : refs)
                {
                    //if (!requires_processing)
                    //{
                    //    archive->attach_continuation(streams[i], std::move(release_and_enqueue));
                    //}

                    //auto frame_ref = archive->track_frame(streams[i]);
                    pref->update_frame_callback_start_ts(std::chrono::high_resolution_clock::now());
                    //frame_ref->log_callback_start(capture_start_time);
                    stream->invoke_callback(pref);
                }
            }
        });
    }

    return std::move(streaming);
}

void uvc_endpoint::stop_streaming()
{
    //std::lock_guard<std::mutex> lock(_configure_lock);
    for (auto& profile : _configuration)
    {
        _device->stop(profile);
    }
    _configuration.clear();
}
