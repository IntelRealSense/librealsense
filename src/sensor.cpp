// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"
#include "algo.h"
#include "metadata-parser.h"

#include <array>
#include <set>
#include <unordered_set>
#include "device.h"

namespace librealsense
{
    sensor_base::sensor_base(std::string name, std::shared_ptr<platform::time_service> ts, const device* dev)
        : _source(ts),
          _device(dev),
        _is_streaming(false),
        _is_opened(false),
        _stream_profiles([this]() { return this->init_stream_profiles(); }),
        _notifications_proccessor(std::shared_ptr<notifications_proccessor>(new notifications_proccessor())),
        _metadata_parsers(std::make_shared<metadata_parser_map>()),
        _on_before_frame_callback(nullptr),
        _on_open(nullptr),
        _ts(ts)
    {
        register_option(RS2_OPTION_FRAMES_QUEUE_SIZE, _source.get_published_size_option());

        register_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL, std::make_shared<librealsense::md_time_of_arrival_parser>());

        register_info(RS2_CAMERA_INFO_NAME, name);
    }

    void sensor_base::register_notifications_callback(notifications_callback_ptr callback)
    {
        _notifications_proccessor->set_callback(std::move(callback));
    }

    std::shared_ptr<notifications_proccessor> sensor_base::get_notifications_proccessor()
    {
        return _notifications_proccessor;
    }

    rs2_extrinsics sensor_base::get_extrinsics_to(rs2_stream from, const sensor_interface& other, rs2_stream to) const
    {
        return _device->get_extrinsics(_device->find_sensor_idx(*this), from, _device->find_sensor_idx(other), to);
    }

    bool sensor_base::try_get_pf(const platform::stream_profile& p, native_pixel_format& result) const
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

    std::vector<request_mapping> sensor_base::resolve_requests(std::vector<stream_profile> requests)
    {
        // per requested profile, find all 4ccs that support that request.
        std::map<rs2_stream, std::set<uint32_t>> legal_fourccs;
        auto profiles = get_stream_profiles();
        for (auto&& request : requests) {
             for (auto&& mode : profiles) {
                if (mode.fps == request.fps && mode.height == request.height && mode.width == request.width)
                    legal_fourccs[request.stream].insert(mode.format);
            }
        }

        //if you want more efficient data structure use std::unordered_set
        //with well-defined hash function
        std::set <request_mapping> results;

        while (!requests.empty() && !_pixel_formats.empty())
        {
            auto max = 0;
            size_t best_size = 0;
            auto best_pf = &_pixel_formats.front();
            auto best_unpacker = &_pixel_formats.front().unpackers.front();
            for (auto&& pf : _pixel_formats)
            {
                // Speeds up algorithm by skipping obviously useless 4ccs
                // if (std::none_of(begin(legal_fourccs), end(legal_fourccs), [&](const uint32_t fourcc) {return fourcc == pf.fourcc; })) continue;
                for (auto&& unpacker : pf.unpackers)
                {
                    auto count = static_cast<int>(std::count_if(begin(requests), end(requests),
                        [&pf, &legal_fourccs, &unpacker](stream_profile& r)
                    {
                        // only count if the 4cc can be unpacked into the relevant stream/format
                        // and also, the pixel format can be streamed in the requested dimensions/fps.
                        return unpacker.satisfies(r) && legal_fourccs[r.stream].count(pf.fourcc);
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
                [best_unpacker, best_pf, &results, &legal_fourccs, this](stream_profile& r)
            {
                if (best_unpacker->satisfies(r) && legal_fourccs[r.stream].count(best_pf->fourcc))
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

        throw invalid_value_exception("Subdevice unable to satisfy stream requests!");
    }

    uvc_sensor::~uvc_sensor()
    {
        try
        {
            if (_is_streaming)
                stop();

            if (_is_opened)
                close();
        }
        catch(...)
        {
            LOG_ERROR("An error has occurred while stop_streaming()!");
        }
    }

    std::vector<platform::stream_profile> uvc_sensor::init_stream_profiles()
    {
        power on(std::dynamic_pointer_cast<uvc_sensor>(shared_from_this()));
        return _device->get_profiles();
    }

    std::vector<stream_profile> uvc_sensor::get_principal_requests()
    {
        std::unordered_set<stream_profile> results;
        std::set<uint32_t> unutilized_formats;
        std::set<uint32_t> supported_formats;

        auto profiles = get_stream_profiles();
        for (auto&& p : profiles)
        {
            supported_formats.insert(p.format);
            native_pixel_format pf{};
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

        if (unutilized_formats.size())
        {
            std::stringstream ss;
            ss << "Unused media formats : ";
            for (auto& elem : unutilized_formats)
            {
                uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t>&>(elem);
                char fourcc[sizeof(device_fourcc) + 1];
            librealsense::copy(fourcc, &device_fourcc, sizeof(device_fourcc));
                fourcc[sizeof(device_fourcc)] = 0;
                ss << fourcc << " ";
            }

            ss << "; Device-supported: ";
            for (auto& elem : supported_formats)
            {
                uint32_t device_fourcc = reinterpret_cast<const big_endian<uint32_t>&>(elem);
                char fourcc[sizeof(device_fourcc) + 1];
                librealsense::copy(fourcc, &device_fourcc, sizeof(device_fourcc));
                fourcc[sizeof(device_fourcc)] = 0;
                ss << fourcc << " ";
            }
            LOG_WARNING(ss.str());
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

    const device_interface& sensor_base::get_device()
    {
        return *_device;
    }

    void uvc_sensor::open(const std::vector<stream_profile>& requests)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("open(...) failed. UVC device is streaming!");
        else if (_is_opened)
            throw wrong_api_call_sequence_exception("open(...) failed. UVC device is already opened!");

        auto on = std::unique_ptr<power>(new power(std::dynamic_pointer_cast<uvc_sensor>(shared_from_this())));
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());
        auto mapping = resolve_requests(requests);

        auto timestamp_reader = _timestamp_reader.get();

        std::vector<request_mapping> commited;

        for (auto&& mode : mapping)
        {
            try
            {
                _device->probe_and_commit(mode.profile, !mode.requires_processing(),
                [this, mode, timestamp_reader, requests](platform::stream_profile p, platform::frame_object f, std::function<void()> continuation) mutable
                {

                auto system_time = _ts->get_time();

                if (!this->is_streaming())
                {
                    LOG_WARNING("Frame received with streaming inactive," << librealsense::get_string(mode.unpacker->outputs.front().first)
                            << ", Arrived," << std::fixed << system_time);
                    return;
                }

                frame_continuation release_and_enqueue(continuation, f.pixels);

                // Ignore any frames which appear corrupted or invalid
                // Determine the timestamp for this frame
                auto timestamp = timestamp_reader->get_frame_timestamp(mode, f);
                auto timestamp_domain = timestamp_reader->get_frame_timestamp_domain(mode, f);

                auto frame_counter = timestamp_reader->get_frame_counter(mode, f);

                auto requires_processing = mode.requires_processing();

                auto width = mode.profile.width;
                auto height = mode.profile.height;
                auto fps = mode.profile.fps;

                std::vector<byte *> dest;
                std::vector<frame_holder> refs;

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
                    LOG_DEBUG("FrameAccepted," << std::dec<< librealsense::get_string(output.first) << "," << frame_counter
                        << ",Arrived," << std::fixed << system_time
                        << ",TS," << std::fixed << timestamp << ",TS_Domain," << rs2_timestamp_domain_to_string(timestamp_domain));

                    auto bpp = get_image_bpp(output.second);
                    frame_additional_data additional_data(timestamp,
                        frame_counter,
                        system_time,
                        output.second,
                        output.first,
                        fps,
                        static_cast<uint8_t>(f.metadata_size),
                        (const uint8_t*)f.metadata);

                    frame_holder frame = _source.alloc_frame(RS2_EXTENSION_TYPE_VIDEO_FRAME, width * height * bpp / 8, additional_data, requires_processing);
                    if (frame.frame)
                    {
                        auto video = (video_frame*)frame.frame;
                        video->assign(width, height, width * bpp / 8, bpp);
                        video->set_timestamp_domain(timestamp_domain);
                        dest.push_back(const_cast<byte*>(video->get_frame_data()));
                        refs.push_back(std::move(frame));
                    }
                    else
                    {
                        LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
                        return;
                    }

                    // Obtain buffers for unpacking the frame
                    //dest.push_back(archive->alloc_frame(output.first, additional_data, requires_processing));
                }

                // Unpack the frame
                if (requires_processing && (dest.size() > 0))
                {
                    unpacker.unpack(dest.data(), reinterpret_cast<const byte *>(f.pixels), width * height);
                }

                // If any frame callbacks were specified, dispatch them now
                for (auto&& pref : refs)
                {
                    if (!requires_processing)
                    {
                        pref->attach_continuation(std::move(release_and_enqueue));
                    }

                    // all the streams the unpacker generates are handled here.
                    // If it matches one of the streams the user requested, send it to the user.
                    if (std::any_of(begin(requests), end(requests), [&pref](stream_profile request) { return request.stream == pref->get_stream_type(); }))
                    {
                        if (_on_before_frame_callback)
                        {
                            auto callback = _source.begin_callback();
                            auto stream_type = pref->get_stream_type();
                            _on_before_frame_callback(stream_type, pref, std::move(callback));
                        }

                        _source.invoke_callback(std::move(pref));
                    }
                 }
                },
                static_cast<int>(_source.get_published_size_option()->query()));
            }
            catch(...)
            {
                for (auto&& commited_mode : commited)
                {
                    _device->close(mode.profile);
                }
                throw;
            }
            commited.push_back(mode);
        }

        for (auto& mode : mapping)
        {
            _configuration.push_back(mode.profile);
        }

        if (_on_open)
            _on_open(_configuration);

        _power = move(on);
        _is_opened = true;

        try {
            _device->stream_on([&](const notification& n)
            {
                _notifications_proccessor->raise_notification(n);
            });
        }
        catch (...)
        {
            for (auto& profile : _configuration)
            {
                try {
                    _device->close(profile);
                }
                catch (...) {}
            }
            reset_streaming();
            _is_opened = false;
            throw;
        }
    }

    void uvc_sensor::close()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. UVC device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. UVC device was not opened!");

        for (auto& profile : _configuration)
        {
            _device->close(profile);
        }
        reset_streaming();
        _power.reset();
        _is_opened = false;
    }

    void uvc_sensor::register_xu(platform::extension_unit xu)
    {
        _xus.push_back(std::move(xu));
    }

    void uvc_sensor::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. UVC device is already streaming!");
        else if(!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. UVC device was not opened!");

        _source.set_callback(callback);

        _is_streaming = true;
        _device->start_callbacks();
    }

    void uvc_sensor::stop()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. UVC device is not streaming!");

        _is_streaming = false;
        _device->stop_callbacks();
    }


    void uvc_sensor::reset_streaming()
    {
        _source.flush();
        _configuration.clear();
        _source.reset();
        _timestamp_reader->reset();
    }

    void uvc_sensor::acquire_power()
    {
        std::lock_guard<std::mutex> lock(_power_lock);
        if (_user_count.fetch_add(1) == 0)
        {
            _device->set_power_state(platform::D0);
            for (auto& xu : _xus) _device->init_xu(xu);
        }
    }

    void uvc_sensor::release_power()
    {
        std::lock_guard<std::mutex> lock(_power_lock);
        if (_user_count.fetch_add(-1) == 1)
        {
            _device->set_power_state(platform::D3);
        }
    }

    bool info_container::supports_info(rs2_camera_info info) const
    {
        auto it = _camera_info.find(info);
        return it != _camera_info.end();
    }

    void info_container::register_info(rs2_camera_info info, const std::string& val)
    {
        if (supports_info(info) && (get_info(info) != val)) // Append existing infos
        {
            _camera_info[info] += "\n" + std::move(val);
        }
        else
        {
            _camera_info[info] = std::move(val);
        }
    }

    const std::string& info_container::get_info(rs2_camera_info info) const
    {
        auto it = _camera_info.find(info);
        if (it == _camera_info.end())
            throw invalid_value_exception("Selected camera info is not supported for this camera!");

        return it->second;
    }
    void info_container::create_snapshot(std::shared_ptr<info_interface>& snapshot)
    {
        snapshot = std::make_shared<info_snapshot>(this);
    }
    void info_container::create_recordable(std::shared_ptr<info_interface>& recordable,
                                           std::function<void(std::shared_ptr<extension_snapshot>)> record_action)
    {
        recordable = std::make_shared<info_container>(*this);
    }

    void uvc_sensor::register_pu(rs2_option id)
    {
        register_option(id, std::make_shared<uvc_pu_option>(*this, id));
    }

    void sensor_base::register_metadata(rs2_frame_metadata metadata, std::shared_ptr<md_attribute_parser_base> metadata_parser)
    {
        if (_metadata_parsers.get()->end() != _metadata_parsers.get()->find(metadata))
            throw invalid_value_exception( to_string() << "Metadata attribute parser for " << rs2_frame_metadata_to_string(metadata)
                                           <<  " is already defined");

        _metadata_parsers.get()->insert(std::pair<rs2_frame_metadata, std::shared_ptr<md_attribute_parser_base>>(metadata, metadata_parser));
    }

    hid_sensor::~hid_sensor()
    {
        try
        {
            if (_is_streaming)
                stop();

            if (_is_opened)
                close();
        }
        catch(...)
        {
            LOG_ERROR("An error has occurred while stop_streaming()!");
        }
    }

    std::vector<platform::stream_profile> hid_sensor::init_stream_profiles()
    {
        std::unordered_set<platform::stream_profile> results;
        for (auto& elem : get_device_profiles())
        {
            results.insert({elem.width, elem.height, elem.fps, stream_to_fourcc(elem.stream)});
        }
        return std::vector<platform::stream_profile>(results.begin(), results.end());
    }

    std::vector<stream_profile> hid_sensor::get_sensor_profiles(std::string sensor_name) const
    {
        std::vector<stream_profile> profiles{};
        for (auto& elem : _sensor_name_and_hid_profiles)
        {
            if (!elem.first.compare(sensor_name))
            {
                profiles.push_back(elem.second);
            }
        }

        return profiles;
    }

    std::vector<stream_profile> hid_sensor::get_principal_requests()
    {
        return get_device_profiles();
    }

    void hid_sensor::open(const std::vector<stream_profile>& requests)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("open(...) failed. Hid device is streaming!");
        else if (_is_opened)
            throw wrong_api_call_sequence_exception("Hid device is already opened!");

        auto mapping = resolve_requests(requests);
        for (auto& request : requests)
        {
            auto sensor_name = rs2_stream_to_sensor_name(request.stream);
            for (auto& map : mapping)
            {
                auto it = std::find_if(begin(map.unpacker->outputs), end(map.unpacker->outputs),
                                       [&](const std::pair<rs2_stream, rs2_format>& pair)
                {
                    return pair.first == request.stream;
                });

                if (it != end(map.unpacker->outputs))
                {
                    _configured_profiles.insert(std::make_pair(sensor_name,
                                                               stream_profile{request.stream,
                                                                              request.width,
                                                                              request.height,
                                                                              fps_to_sampling_frequency(request.stream, request.fps),
                                                                              request.format}));
                    _is_configured_stream[request.stream] = true;
                    _hid_mapping.insert(std::make_pair(sensor_name, map));
                }
            }
        }

        std::vector<platform::hid_profile> configured_hid_profiles;
        for (auto& elem : _configured_profiles)
        {
            configured_hid_profiles.push_back(platform::hid_profile{elem.first, elem.second.fps});
        }
        _hid_device->open(configured_hid_profiles);
        _is_opened = true;
    }

    void hid_sensor::close()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. Hid device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. Hid device was not opened!");

        _hid_device->close();
        _configured_profiles.clear();
        _is_configured_stream.clear();
        _is_configured_stream.resize(RS2_STREAM_COUNT);
        _hid_mapping.clear();
        _is_opened = false;
    }

    // TODO:
    static rs2_stream custom_gpio_to_stream_type(uint32_t custom_gpio)
    {
        auto gpio = RS2_STREAM_GPIO1 + custom_gpio;
        if (gpio >= RS2_STREAM_GPIO1 &&
            gpio <= RS2_STREAM_GPIO4)
        {
            return static_cast<rs2_stream>(gpio);
        }

        LOG_ERROR("custom_gpio " << std::to_string(custom_gpio) << " is incorrect!");
        return RS2_STREAM_ANY;
    }

    void hid_sensor::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Hid device is already streaming!");
        else if(!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Hid device was not opened!");

        _source.set_callback(callback);
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());

        _hid_device->start_capture([this](const platform::sensor_data& sensor_data)
        {
            auto system_time = _ts->get_time();
            auto timestamp_reader = _hid_iio_timestamp_reader.get();

            // TODO:
            static const std::string custom_sensor_name = "custom";
            auto sensor_name = sensor_data.sensor.name;
            bool is_custom_sensor = false;
            static const uint32_t custom_source_id_offset = 16;
            uint8_t custom_gpio = 0;
            auto custom_stream_type = RS2_STREAM_ANY;
            if (sensor_name == custom_sensor_name)
            {
                custom_gpio = *(reinterpret_cast<uint8_t*>((uint8_t*)(sensor_data.fo.pixels) + custom_source_id_offset));
                custom_stream_type = custom_gpio_to_stream_type(custom_gpio);

                if (!_is_configured_stream[custom_stream_type])
                {
                    LOG_DEBUG("Unrequested " << rs2_stream_to_string(custom_stream_type) << " frame was dropped.");
                    return;
                }

                is_custom_sensor = true;
                timestamp_reader = _custom_hid_timestamp_reader.get();
            }

            if (!this->is_streaming())
            {
                LOG_INFO("HID Frame received when Streaming is not active,"
                            << get_string(_configured_profiles[sensor_name].stream)
                            << ",Arrived," << std::fixed << system_time);
                return;
            }

            auto mode = _hid_mapping[sensor_name];
            auto data_size = sensor_data.fo.frame_size;
            mode.profile.width = (uint32_t)data_size;
            mode.profile.height = 1;

            // Determine the timestamp for this HID frame
            auto timestamp = timestamp_reader->get_frame_timestamp(mode, sensor_data.fo);
            auto frame_counter = timestamp_reader->get_frame_counter(mode, sensor_data.fo);

            frame_additional_data additional_data{};
            additional_data.format = _configured_profiles[sensor_name].format;

            // TODO: Add frame_additional_data reader?
            if (is_custom_sensor)
                additional_data.stream_type = custom_stream_type;
            else
                additional_data.stream_type = _configured_profiles[sensor_name].stream;

            additional_data.fps = mode.profile.fps;
            additional_data.timestamp = timestamp;
            additional_data.frame_number = frame_counter;
            additional_data.timestamp_domain = timestamp_reader->get_frame_timestamp_domain(mode, sensor_data.fo);
            additional_data.system_time = system_time;

            LOG_DEBUG("FrameAccepted," << std::dec<< get_string(additional_data.stream_type) << "," << frame_counter
                      << ",Arrived," << std::fixed << system_time
                      << ",TS," << std::fixed << timestamp
                      << ",TS_Domain," << rs2_timestamp_domain_to_string(additional_data.timestamp_domain));

            auto frame = _source.alloc_frame(RS2_EXTENSION_TYPE_MOTION_FRAME, data_size, additional_data, true);
            if (!frame)
            {
                LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
                return;
            }

            std::vector<byte*> dest{const_cast<byte*>(frame->get_frame_data())};
            mode.unpacker->unpack(dest.data(),(const byte*)sensor_data.fo.pixels, (int)data_size);

            if (_on_before_frame_callback)
            {
                auto callback = _source.begin_callback();
                auto stream_type = frame->get_stream_type();
                _on_before_frame_callback(stream_type, frame, std::move(callback));
            }

            _source.invoke_callback(std::move(frame));
        });

        _is_streaming = true;
    }

    void hid_sensor::stop()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. Hid device is not streaming!");


        _hid_device->stop_capture();
        _is_streaming = false;
        _source.flush();
        _source.reset();
        _hid_iio_timestamp_reader->reset();
        _custom_hid_timestamp_reader->reset();
    }


    std::vector<stream_profile> hid_sensor::get_device_profiles()
    {
        std::vector<stream_profile> stream_requests;
        for (auto it = _hid_sensors.rbegin(); it != _hid_sensors.rend(); ++it)
        {
            auto profiles = get_sensor_profiles(it->name);
            stream_requests.insert(stream_requests.end(), profiles.begin() ,profiles.end());
        }

        return stream_requests;
    }

    const std::string& hid_sensor::rs2_stream_to_sensor_name(rs2_stream stream) const
    {
        for (auto& elem : _sensor_name_and_hid_profiles)
        {
            if (stream == elem.second.stream)
                return elem.first;
        }
        throw invalid_value_exception("rs2_stream not found!");
    }

    uint32_t hid_sensor::stream_to_fourcc(rs2_stream stream) const
    {
        uint32_t fourcc;
        try{
            fourcc = stream_and_fourcc.at(stream);
        }
        catch(std::out_of_range)
        {
            throw invalid_value_exception(to_string() << "fourcc of stream " << rs2_stream_to_string(stream) << " not found!");
        }

        return fourcc;
    }

    uint32_t hid_sensor::fps_to_sampling_frequency(rs2_stream stream, uint32_t fps) const
    {
        // TODO: Add log prints
        auto it = _fps_and_sampling_frequency_per_rs2_stream.find(stream);
        if (it == _fps_and_sampling_frequency_per_rs2_stream.end())
            return fps;

        auto fps_mapping = it->second.find(fps);
        if (fps_mapping != it->second.end())
            return fps_mapping->second;
        else
            return fps;
    }

}
