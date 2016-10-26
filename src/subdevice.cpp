// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"
#include <array>

using namespace rsimpl;

streaming_lock::streaming_lock() : _is_streaming(false),
_callback(nullptr, [](rs_frame_callback*) {}),
_archive(&max_publish_list_size), _owner(nullptr)
{

}

void streaming_lock::play(frame_callback_ptr callback)
{
    std::lock_guard<std::mutex> lock(_callback_mutex);
    _callback = std::move(callback);
    _is_streaming = true;
}

void streaming_lock::stop()
{
    _is_streaming = false;
    flush();
    std::lock_guard<std::mutex> lock(_callback_mutex);
    _callback.reset();
}

void streaming_lock::release_frame(rs_frame* frame)
{
    _archive.release_frame_ref(frame);
}

rs_frame* streaming_lock::alloc_frame(size_t size, frame_additional_data additional_data)
{
    auto frame = _archive.alloc_frame(size, additional_data, true);
    return _archive.track_frame(frame);
}

void streaming_lock::invoke_callback(rs_frame* frame_ref) const
{
    if (frame_ref)
    {
        frame_ref->update_frame_callback_start_ts(std::chrono::high_resolution_clock::now());
        //frame_ref->log_callback_start(capture_start_time);
        //on_before_callback(streams[i], frame_ref, archive);
        _callback->on_frame(_owner, frame_ref);
    }
}

void streaming_lock::flush()
{
    _archive.flush();
}

streaming_lock::~streaming_lock()
{
    stop();
}

std::vector<stream_request> endpoint::get_principal_requests()
{
    std::unordered_set<stream_request> results;

    auto profiles = get_stream_profiles();
    for (auto&& p : profiles)
    {
        native_pixel_format pf;
        if (try_get_pf(p, pf))
        {
            for (auto&& unpacker : pf.unpackers)
            {
                for (auto&& output : unpacker.outputs)
                {
                    results.insert({ output.first, p.width, p.height, p.fps, output.second });
                }
            }
        }
        else
        {
            uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t>&>(p.format);
            char fourcc[sizeof(device_fourcc) + 1];
            memcpy(fourcc, &device_fourcc, sizeof(device_fourcc));
            fourcc[sizeof(device_fourcc)] = 0;
            LOG_WARNING("Unsupported pixel-format " << fourcc);
        }
    }

    std::vector<stream_request> res{ begin(results), end(results) };
    std::sort(res.begin(), res.end(), [](const stream_request& a, const stream_request& b)
    {
        return a.width > b.width;
    });
    return res;
}

bool endpoint::try_get_pf(const uvc::stream_profile& p, native_pixel_format& result) const
{
    auto it = std::find_if(begin(_pixel_formats), end(_pixel_formats),
        [&p](const native_pixel_format& pf)
    {
        return pf.fourcc == p.format;
    });
    if (it != end(_pixel_formats))
    {
        result = *it;
        return true;
    }
    return false;
}


std::vector<request_mapping> endpoint::resolve_requests(std::vector<stream_request> requests)
{
    if (!auto_complete_request(requests))
    {
        throw std::runtime_error("Subdevice could not auto complete requests!");
    }

    std::unordered_set<request_mapping> results;

    while (!requests.empty() && !_pixel_formats.empty())
    {
        auto max = 0;
        auto best_pf = &_pixel_formats[0];
        auto best_unpacker = &_pixel_formats[0].unpackers[0];
        for (auto&& pf : _pixel_formats)
        {
            for (auto&& unpacker : pf.unpackers)
            {
                auto count = static_cast<int>(std::count_if(begin(requests), end(requests),
                    [&unpacker](stream_request& r)
                {
                    return unpacker.satisfies(r);
                }));
                if (count > max && unpacker.outputs.size() == count)
                {
                    max = count;
                    best_pf = &pf;
                    best_unpacker = &unpacker;
                }
            }
        }

        if (max == 0) break;

        requests.erase(std::remove_if(begin(requests), end(requests),
            [best_unpacker, best_pf, &results, this](stream_request& r)
        {

            if (best_unpacker->satisfies(r))
            {
                request_mapping mapping;
                mapping.profile = { r.width, r.height, r.fps, best_pf->fourcc };
                mapping.unpacker = best_unpacker;
                mapping.pf = best_pf;

                results.insert(mapping);
                return true;
            }
            return false;
        }), end(requests));
    }

    if (requests.empty()) return{ begin(results), end(results) };

    throw std::runtime_error("Subdevice unable to satisfy stream requests!");
}

bool rsimpl::endpoint::auto_complete_request(std::vector<stream_request>& requests)
{
    for (stream_request& request : requests)
    {
        for (auto&& req : get_principal_requests())
        {
            if (req.match(request) && !req.contradicts(requests))
            {
                request = req;
                break;
            }
        }

        if (request.has_wildcards()) throw std::runtime_error("Subdevice auto complete the stream requests!");
    }
    return true;
}


std::vector<uvc::stream_profile> uvc_endpoint::get_stream_profiles()
{
    power on(this);
    return _device->get_profiles();
}

std::shared_ptr<streaming_lock> uvc_endpoint::configure(
    const std::vector<stream_request>& requests)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
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
            if (stream && stream->is_streaming())
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
                std::vector<rs_frame*> refs;

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
                if (dest.size() > 0)
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
    std::lock_guard<std::mutex> lock(_configure_lock);
    for (auto& profile : _configuration)
    {
        _device->stop(profile);
    }
    _configuration.clear();
}

void uvc_endpoint::acquire_power()
{
    std::lock_guard<std::mutex> lock(_power_lock);
    if (!_user_count)
    {
        _device->set_power_state(uvc::D0);
        for (auto& xu : _xus) _device->init_xu(xu);
    }
    _user_count++;
}

void uvc_endpoint::release_power()
{
    std::lock_guard<std::mutex> lock(_power_lock);
    _user_count--;
    if (!_user_count) _device->set_power_state(uvc::D3);
}
