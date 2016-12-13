// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"
#include <array>
#include <set>

using namespace rsimpl;

rs_frame* endpoint::alloc_frame(size_t size, frame_additional_data additional_data) const
{
    auto frame = _archive->alloc_frame(size, additional_data, true);
    return _archive->track_frame(frame);
}

void endpoint::invoke_callback(rs_frame* frame_ref) const
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

void endpoint::flush() const
{
    if (_archive.get())
        _archive->flush();
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

uvc_endpoint::~uvc_endpoint()
{
    try
    {
        if (_is_streaming)
            stop_streaming();

        if (_is_opened)
            close();
    }
    catch(...)
    {
        LOG_ERROR("An error has occurred while stop_streaming()!");
    }
}

std::vector<uvc::stream_profile> uvc_endpoint::init_stream_profiles()
{
    power on(shared_from_this());
    return _device->get_profiles();
}

std::vector<stream_profile> uvc_endpoint::get_principal_requests()
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

    // Sort the results to make sure that the user will receive predictable deterministic output from the API
    std::vector<stream_profile> res{ begin(results), end(results) };
    std::sort(res.begin(), res.end(), [](const stream_profile& a, const stream_profile& b)
    {
        auto at = std::make_tuple(a.stream, a.width, a.height, a.fps, a.format);
        auto bt = std::make_tuple(b.stream, b.width, b.height, b.fps, b.format);

        return at > bt;
    });

    return res;
}

void uvc_endpoint::open(const std::vector<stream_profile>& requests)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is streaming!");
    else if (_is_opened)
        throw std::runtime_error("Device is already opened!");

    auto on = std::unique_ptr<power>(new power(shared_from_this()));
    _archive = std::make_shared<frame_archive>(&_max_publish_list_size);
    auto mapping = resolve_requests(requests);

    auto timestamp_readers = create_frame_timestamp_readers();
    auto timestamp_reader = timestamp_readers[0];

    std::vector<request_mapping> commited;
    for (auto& mode : mapping)
    {
        try
        {
            using namespace std::chrono;

            auto start = high_resolution_clock::now();
            auto frame_number = 0;
            _device->probe_and_commit(mode.profile,
            [this, mode, start, frame_number, timestamp_reader, requests](uvc::stream_profile p, uvc::frame_object f) mutable
            {
            if (!this->is_streaming())
                return;

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
            auto&& unpacker = *mode.unpacker;
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

                auto frame_ref = this->alloc_frame(width * height * bpp / 8, additional_data);
                if (frame_ref)
                {
                    refs.push_back(frame_ref);
                    dest.push_back(const_cast<byte*>(frame_ref->get()->get_frame_data()));
                }
                else
                {
                    return;
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
                // all the streams the unpacker generates are handled here.
                // If it matches one of the streams the user requested, send it to the user.
                if (std::any_of(begin(requests), end(requests), [&pref](stream_profile request) { return request.stream == pref->get()->get_stream_type(); }))
                    this->invoke_callback(pref);
                // However, if this is an extra stream we had to open to properly satisty the user's request,
                // deallocate the frame and prevent the excess data from reaching the user.
                else
                    pref->get()->get_owner()->release_frame_ref(pref);
             }
            });
        }
        catch(...)
        {
            for (auto&& commited_mode : commited)
            {
                _device->stop(mode.profile);
            }
            throw;
        }
        commited.push_back(mode);
    }

    for (auto& mode : mapping)
    {
        _configuration.push_back(mode.profile);
    }
    _power = move(on);
    _is_opened = true;

    try {
        _device->play();
    }
    catch (...)
    {
        for (auto& profile : _configuration)
        {
            try {
                _device->stop(profile);
            }
            catch (...) {}
        }
        reset_streaming();
        throw;
    }
}

void uvc_endpoint::close()
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is streaming!");
    else if (!_is_opened)
        throw std::runtime_error("Device was not opened!");

    reset_streaming();
    _power.reset();
    _is_opened = false;
}

void uvc_endpoint::register_xu(uvc::extension_unit xu)
{
    _xus.push_back(std::move(xu));
}

std::vector<std::shared_ptr<frame_timestamp_reader>> uvc_endpoint::create_frame_timestamp_readers() const
{
    auto the_reader = std::make_shared<rolling_timestamp_reader>(); // single shared timestamp reader for all subdevices
    return{ the_reader, the_reader };                               // clone the reference for color and depth
}

void uvc_endpoint::start_streaming(frame_callback_ptr callback)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is already streaming!");

    _callback = std::move(callback);
    _is_streaming = true;
}

void uvc_endpoint::stop_streaming()
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (!_is_streaming)
        throw std::runtime_error("Device is not streaming!");

    _is_streaming = false;
    for (auto& profile : _configuration)
    {
        _device->stop(profile);
    }
    reset_streaming();
}

