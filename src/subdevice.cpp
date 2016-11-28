// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"
#include <array>
#include <set>

using namespace rsimpl;

streaming_lock::streaming_lock() 
    : _is_streaming(false),
      _callback(nullptr, [](rs_frame_callback*) {}),
      _archive(std::make_shared<frame_archive>(&_max_publish_list_size)),
      _max_publish_list_size(16), _owner(nullptr)
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

rs_frame* streaming_lock::alloc_frame(size_t size, frame_additional_data additional_data) const
{
    auto frame = _archive->alloc_frame(size, additional_data, true);
    return _archive->track_frame(frame);
}

void streaming_lock::invoke_callback(rs_frame* frame_ref) const
{
    if (frame_ref)
    {
        auto callback = _archive->begin_callback();
        try
        {
            if (_callback) _callback->on_frame(frame_ref);
        }
        catch(...)
        {
            LOG_ERROR("Exception was thrown during user callback!");
        }
    }
}

void streaming_lock::flush() const
{
    _archive->flush();
}

streaming_lock::~streaming_lock()
{
    try {
        streaming_lock::stop();
    }
    catch (...)
    {
        // TODO: Write to Log
    }
}

std::vector<stream_profile> endpoint::get_principal_requests()
{
    std::unordered_set<stream_profile> results;
    std::set<uint32_t> unutilized_formats;
    std::set<uint32_t> supported_formats;

    auto profiles = get_stream_profiles();
    for (auto&& p : profiles)
    {
        supported_formats.insert(p.format);
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
            unutilized_formats.insert(p.format);
        }
    }

    for (auto& elem : unutilized_formats)
    {
        uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t>&>(elem);
        char fourcc[sizeof(device_fourcc) + 1];
        memcpy(fourcc, &device_fourcc, sizeof(device_fourcc));
        fourcc[sizeof(device_fourcc)] = 0;
        LOG_WARNING("Unutilized format " << fourcc);
    }

    if (!unutilized_formats.empty())
    {
        std::stringstream ss;
        for (auto& elem : supported_formats)
        {
            uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t>&>(elem);
            char fourcc[sizeof(device_fourcc) + 1];
            memcpy(fourcc, &device_fourcc, sizeof(device_fourcc));
            fourcc[sizeof(device_fourcc)] = 0;
            ss << fourcc << std::endl;
        }
        LOG_WARNING("\nDevice supported formats:\n" << ss.str());
    }

    std::vector<stream_profile> res{ begin(results), end(results) };
    std::sort(res.begin(), res.end(), [](const stream_profile& a, const stream_profile& b)
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


std::vector<request_mapping> endpoint::resolve_requests(std::vector<stream_profile> requests)
{
    std::vector<uint32_t> legal_4ccs;
    for (auto mode : get_stream_profiles()) {
        if (mode.fps == requests[0].fps && mode.height == requests[0].height && mode.width == requests[0].width)
            legal_4ccs.push_back(mode.format);
    }

    std::unordered_set<request_mapping> results;

    while (!requests.empty() && !_pixel_formats.empty())
    {
        auto max = 0;
        auto best_size = 0;
        auto best_pf = &_pixel_formats[0];
        auto best_unpacker = &_pixel_formats[0].unpackers[0];
        for (auto&& pf : _pixel_formats)
        {
            if (std::none_of(begin(legal_4ccs), end(legal_4ccs), [&](const uint32_t fourcc) {return fourcc == pf.fourcc; })) continue;
            for (auto&& unpacker : pf.unpackers)
            {
                auto count = static_cast<int>(std::count_if(begin(requests), end(requests),
                    [&unpacker](stream_profile& r)
                {
                    return unpacker.satisfies(r);
                }));

                // Here we check if the current pixel format / unpacker combination is better than the current best.
                // We judge on two criteria. A: how many of the requested streams can we supply? B: how many total streams do we open?
                // Optimally, we want to find a combination that supplies all the requested streams, and no additional streams.
                if (
                    count > max                                 // If the current combination supplies more streams, it is better.
                    || (count == max                            // Alternatively, if it supplies the same number of streams,
                        && unpacker.outputs.size() < best_size) // but this combination opens fewer total streams, it is also better
                    )
                {
                    max = count;
                    best_size = unpacker.outputs.size();
                    best_pf = &pf;
                    best_unpacker = &unpacker;
                }
            }
        }

        if (max == 0) break;

        requests.erase(std::remove_if(begin(requests), end(requests),
            [best_unpacker, best_pf, &results, this](stream_profile& r)
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

std::vector<uvc::stream_profile> uvc_endpoint::init_stream_profiles()
{
    power on(shared_from_this());
    return _device->get_profiles();
}

std::shared_ptr<streaming_lock> uvc_endpoint::configure(
    const std::vector<stream_profile>& requests)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    std::shared_ptr<uvc_streaming_lock> streaming(new uvc_streaming_lock(shared_from_this()));
    power on(shared_from_this());

    auto mapping = resolve_requests(requests);

    auto timestamp_readers = create_frame_timestamp_readers();
    auto timestamp_reader = timestamp_readers[0];

    for (auto& mode : mapping)
    {
        using namespace std::chrono;

        std::weak_ptr<uvc_streaming_lock> stream_ptr(streaming);

        auto start = high_resolution_clock::now();
        auto frame_number = 0;
        _device->probe_and_commit(mode.profile,
            [stream_ptr, mode, start, frame_number, timestamp_reader, requests](uvc::stream_profile p, uvc::frame_object f) mutable
        {
            auto&& unpacker = *mode.unpacker;

            auto stream = stream_ptr.lock();
            if (stream && stream->is_streaming())
            {
                frame_continuation release_and_enqueue([]() {}, f.pixels);

                // Ignore any frames which appear corrupted or invalid
                if (!timestamp_reader->validate_frame(mode, f.pixels)) return;

                // Determine the timestamp for this frame
                auto timestamp = timestamp_reader->get_frame_timestamp(mode, f.pixels);
                auto frame_counter = timestamp_reader->get_frame_counter(mode, f.pixels);
                //auto received_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - capture_start_time).count();

                auto width = mode.profile.width;
                auto height = mode.profile.height;
                auto fps = mode.profile.fps;

                std::vector<byte *> dest;
                std::vector<rs_frame*> refs;

                auto now = high_resolution_clock::now();
                auto ms = duration_cast<milliseconds>(now - start);
                auto system_time = static_cast<double>(ms.count());

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
                    frame_additional_data additional_data(timestamp,
                        frame_number++,
                        system_time,
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
                        dest.push_back(const_cast<byte*>(frame_ref->get()->get_frame_data()));
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
                    // all the streams the unpacker generates get here. If it matches one of the streams the user requested, dispatch it
                    if (std::any_of(begin(requests), end(requests), [&pref](stream_profile request) { return request.stream == pref->get()->get_stream_type(); }))
                        stream->invoke_callback(pref);
                    // otherwise, the stream is a garbage stream we were forced to open, and we simply deallocate the frame.
                    else
                        pref->get()->get_owner()->release_frame_ref(pref);
                }
            }
        });
    }

    _device->play();

    for (auto& mode : mapping)
    {
        _configuration.push_back(mode.profile);
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

option& endpoint::get_option(rs_option id)
{
    auto it = _options.find(id);
    if (it == _options.end())
    {
        throw std::runtime_error(to_string() 
            << "Device does not support option " 
            << rs_option_to_string(id) << "!");
    }
    return *it->second;
}

const option& endpoint::get_option(rs_option id) const
{
    auto it = _options.find(id);
    if (it == _options.end())
    {
        throw std::runtime_error(to_string() 
            << "Device does not support option " 
            << rs_option_to_string(id) << "!");
    }
    return *it->second;
}

bool endpoint::supports_option(rs_option id) const
{
    auto it = _options.find(id);
    if (it == _options.end()) return false;
    return it->second->is_enabled();
}

bool endpoint::supports_info(rs_camera_info info) const
{
    auto it = _camera_info.find(info);
    return it != _camera_info.end();
}

void endpoint::register_info(rs_camera_info info, std::string val)
{
    _camera_info[info] = std::move(val);
}

const std::string& endpoint::get_info(rs_camera_info info) const
{
    auto it = _camera_info.find(info);
    if (it == _camera_info.end())
        throw std::runtime_error("Selected camera info is not supported for this camera!");
    return it->second;
}

void endpoint::register_option(rs_option id, std::shared_ptr<option> option)
{
    _options[id] = std::move(option);
}

void uvc_endpoint::register_pu(rs_option id)
{
    register_option(id, std::make_shared<uvc_pu_option>(*this, id));
}