void uvc_endpoint::reset_streaming()
{
    flush();
    _configuration.clear();
    _callback.reset();
    _archive.reset();
}

void uvc_endpoint::acquire_power()
{
    if (_user_count.fetch_add(1) == 0)
    {
        _device->set_power_state(uvc::D0);
        for (auto& xu : _xus) _device->init_xu(xu);
    }
}

void uvc_endpoint::release_power()
{
    if (_user_count.fetch_add(-1) == 1) _device->set_power_state(uvc::D3);
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

hid_endpoint::~hid_endpoint()
{
    try
    {
        if (_is_streaming)
            stop_streaming();

        if (_is_opened)
            close();
    }
    catch(...)
    {
        LOG_ERROR("An error has occurred while stop_streaming()!");
    }
}

std::vector<uvc::stream_profile> hid_endpoint::init_stream_profiles()
{
    std::vector<uvc::stream_profile> results;
    for (auto& elem : get_device_profiles())
    {
        results.push_back({elem.width, elem .height, elem .fps, uint32_t(elem .format)});
    }
    return results;
}

std::vector<stream_profile> hid_endpoint::get_principal_requests()
{
    return get_device_profiles();
}

void hid_endpoint::open(const std::vector<stream_profile>& requests)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is streaming!");
    else if (_is_opened)
        throw std::runtime_error("Device is already opened!");

    for (auto& elem : requests)
    {
        _sensor_iio.push_back(rs_stream_to_sensor_iio(elem.stream));
    }
    _is_opened = true;
}

// TODO: lock device on open and unlock on close
void hid_endpoint::close()
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is streaming!");
    else if (!_is_opened)
        throw std::runtime_error("Device was not opened!");

    _sensor_iio.empty();
    _is_opened = false;
}

void hid_endpoint::start_streaming(frame_callback_ptr callback)
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (_is_streaming)
        throw std::runtime_error("Device is already streaming!");
    else if(!_is_opened)
        throw std::runtime_error("Device was not opened!");

    _archive = std::make_shared<frame_archive>(&_max_publish_list_size);
    _callback = std::move(callback);
    _is_streaming = true;
    _hid_device->start_capture(_sensor_iio, [this](const uvc::callback_data& data){
        if (!this->is_streaming())
            return;

        frame_additional_data additional_data;
        auto stream_format = sensor_name_to_stream_format(data.sensor.name);
        additional_data.format = stream_format.format;
        additional_data.stream_type = stream_format.stream;
        auto data_size = sizeof(data.value);
        additional_data.width = data_size;
        additional_data.height = 1;
        auto frame = this->alloc_frame(data_size, additional_data);

        std::vector<byte> raw_data;
        for (auto i = 0 ; i < sizeof(data.value); ++i)
        {
            raw_data.push_back((data.value >> (CHAR_BIT * i)) & 0xff);
        }

        memcpy(frame->get()->data.data(), raw_data.data(), data_size);
        this->invoke_callback(frame);
    });
}

void hid_endpoint::stop_streaming()
{
    std::lock_guard<std::mutex> lock(_configure_lock);
    if (!_is_streaming)
        throw std::runtime_error("Device is not streaming!");

    _is_streaming = false;
    _hid_device->stop_capture();
    flush();
    _sensor_iio.clear();
    _callback.reset();
    _archive.reset();
}

std::vector<stream_profile> hid_endpoint::get_device_profiles()
{
    std::vector<stream_profile> stream_requests;
    for (auto& elem : _hid_device->get_sensors())
    {
        auto stream_format = sensor_name_to_stream_format(elem.name);
        stream_requests.push_back({ stream_format.stream,
                                   0,
                                   0,
                                   0,
                                   stream_format.format});
    }

    return stream_requests;
}

int hid_endpoint::rs_stream_to_sensor_iio(rs_stream stream)
{
    for (auto& elem : sensor_name_and_stream_format)
    {
        if (stream == elem.second.stream)
            return get_iio_by_name(elem.first);
    }
    throw std::runtime_error("rs_stream not found!");
}

int hid_endpoint::get_iio_by_name(const std::string& name) const
{
    auto sensors =_hid_device->get_sensors();
    for (auto& elem : sensors)
    {
        if (!elem.name.compare(name))
            return elem.iio;
    }
    throw std::runtime_error("sensor_name not found!");
}

hid_endpoint::stream_format hid_endpoint::sensor_name_to_stream_format(const std::string& sensor_name) const
{
    stream_format stream_and_format;
    try{
        stream_and_format = sensor_name_and_stream_format.at(sensor_name);
    }
    catch(std::out_of_range)
    {
        throw std::runtime_error("format not found!");
    }

    return stream_and_format;
}
