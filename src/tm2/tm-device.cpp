// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <memory>
#include <thread>
#include "tm-device.h"
#include "TrackingData.h"
#include "stream.h"
#include "tm-conversions.h"
#include "media/playback/playback_device.h"
#include "media/ros/ros_reader.h"
#include "controller_event_serializer.h"

using namespace perc;
using namespace std::chrono;

namespace librealsense
{
    void SetManualExposure(TrackingDevice* device, uint32_t integrationTimeInMili, float gain)
    {
        TrackingData::Exposure::StreamExposure leftStreamExposure;

        leftStreamExposure.cameraId = SET_SENSOR_ID(3, 0);
        leftStreamExposure.gain = gain;
        leftStreamExposure.integrationTime = integrationTimeInMili;

        TrackingData::Exposure::StreamExposure rightStreamExposure;
        rightStreamExposure.cameraId = SET_SENSOR_ID(3, 1);
        rightStreamExposure.gain = gain;
        rightStreamExposure.integrationTime = integrationTimeInMili;

        TrackingData::Exposure exp;
        exp.numOfVideoStreams = 2;
        exp.stream[0] = leftStreamExposure;
        exp.stream[1] = rightStreamExposure;

        Status status = device->SetExposure(exp);

        if (status != Status::SUCCESS)
            throw std::runtime_error("Failed set manual exposure");
    }


    void SetExposureMode(TrackingDevice* device, bool manualMode)
    {
        if (manualMode)
        {
            // Cancel autoExposure
            TrackingData::ExposureModeControl modeControl(0, 0);

            Status status = device->SetExposureModeControl(modeControl);
            if (status != Status::SUCCESS)
                throw std::runtime_error("Failed set manual exposure mode");
        }
        else
        {
            TrackingData::ExposureModeControl modeControl(3, 0);

            Status status = device->SetExposureModeControl(modeControl);

            if (status != Status::SUCCESS)
                throw std::runtime_error("Failed set manual exposure");
        }
    }


    struct video_frame_metadata
    {
        int64_t arrival_ts;
        uint32_t exposure_time;
    };

    struct motion_frame_metadata
    {
        int64_t arrival_ts;
        float temperature;
    };

    struct pose_frame_metadata
    {
        int64_t arrival_ts;
    };

    enum temperature_type { TEMPERATURE_TYPE_ASIC, TEMPERATURE_TYPE_MOTION};

    class temperature_option : public readonly_option
    {
    public:
        float query() const override
        {
            if (!is_enabled())
                throw wrong_api_call_sequence_exception("query option is allow only in streaming!");
            return _ep.get_temperature().sensor[_type].temperature;
        }

        option_range get_range() const override { return _range; }

        bool is_enabled() const override { return true; }

        explicit temperature_option(tm2_sensor& ep, temperature_type type) : _ep(ep), _type(type),
            _range(option_range{ 0, ep.get_temperature().sensor[type].threshold, 0, 0 }) { }

    private:
        tm2_sensor&         _ep;
        temperature_type    _type;
        option_range        _range;
    };

    class exposure_option : public option_base
    {
    public:
        float query() const override
        {
            return _ep.get_exposure();
        }

        void set(float value) override
        {
            return _ep.set_exposure(value);
        }

        const char* get_description() const override { return "Exposure"; }

        bool is_enabled() const override { return true; }

        explicit exposure_option(tm2_sensor& ep) : _ep(ep),
            option_base(option_range{ 200, 16000, 0, 200 }) { }

    private:
        tm2_sensor&         _ep;
    };

    class exposure_mode_option : public option_base
    {
    public:
        float query() const override
        {
            return 1.f - _ep.is_manual_exposure();
        }

        void set(float value) override
        {
            return _ep.set_manual_exposure(1.f - value);
        }

        const char* get_description() const override { return "Auto-Exposure"; }

        bool is_enabled() const override { return true; }

        explicit exposure_mode_option(tm2_sensor& ep) : _ep(ep),
            option_base(option_range{ 0, 1, 1, 1 }) { }

    private:
        tm2_sensor&         _ep;
    };

    class gain_option : public option_base
    {
    public:
        float query() const override
        {
            return _ep.get_gain();
        }

        void set(float value) override
        {
            return _ep.set_gain(value);
        }

        const char* get_description() const override { return "Gain"; }

        bool is_enabled() const override { return true; }

        explicit gain_option(tm2_sensor& ep) : _ep(ep),
            option_base(option_range{ 1, 10, 0, 1 }) { }

    private:
        tm2_sensor&         _ep;
    };

    template <perc::SIXDOF_MODE flag, perc::SIXDOF_MODE depends_on, bool invert = false>
    class tracking_mode_option : public option_base
    {
    public:
        float query() const override { return !!(s._tm_mode & flag) ^ invert ? 1 : 0; }

        void set(float value) override {
            if (s._is_streaming)
                throw io_exception("Option is read-only while streaming");
            s._tm_mode = (!!value ^ invert) ? (s._tm_mode | flag) : (s._tm_mode & ~flag);
        }

        const char* get_description() const override { return description; }

        bool is_enabled() const override { return !depends_on || (s._tm_mode & depends_on) ? true : false; }

        bool is_read_only() const override { return s._is_streaming; }

        explicit tracking_mode_option(tm2_sensor& sensor, const char *description_) :
            s(sensor), description(description_), option_base(option_range{ 0, 1, 1, !!(sensor._tm_mode & flag) ^ invert ? 1.f : 0.f }) { }

    private:
        tm2_sensor &s;
        const char *description;
    };

    class asic_temperature_option : public temperature_option
    {
    public:
        const char* get_description() const override
        {
            return "Current TM2 Asic Temperature (degree celsius)";
        }
        explicit asic_temperature_option(tm2_sensor& ep) :temperature_option(ep, temperature_type::TEMPERATURE_TYPE_ASIC) { }
    };

    class motion_temperature_option : public temperature_option
    {
    public:

        const char* get_description() const override
        {
            return "Current TM2 Motion Module Temperature (degree celsius)";
        }
        explicit motion_temperature_option(tm2_sensor& ep) :temperature_option(ep, temperature_type::TEMPERATURE_TYPE_MOTION) { }
    };

    class md_tm2_parser : public md_attribute_parser_base
    {
    public:
        md_tm2_parser(rs2_frame_metadata_value type) : _type(type) {}
        rs2_metadata_type get(const frame& frm) const override
        {
            if(_type == RS2_FRAME_METADATA_ACTUAL_EXPOSURE)
            {
                if (auto* vf = dynamic_cast<const video_frame*>(&frm))
                {
                    const video_frame_metadata* md = reinterpret_cast<const video_frame_metadata*>(frm.additional_data.metadata_blob.data());
                    return (rs2_metadata_type)(md->exposure_time);
                }
            }
            if(_type == RS2_FRAME_METADATA_TIME_OF_ARRIVAL)
            {
                // Note: additional_data.system_time is the arrival time
                // (backend_time is what we have traditionally called
                // system_time)
                if (auto* vf = dynamic_cast<const video_frame*>(&frm))
                {
                    return (rs2_metadata_type)(vf->additional_data.system_time);
                }
                if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
                {
                    return (rs2_metadata_type)(mf->additional_data.system_time);
                }
                if (auto* pf = dynamic_cast<const pose_frame*>(&frm))
                {
                    return (rs2_metadata_type)(pf->additional_data.system_time);
                }
            }
            if(_type == RS2_FRAME_METADATA_FRAME_TIMESTAMP)
            {
                if (auto* vf = dynamic_cast<const video_frame*>(&frm))
                {
                    return (rs2_metadata_type)(vf->additional_data.timestamp*1e+3);
                }
                if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
                {
                    return (rs2_metadata_type)(mf->additional_data.timestamp*1e+3);
                }
                if (auto* pf = dynamic_cast<const pose_frame*>(&frm))
                {
                    return (rs2_metadata_type)(pf->additional_data.timestamp*1e+3);
                }
            }
            if (_type == RS2_FRAME_METADATA_TEMPERATURE)
            {
                if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
                {
                    const motion_frame_metadata* md = reinterpret_cast<const motion_frame_metadata*>(frm.additional_data.metadata_blob.data());
                    return (rs2_metadata_type)(md->temperature);
                }
            }
            return 0;
        }

        bool supports(const frame& frm) const override
        {
            if (_type == RS2_FRAME_METADATA_ACTUAL_EXPOSURE)
            {
                return dynamic_cast<const video_frame*>(&frm) != nullptr;
            }
            if (_type == RS2_FRAME_METADATA_TEMPERATURE)
            {
                return dynamic_cast<const motion_frame*>(&frm) != nullptr;
            }
            if (_type == RS2_FRAME_METADATA_TIME_OF_ARRIVAL)
            {
                return dynamic_cast<const video_frame*>(&frm) != nullptr || dynamic_cast<const motion_frame*>(&frm) != nullptr;
            }
            if (_type == RS2_FRAME_METADATA_FRAME_TIMESTAMP)
            {
                return (dynamic_cast<const video_frame*>(&frm) != nullptr) || (dynamic_cast<const motion_frame*>(&frm) != nullptr) || (dynamic_cast<const pose_frame*>(&frm) != nullptr);
            }
            return false;
        }
    private:
        rs2_frame_metadata_value _type;
    };

    inline std::ostream&  operator<<(std::ostream& os, const uint8_t(&mac)[6])
    {
        return os << buffer_to_string(mac, ':', true);
    }

    tm2_sensor::tm2_sensor(tm2_device* owner, perc::TrackingDevice* dev)
        : sensor_base("Tracking Module", owner, this), _dispatcher(10), _tm_dev(dev)
    {
        _source.set_max_publish_list_size(64); //increase frame source queue size for TM2
        register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_ACTUAL_EXPOSURE));
        register_metadata(RS2_FRAME_METADATA_TEMPERATURE    , std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_TEMPERATURE));
        //Replacing md parser for RS2_FRAME_METADATA_TIME_OF_ARRIVAL
        _metadata_parsers->operator[](RS2_FRAME_METADATA_TIME_OF_ARRIVAL) = std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);
        _metadata_parsers->operator[](RS2_FRAME_METADATA_FRAME_TIMESTAMP) = std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
    }

    tm2_sensor::~tm2_sensor()
    {
        if (!_tm_dev)
            return;

        try
        {
            if (_is_streaming)
                stop();

            if (_is_opened)
                close();
        }
        catch (...)
        {
            LOG_ERROR("An error has occurred while stop_streaming()!");
        }
    }

    void tm2_sensor::dispose()
    {
        _tm_dev = nullptr;
    }

    //sensor
    ////////
    stream_profiles tm2_sensor::init_stream_profiles()
    {
        stream_profiles results;

        //get TM2 profiles
        Status status = _tm_dev->GetSupportedProfile(_tm_supported_profiles);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to get supported raw streams");
        }

        std::map<SensorId, std::shared_ptr<stream_profile_interface> > profile_map;
        //extract video profiles
        std::vector<TrackingData::VideoProfile> video_profiles(_tm_supported_profiles.video, _tm_supported_profiles.video + VideoProfileMax);
        for (auto tm_profile : video_profiles)
        {
            if (tm_profile.sensorIndex == VideoProfileMax)
            {
                continue;
            }
            rs2_stream stream = RS2_STREAM_FISHEYE; //TM2_API provides only fisheye video streams
            platform::stream_profile p = { tm_profile.profile.width, tm_profile.profile.height, tm_profile.fps, static_cast<uint32_t>(tm_profile.profile.pixelFormat) };
            auto profile = std::make_shared<video_stream_profile>(p);
            profile->set_dims(p.width, p.height);
            profile->set_stream_type(stream);
            //TM2_API video sensor index represents a fisheye virtual sensor.
            profile->set_stream_index(tm_profile.sensorIndex + 1);  // for nice presentation by the viewer - add 1 to stream index
            profile->set_format(convertTm2PixelFormat(tm_profile.profile.pixelFormat));
            profile->set_framerate(p.fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            if (tm_profile.sensorIndex == 0 || tm_profile.sensorIndex == 1)
            {
                profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT | profile_tag::PROFILE_TAG_SUPERSET);
            }
            stream_profile sp = { profile->get_format(), stream, profile->get_stream_index(), p.width, p.height, p.fps };
            auto intrinsics = get_intrinsics(sp);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            profile_map[SET_SENSOR_ID(SensorType::Fisheye, profile->get_stream_index() - 1)] = profile;
            if (tm_profile.sensorIndex <= 1) results.push_back(profile);

            //TODO - need to register to have resolve_requests work
            //native_pixel_format pf;
            //register_pixel_format(pf);
        }

        //TM2_API - the following profiles follow the convention:
        // index 0 - HMD, index 1 - controller 1, index 2 - controller 2

        //extract gyro profiles
        std::vector<TrackingData::GyroProfile> gyro_profiles(_tm_supported_profiles.gyro, _tm_supported_profiles.gyro + GyroProfileMax);
        for (auto tm_profile : gyro_profiles)
        {
            rs2_format format = RS2_FORMAT_MOTION_XYZ32F;
            auto profile = std::make_shared<motion_stream_profile>(platform::stream_profile{ 0, 0, tm_profile.fps, static_cast<uint32_t>(format) });
            profile->set_stream_type(RS2_STREAM_GYRO);
            profile->set_stream_index(tm_profile.sensorIndex); // for nice presentation by the viewer - add 1 to stream index
            profile->set_format(format);
            profile->set_framerate(tm_profile.fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            auto intrinsics = get_motion_intrinsics(*profile);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            if (tm_profile.sensorIndex == 0)
            {
                profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT | profile_tag::PROFILE_TAG_SUPERSET);
                results.push_back(profile);
                profile_map[SET_SENSOR_ID(SensorType::Gyro, profile->get_stream_index())] = profile;
            }
        }

        //extract accelerometer profiles
        std::vector<TrackingData::AccelerometerProfile> accel_profiles(_tm_supported_profiles.accelerometer, _tm_supported_profiles.accelerometer + AccelerometerProfileMax);
        for (auto tm_profile : accel_profiles)
        {
            rs2_format format = RS2_FORMAT_MOTION_XYZ32F;
            auto profile = std::make_shared<motion_stream_profile>(platform::stream_profile{ 0, 0, tm_profile.fps, static_cast<uint32_t>(format) });
            profile->set_stream_type(RS2_STREAM_ACCEL);
            profile->set_stream_index(tm_profile.sensorIndex); // for nice presentation by the viewer - add 1 to stream index
            profile->set_format(format);
            profile->set_framerate(tm_profile.fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            auto intrinsics = get_motion_intrinsics(*profile);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            if (tm_profile.sensorIndex == 0)
            {
                profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT | profile_tag::PROFILE_TAG_SUPERSET);
                results.push_back(profile);
                profile_map[SET_SENSOR_ID(SensorType::Accelerometer, profile->get_stream_index())] = profile;
            }
        }

        //extract 6Dof profiles - TODO
        std::vector<TrackingData::SixDofProfile> pose_profiles(_tm_supported_profiles.sixDof, _tm_supported_profiles.sixDof + SixDofProfileMax);
        for (auto tm_profile : pose_profiles)
        {
            rs2_format format = RS2_FORMAT_6DOF;
            uint32_t fps = convertTm2InterruptRate(tm_profile.interruptRate);
            if (fps == 0)
            {
                //pose stream is calculated but not streamed. can be queried by the host
                continue;
            }
            auto profile = std::make_shared<pose_stream_profile>(platform::stream_profile{ 0, 0, fps, static_cast<uint32_t>(format) });
            profile->set_stream_type(RS2_STREAM_POSE);
            // TM2_API - 6dof profile type enum behaves the same as stream index
            profile->set_stream_index(static_cast<int32_t>(tm_profile.profileType));
            profile->set_format(format);
            profile->set_framerate(fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            if (tm_profile.profileType == SixDofProfile0)
            {
                profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT | profile_tag::PROFILE_TAG_SUPERSET);
                results.push_back(profile);
                profile_map[SET_SENSOR_ID(SensorType::Pose, profile->get_stream_index())] = profile;
            }
            //TODO - do I need to define native_pixel_format for RS2_STREAM_POSE? and how to draw it?
        }

        //add extrinic parameters
        for (auto profile : results)
        {
            SensorId current_reference;
            auto current_extrinsics = get_extrinsics(*profile, current_reference);
            environment::get_instance().get_extrinsics_graph().register_extrinsics(*profile, *(profile_map[current_reference]), current_extrinsics);
        }

        auto accel_it = std::find_if(results.begin(), results.end(),
            [](std::shared_ptr<stream_profile_interface> spi) { return RS2_STREAM_ACCEL == spi->get_stream_type(); });
        auto gyro_it = std::find_if(results.begin(), results.end(),
            [](std::shared_ptr<stream_profile_interface> spi) { return RS2_STREAM_GYRO == spi->get_stream_type(); });
        if ((accel_it != results.end()) && (gyro_it != results.end()))
            environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*(accel_it->get()), *(gyro_it->get()));

        return results;
    }

    void tm2_sensor::open(const stream_profiles& requests)
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("open(...) failed. TM2 device is streaming!");
        else if (_is_opened)
            throw wrong_api_call_sequence_exception("open(...) failed. TM2 device is already opened!");

        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());

        //TODO - TM2_API currently supports a single profile is supported per stream. 
        // this function uses stream index to determine stream profile, and just validates the requested profile.
        // need to fix this to search among supported profiles per stream. ( use sensor_base::resolve_requests?)
        if (_loopback)
        {
            auto& loopback_sensor = _loopback->get_sensor(0);
            //TODO: Future work: filter out raw streams according to requested output.
            loopback_sensor.open(loopback_sensor.get_stream_profiles());
            _tm_active_profiles.playbackEnabled = true;
            for (auto&& r : loopback_sensor.get_stream_profiles())
            {
                auto sp = to_profile(r.get());
                int stream_index = sp.index - 1;

                switch (r->get_stream_type())
                {
                case RS2_STREAM_FISHEYE:
                {
                    //TODO: check bound for _tm_supported_profiles.___[]

                    auto tm_profile = _tm_supported_profiles.video[stream_index];
                    if (tm_profile.fps == sp.fps &&
                        tm_profile.profile.height == sp.height &&
                        tm_profile.profile.width == sp.width &&
                        tm_profile.profile.pixelFormat == convertToTm2PixelFormat(sp.format))
                    {
                        _tm_active_profiles.set(tm_profile, true, false);

                        // need to set both L & R streams profile, due to TM2 FW limitation
                        //int pair_stream_index = (stream_index % 2) == 0 ? stream_index + 1 : stream_index - 1;
                        //auto tm_profile_pair = _tm_supported_profiles.video[pair_stream_index];
                        //// no need to get the pair stream output, but make sure not to disable a previously enabled stream
                        //_tm_active_profiles.set(tm_profile_pair, true, _tm_active_profiles.video[pair_stream_index].outputEnabled);
                    }
                    break;
                }
                case RS2_STREAM_GYRO:
                {
                    auto tm_profile = _tm_supported_profiles.gyro[stream_index];
                    if (tm_profile.fps == sp.fps)
                    {
                        _tm_active_profiles.set(tm_profile, true, false);
                    }
                    break;
                }
                case RS2_STREAM_ACCEL:
                {
                    auto tm_profile = _tm_supported_profiles.accelerometer[stream_index];
                    if (tm_profile.fps == sp.fps)
                    {
                        _tm_active_profiles.set(tm_profile, true, false);
                    }
                    break;
                }
                case RS2_STREAM_POSE:
                {
                    break; //ignored - not a raw stream
                }
                default:
                    throw invalid_value_exception("Invalid stream type");
                }
            }
        }
        for (auto&& r : requests)
        {
            auto sp = to_profile(r.get());
            int stream_index = sp.index;
            
            switch (r->get_stream_type())
            {
            case RS2_STREAM_FISHEYE:
            {
                if(stream_index != 1 && stream_index != 2) {
                    throw invalid_value_exception("Invalid stream index, must be 1 or 2");
                }
                stream_index -= 1; // for multiple streams, the index starts from 1
                //TODO: check bound for _tm_supported_profiles.___[]
                auto tm_profile = _tm_supported_profiles.video[stream_index];
                if (tm_profile.fps == sp.fps &&
                    tm_profile.profile.height == sp.height &&
                    tm_profile.profile.width == sp.width &&
                    tm_profile.profile.pixelFormat == convertToTm2PixelFormat(sp.format))
                {
                    _tm_active_profiles.set(tm_profile, true, true);

                    // need to set both L & R streams profile, due to TM2 FW limitation
                    int pair_stream_index = (stream_index % 2) == 0 ? stream_index + 1 : stream_index - 1;
                    auto tm_profile_pair = _tm_supported_profiles.video[pair_stream_index];
                    // no need to get the pair stream output, but make sure not to disable a previously enabled stream
                    _tm_active_profiles.set(tm_profile_pair, true, _tm_active_profiles.video[pair_stream_index].outputEnabled);
                }
                break;
            }
            case RS2_STREAM_GYRO:
            {
                auto tm_profile = _tm_supported_profiles.gyro[stream_index];
                if (tm_profile.fps == sp.fps)
                {
                    _tm_active_profiles.set(tm_profile, true, true);
                }
                break;
            }
            case RS2_STREAM_ACCEL:
            {
                auto tm_profile = _tm_supported_profiles.accelerometer[stream_index];
                if (tm_profile.fps == sp.fps)
                {
                    _tm_active_profiles.set(tm_profile, true, true);
                }
                break;
            }
            case RS2_STREAM_POSE:
            {
                auto tm_profile = _tm_supported_profiles.sixDof[stream_index];
                tm_profile.mode = _tm_mode;
                if (convertTm2InterruptRate(tm_profile.interruptRate) == sp.fps)
                {
                    _tm_active_profiles.set(tm_profile, true);
                }
                break;
            }
            default:
                throw invalid_value_exception("Invalid stream type");
            }
        }

        if (_tm_active_profiles.video[0].enabled == _tm_active_profiles.video[1].enabled &&
            _tm_active_profiles.video[0].outputEnabled != _tm_active_profiles.video[1].outputEnabled)
        {
            throw invalid_value_exception("Invalid profile configuration - setting a single FE stream is not supported");
        }

        _is_opened = true;
        set_active_streams(requests);
    }

    void tm2_sensor::close()
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. TM2 device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. TM2 device was not opened!");

        if (_loopback)
        {
            auto& loopback_sensor = _loopback->get_sensor(0);
            loopback_sensor.close();
        }
        //reset active profiles
        _tm_active_profiles.reset();

        _is_opened = false;
        set_active_streams({});

        manual_exposure = 0;
    }

    void tm2_sensor::pass_frames_to_fw(frame_holder fref)
    {
        auto to_nanos = [](rs2_time_t t) { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<rs2_time_t, std::milli>(t)).count(); };
        auto frame_ptr = fref.frame;
        auto get_md_or_default = [frame_ptr](rs2_frame_metadata_value md) {
            rs2_metadata_type v = 0;
            if (frame_ptr->supports_frame_metadata(md))
                v = frame_ptr->get_frame_metadata(md);
            return v;
        };


        int stream_index = fref.frame->get_stream()->get_stream_index() - 1;
        auto lrs_stream = fref.frame->get_stream()->get_stream_type();
        SensorType tm_stream;
        if (!try_convert(lrs_stream, tm_stream))
        {
            LOG_ERROR("Failed to convert lrs stream " << lrs_stream << " to tm stream");
            return;
        }

        //Pass frames to firmware
        if (auto vframe = As<video_frame>(fref.frame))
        {
            TrackingData::VideoFrame f = {};
            f.sensorIndex = stream_index;
            //TODO: Librealsense frame numbers are 64 bit. This is a potentional interface gap
            f.frameId = vframe->additional_data.frame_number;
            f.profile.set(vframe->get_width(), vframe->get_height(), vframe->get_stride(), convertToTm2PixelFormat(vframe->get_stream()->get_format()));
            f.exposuretime = get_md_or_default(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            f.frameLength = vframe->get_height()*vframe->get_stride()* (vframe->get_bpp() / 8);
            f.data = vframe->data.data();
            f.timestamp = to_nanos(vframe->additional_data.timestamp);
            f.systemTimestamp = to_nanos(vframe->additional_data.backend_timestamp);
            f.arrivalTimeStamp = to_nanos(vframe->additional_data.system_time);
            auto sts = _tm_dev->SendFrame(f);
            if (sts != Status::SUCCESS)
            {
                LOG_ERROR("Failed to send video frame to TM");
            }
        }
        else if (auto mframe = As<motion_frame>(fref.frame))
        {
            auto st = mframe->get_stream()->get_stream_type();
            if (st == RS2_STREAM_ACCEL)
            {
                TrackingData::AccelerometerFrame f{};
                auto mdata = reinterpret_cast<float*>(mframe->data.data());
                f.acceleration.set(mdata[0], mdata[1], mdata[2]);
                f.frameId = mframe->additional_data.frame_number;
                f.sensorIndex = stream_index;
                f.temperature = get_md_or_default(RS2_FRAME_METADATA_TEMPERATURE);
                f.timestamp = to_nanos(mframe->additional_data.timestamp);
                f.systemTimestamp = to_nanos(mframe->additional_data.backend_timestamp);
                f.arrivalTimeStamp = to_nanos(mframe->additional_data.system_time);
                auto sts = _tm_dev->SendFrame(f);
                if (sts != Status::SUCCESS)
                {
                    LOG_ERROR("Failed to send video frame to TM");
                }
            }
            else if(st == RS2_STREAM_GYRO)
            {
                TrackingData::GyroFrame f{};
                auto mdata = reinterpret_cast<float*>(mframe->data.data());
                f.angularVelocity.set(mdata[0], mdata[1], mdata[2]);
                f.frameId = mframe->additional_data.frame_number;
                f.sensorIndex = stream_index;
                f.temperature = get_md_or_default(RS2_FRAME_METADATA_TEMPERATURE);
                f.timestamp = to_nanos(mframe->additional_data.timestamp);
                f.systemTimestamp = to_nanos(mframe->additional_data.backend_timestamp);
                f.arrivalTimeStamp = to_nanos(mframe->additional_data.system_time);
                auto sts = _tm_dev->SendFrame(f);
                if (sts != Status::SUCCESS)
                {
                    LOG_ERROR("Failed to send video frame to TM");
                }
            }
            else if (auto mframe = As<pose_frame>(fref.frame))
            {
                //Ignore
            }
            else
            {
                LOG_ERROR("Unhandled motion frame type");
            }
        }
        else
        {
            LOG_ERROR("Unhandled frame type");
        }
    }
    void tm2_sensor::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device is already streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device was not opened!");

        _dispatcher.start();
        _source.set_callback(callback);
        raise_on_before_streaming_changes(true);
        auto status = _tm_dev->Start(this, &_tm_active_profiles);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to start TM2 camera");
        }
        if (_loopback)
        {
            auto& loopback_sensor = _loopback->get_sensor(0);
            auto handle_file_frames = [&](frame_holder fref)
            {
                pass_frames_to_fw(std::move(fref));
            };

            frame_callback_ptr file_frames_callback = { new internal_frame_callback<decltype(handle_file_frames)>(handle_file_frames),
                [](rs2_frame_callback* p) { p->release(); } };
            loopback_sensor.start(file_frames_callback);
        }

        _is_streaming = true;
    }

    void tm2_sensor::stop()
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. TM2 device is not streaming!");
        
        _dispatcher.stop();
        
        if (_loopback)
        {
            auto& loopback_sensor = _loopback->get_sensor(0);
            loopback_sensor.stop();
        }
        auto status = _tm_dev->Stop();
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to stop TM2 camera");
        }
        raise_on_before_streaming_changes(false);
        _is_streaming = false;
    }

    rs2_intrinsics tm2_sensor::get_intrinsics(const stream_profile& profile) const
    {
        rs2_intrinsics result;
        TrackingData::CameraIntrinsics tm_intrinsics{};
        int stream_index = profile.index - 1;

        auto status = _tm_dev->GetCameraIntrinsics(SET_SENSOR_ID(SensorType::Fisheye,stream_index), tm_intrinsics);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to read TM2 intrinsics");
        }

        result.width = tm_intrinsics.width;
        result.height = tm_intrinsics.height;
        result.ppx = tm_intrinsics.ppx;
        result.ppy = tm_intrinsics.ppy;
        result.fx = tm_intrinsics.fx;
        result.fy = tm_intrinsics.fy;
        result.model = convertTm2CameraModel(tm_intrinsics.distortionModel);
        librealsense::copy_array<true>(result.coeffs, tm_intrinsics.coeffs);
        return result;
    }

    rs2_motion_device_intrinsic tm2_sensor::get_motion_intrinsics(const motion_stream_profile_interface& profile) const
    {
        rs2_motion_device_intrinsic result{};
        TrackingData::MotionIntrinsics tm_intrinsics{};
        int stream_index = profile.get_stream_index();
        if (stream_index != 0) //firmware only accepts stream 0
        {
            return result;
        }
        SensorType type = SensorType::Max;
        switch (profile.get_stream_type())
        {
        case RS2_STREAM_ACCEL:
            type = SensorType::Accelerometer;
            break;
        case RS2_STREAM_GYRO:
            type = SensorType::Gyro;
            break;
        default:
            throw invalid_value_exception("Invalid motion stream type");
        }
        auto status = _tm_dev->GetMotionModuleIntrinsics(SET_SENSOR_ID(type, stream_index), tm_intrinsics);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to read TM2 intrinsics");
        }
        librealsense::copy_2darray<true>(result.data, tm_intrinsics.data);
        librealsense::copy_array<true>(result.noise_variances, tm_intrinsics.noiseVariances);
        librealsense::copy_array<true>(result.bias_variances, tm_intrinsics.biasVariances);

        return result;
    }

    rs2_extrinsics tm2_sensor::get_extrinsics(const stream_profile_interface & profile, perc::SensorId & reference_sensor_id) const
    {

        rs2_extrinsics result{};
        TrackingData::SensorExtrinsics tm_extrinsics{};
        int stream_index = profile.get_stream_index();
        SensorType type = SensorType::Max;
        switch (profile.get_stream_type())
        {
        case RS2_STREAM_FISHEYE:
            type = SensorType::Fisheye;
            break;
        case RS2_STREAM_ACCEL:
            type = SensorType::Accelerometer;
            break;
        case RS2_STREAM_GYRO:
            type = SensorType::Gyro;
            break;
        case RS2_STREAM_POSE:
            type = SensorType::Pose;
            break;
        default:
            throw invalid_value_exception("Invalid stream type");
        }

        if (type == SensorType::Fisheye)
        {
            stream_index--;
        }

        auto status = _tm_dev->GetExtrinsics(SET_SENSOR_ID(type, stream_index), tm_extrinsics);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to read TM2 intrinsics");
        }

        librealsense::copy_array<true>(result.rotation, tm_extrinsics.rotation);
        librealsense::copy_array<true>(result.translation, tm_extrinsics.translation);
        reference_sensor_id = tm_extrinsics.referenceSensorId;

        return result;
    }

    void tm2_sensor::set_intrinsics(const stream_profile_interface& stream_profile, const rs2_intrinsics& intr)
    {
        auto convertCameraModelTm2 = [](const rs2_distortion& model) {
            switch (model) {
            case RS2_DISTORTION_FTHETA: return 1;
            case RS2_DISTORTION_NONE:   return 3;
            case RS2_DISTORTION_KANNALA_BRANDT4: return 4;
            default:
                throw invalid_value_exception("Invalid TM2 camera model");
            }
        };

        perc::TrackingData::CameraIntrinsics tm2_intrinsics;
        tm2_intrinsics.width = intr.width;
        tm2_intrinsics.height = intr.height;
        tm2_intrinsics.fx = intr.fx;
        tm2_intrinsics.fy = intr.fy;
        tm2_intrinsics.ppx = intr.ppx;
        tm2_intrinsics.ppy = intr.ppy;
        tm2_intrinsics.distortionModel = convertCameraModelTm2(intr.model);
        librealsense::copy_array(tm2_intrinsics.coeffs, intr.coeffs);

        auto status = _tm_dev->SetCameraIntrinsics(SET_SENSOR_ID(SensorType::Fisheye, stream_profile.get_stream_index() - 1), tm2_intrinsics);
        if (status != perc::Status::SUCCESS) {
            throw io_exception(to_string() << "Error T2xx set intrinsics, status = " << (uint32_t)status);
        }
    }

    void tm2_sensor::set_extrinsics(const stream_profile_interface& from_profile, const stream_profile_interface& to_profile, const rs2_extrinsics& extr)
    {
        switch (to_profile.get_stream_type()) {
        case RS2_STREAM_POSE:
            set_extrinsics_to_ref(from_profile.get_stream_type(), from_profile.get_stream_index(), extr);
            break;
        case RS2_STREAM_FISHEYE:
        {
            auto inv = [](const rs2_extrinsics& src) {
                rs2_extrinsics dst; 
                auto dst_rotation = dst.rotation;
                for (auto i : { 0,3,6,1,4,7,2,5,8 }) { *dst_rotation++ = src.rotation[i]; }
                dst.translation[0] = - src.rotation[0] * src.translation[0] - src.rotation[3] * src.translation[1] - src.rotation[6] * src.translation[2];
                dst.translation[1] = - src.rotation[1] * src.translation[0] - src.rotation[4] * src.translation[1] - src.rotation[7] * src.translation[2];
                dst.translation[2] = - src.rotation[2] * src.translation[0] - src.rotation[5] * src.translation[1] - src.rotation[8] * src.translation[2];
                return dst;
            };

            auto mult = [](const rs2_extrinsics& a, const rs2_extrinsics& b) {
                rs2_extrinsics tf;
                tf.rotation[0] = a.rotation[0] * b.rotation[0] + a.rotation[1] * b.rotation[3] + a.rotation[2] * b.rotation[6];
                tf.rotation[1] = a.rotation[0] * b.rotation[1] + a.rotation[1] * b.rotation[4] + a.rotation[2] * b.rotation[7];
                tf.rotation[2] = a.rotation[0] * b.rotation[2] + a.rotation[1] * b.rotation[5] + a.rotation[2] * b.rotation[8];
                tf.rotation[3] = a.rotation[3] * b.rotation[0] + a.rotation[4] * b.rotation[3] + a.rotation[5] * b.rotation[6];
                tf.rotation[4] = a.rotation[3] * b.rotation[1] + a.rotation[4] * b.rotation[4] + a.rotation[5] * b.rotation[7];
                tf.rotation[5] = a.rotation[3] * b.rotation[2] + a.rotation[4] * b.rotation[5] + a.rotation[5] * b.rotation[8];
                tf.rotation[6] = a.rotation[6] * b.rotation[0] + a.rotation[7] * b.rotation[3] + a.rotation[8] * b.rotation[6];
                tf.rotation[7] = a.rotation[6] * b.rotation[1] + a.rotation[7] * b.rotation[4] + a.rotation[8] * b.rotation[7];
                tf.rotation[8] = a.rotation[6] * b.rotation[2] + a.rotation[7] * b.rotation[5] + a.rotation[8] * b.rotation[8];
                tf.translation[0] = a.rotation[0] * b.translation[0] + a.rotation[1] * b.translation[1] + a.rotation[2] * b.translation[2] + a.translation[0];
                tf.translation[1] = a.rotation[3] * b.translation[0] + a.rotation[4] * b.translation[1] + a.rotation[5] * b.translation[2] + a.translation[1];
                tf.translation[2] = a.rotation[6] * b.translation[0] + a.rotation[7] * b.translation[1] + a.rotation[8] * b.translation[2] + a.translation[2];
                return tf;
            };

            perc::SensorId reference_sensor_id;
            auto& H_fe1_fe2 = extr;
            auto H_fe2_fe1  = inv(H_fe1_fe2);
            auto H_fe1_pose = get_extrinsics(from_profile, reference_sensor_id);
            auto H_fe2_pose = mult(H_fe2_fe1, H_fe1_pose);  //H_fe2_pose = H_fe2_fe1 * H_fe1_pose
            set_extrinsics_to_ref(RS2_STREAM_FISHEYE, 2, H_fe2_pose);
            break;
        }
        default:
            throw invalid_value_exception("Invalid stream type: set_extrinsics only support fisheye stream");
        }
    }

    void tm2_sensor::set_extrinsics_to_ref(rs2_stream stream_type, int stream_index, const rs2_extrinsics& extr)
    {
        SensorType type = SensorType::Max;
        switch (stream_type)
        {
        case RS2_STREAM_FISHEYE:
            type = SensorType::Fisheye;
            break;
        case RS2_STREAM_ACCEL:
            type = SensorType::Accelerometer;
            break;
        case RS2_STREAM_GYRO:
            type = SensorType::Gyro;
            break;
        case RS2_STREAM_POSE:
            type = SensorType::Pose;
            break;
        default:
            throw invalid_value_exception("Invalid stream type");
        }

        if (type == SensorType::Fisheye)
        {
            stream_index--;
        }

        perc::TrackingData::SensorExtrinsics tm2_extrinsics;
        tm2_extrinsics.referenceSensorId = -1; //TODO: figure out default common reference sensor id defined internally.
        librealsense::copy_array(tm2_extrinsics.rotation, extr.rotation);
        librealsense::copy_array(tm2_extrinsics.translation, extr.translation);

        auto status = _tm_dev->SetExtrinsics(SET_SENSOR_ID(type, stream_index), tm2_extrinsics);
        if (status != perc::Status::SUCCESS) {
            throw io_exception(to_string() << "Error in T2xx set extrinsics, status = " << (uint32_t)status);
        }
    }

    void tm2_sensor::set_motion_device_intrinsics(const stream_profile_interface& stream_profile, const rs2_motion_device_intrinsic& intr)
    {
        if (stream_profile.get_stream_index() != 0) {
            throw invalid_value_exception("Invalid stream index");
        }
        SensorType type = SensorType::Max;
        switch (stream_profile.get_stream_type()) {
        case RS2_STREAM_GYRO: type = SensorType::Gyro; break;
        case RS2_STREAM_ACCEL: type = SensorType::Accelerometer; break;
        default:
            throw invalid_value_exception("Invalid stream type");
        }

        perc::TrackingData::MotionIntrinsics tm2_motion_intrinsics;
        librealsense::copy_2darray(tm2_motion_intrinsics.data, intr.data);
        librealsense::copy_array(tm2_motion_intrinsics.biasVariances, intr.bias_variances);
        librealsense::copy_array(tm2_motion_intrinsics.noiseVariances, intr.noise_variances);

        auto status = _tm_dev->SetMotionModuleIntrinsics(SET_SENSOR_ID(type, 0), tm2_motion_intrinsics);
        if (status != perc::Status::SUCCESS) {
            throw io_exception(to_string() << "Error in T2xx set motion device intrinsics, status = " << (uint32_t)status);
        }
    }

    void tm2_sensor::write_calibration()
    {
        auto status = _tm_dev->WriteConfiguration(ID_OEM_CAL, 0, nullptr);
        if (status != perc::Status::SUCCESS) {
            throw io_exception(to_string() << "Error T2xx set motion device intrinsics, status = " << (uint32_t)status);
        }
    }

    void tm2_sensor::reset_to_factory_calibration()
    {
        auto status = _tm_dev->DeleteConfiguration(ID_OEM_CAL);
        switch(status){
            case perc::Status::SUCCESS:
                break;
            case perc::Status::TABLE_NOT_EXIST:
                LOG_WARNING("Warning, T2xx has already been using factory calibration, status = TABLE_NOT_EXIST");
                break;
            default:
                throw io_exception(to_string() << "Error in T2xx reset to factory calibration, status = " << (uint32_t)status);
        }
    }


    // Tracking listener
    ////////////////////
    void tm2_sensor::onVideoFrame(perc::TrackingData::VideoFrame& tm_frame)
    {
        if (!_is_streaming)
        {
            LOG_WARNING("Frame received with streaming inactive");
            return;
        }
        if (tm_frame.frameLength == 0)
        {
            LOG_WARNING("Dropped frame. Frame length is 0");
            return;
        }

        auto bpp = get_image_bpp(convertTm2PixelFormat(tm_frame.profile.pixelFormat));
        auto ts_double_nanos = duration<double, std::nano>(tm_frame.timestamp);
        duration<double, std::milli> ts_ms(ts_double_nanos);
        auto sys_ts_double_nanos = duration<double, std::nano>(tm_frame.systemTimestamp);
        duration<double, std::milli> system_ts_ms(sys_ts_double_nanos);
        auto arr_ts_double_nanos = duration<double, std::nano>(tm_frame.arrivalTimeStamp);
        duration<double, std::milli> arrival_ts_ms(arr_ts_double_nanos);
        video_frame_metadata video_md{};
        video_md.arrival_ts = tm_frame.arrivalTimeStamp;
        video_md.exposure_time = tm_frame.exposuretime;

        last_exposure = tm_frame.exposuretime;
        last_gain = tm_frame.gain;

        frame_additional_data additional_data(ts_ms.count(), tm_frame.frameId, arrival_ts_ms.count(), sizeof(video_md), (uint8_t*)&video_md, system_ts_ms.count(), 0 ,0, false);

        // Find the frame stream profile
        std::shared_ptr<stream_profile_interface> profile = nullptr;
        auto profiles = get_stream_profiles();
        for (auto&& p : profiles)
        {
            if (p->get_stream_type() == RS2_STREAM_FISHEYE && //TM2_API - all video are fisheye 
                p->get_stream_index() == tm_frame.sensorIndex + 1 &&
                p->get_format() == convertTm2PixelFormat(tm_frame.profile.pixelFormat))
            {
                auto video = dynamic_cast<video_stream_profile_interface*>(p.get()); //this must succeed for fisheye stream
                if (video->get_width() == tm_frame.profile.width &&
                    video->get_height() == tm_frame.profile.height)
                {
                    profile = p;
                    break;
                }
            }
        }
        if (profile == nullptr)
        {
            LOG_WARNING("Dropped frame. No valid profile");
            return;
        }
        //TODO - extension_type param assumes not depth
        frame_holder frame = _source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, tm_frame.profile.height * tm_frame.profile.stride, additional_data, true);
        if (frame.frame)
        {
            auto video = (video_frame*)(frame.frame);
            video->assign(tm_frame.profile.width, tm_frame.profile.height, tm_frame.profile.stride, bpp);
            frame->set_timestamp(system_ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME);
            frame->set_stream(profile);
            frame->set_sensor(this->shared_from_this()); //TODO? uvc doesn't set it?
            video->data.assign(tm_frame.data, tm_frame.data + (tm_frame.profile.height * tm_frame.profile.stride));
        }
        else
        {
            LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
            return;
        }
        _source.invoke_callback(std::move(frame));
    }

    void tm2_sensor::onAccelerometerFrame(perc::TrackingData::AccelerometerFrame& tm_frame)
    {
        if (!_is_streaming)
        {
            LOG_WARNING("Frame received with streaming inactive");
            return;
        }

        float3 data = { tm_frame.acceleration.x, tm_frame.acceleration.y, tm_frame.acceleration.z };
        handle_imu_frame(tm_frame, tm_frame.frameId, RS2_STREAM_ACCEL, tm_frame.sensorIndex, data, tm_frame.temperature);
    }

    void tm2_sensor::onGyroFrame(perc::TrackingData::GyroFrame& tm_frame)
    {
        if (!_is_streaming)
        {
            LOG_WARNING("Frame received with streaming inactive");
            return;
        }
        float3 data = { tm_frame.angularVelocity.x, tm_frame.angularVelocity.y, tm_frame.angularVelocity.z };
        handle_imu_frame(tm_frame, tm_frame.frameId, RS2_STREAM_GYRO, tm_frame.sensorIndex, data, tm_frame.temperature);
    }

    void tm2_sensor::onPoseFrame(perc::TrackingData::PoseFrame& tm_frame)
    {
        if (!_is_streaming)
        {
            LOG_WARNING("Frame received with streaming inactive");
            return;
        }
        static unsigned long long frame_num = 0; //TODO - TM2 doesn't expose frame number for pose

        auto ts_double_nanos = duration<double, std::nano>(tm_frame.timestamp);
        duration<double, std::milli> ts_ms(ts_double_nanos);
        auto sys_ts_double_nanos = duration<double, std::nano>(tm_frame.systemTimestamp);
        duration<double, std::milli> system_ts_ms(sys_ts_double_nanos);
        auto arr_ts_double_nanos = duration<double, std::nano>(tm_frame.arrivalTimeStamp);
        duration<double, std::milli> arrival_ts_ms(arr_ts_double_nanos);
        pose_frame_metadata frame_md = { 0 };
        frame_md.arrival_ts = tm_frame.arrivalTimeStamp;

        frame_additional_data additional_data(ts_ms.count(), frame_num++, arrival_ts_ms.count(), sizeof(frame_md), (uint8_t*)&frame_md, system_ts_ms.count(), 0, 0, false);

        // Find the frame stream profile
        std::shared_ptr<stream_profile_interface> profile = nullptr;
        auto profiles = get_stream_profiles();
        for (auto&& p : profiles)
        {
            //TODO - assuming single profile per motion stream
            if (p->get_stream_type() == RS2_STREAM_POSE &&
                p->get_stream_index() == tm_frame.sourceIndex)
            {
                profile = p;
                break;
            }
        }
        if (profile == nullptr)
        {
            LOG_WARNING("Dropped frame. No valid profile");
            return;
        }

        //TODO - maybe pass a raw data and parse up? do I have to pass any data on the buffer?
        frame_holder frame = _source.alloc_frame(RS2_EXTENSION_POSE_FRAME, sizeof(librealsense::pose_frame::pose_info), additional_data, true);
        if (frame.frame)
        {
            auto pose_frame = static_cast<librealsense::pose_frame*>(frame.frame);
            frame->set_timestamp(system_ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME);
            frame->set_stream(profile);

            auto info = reinterpret_cast<librealsense::pose_frame::pose_info*>(pose_frame->data.data());
            info->translation = toFloat3(tm_frame.translation);
            info->velocity = toFloat3(tm_frame.velocity);
            info->acceleration = toFloat3(tm_frame.acceleration);
            info->rotation = toFloat4(tm_frame.rotation);
            info->angular_velocity = toFloat3(tm_frame.angularVelocity);
            info->angular_acceleration = toFloat3(tm_frame.angularAcceleration);
            info->tracker_confidence = tm_frame.trackerConfidence;
            info->mapper_confidence = tm_frame.mapperConfidence;
        }
        else
        {
            LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
            return;
        }
        _source.invoke_callback(std::move(frame));
    }

    void tm2_sensor::onControllerDiscoveryEventFrame(perc::TrackingData::ControllerDiscoveryEventFrame& frame)
    {
        std::string msg = to_string() << "Controller discovered with MAC " << frame.macAddress;
        raise_hardware_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.controllerId << " disconnected";
        raise_hardware_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerFrame(perc::TrackingData::ControllerFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.sensorIndex << " button ["<< (int)frame.eventId << ", " << (int)frame.instanceId << "]";
        raise_hardware_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerConnectedEventFrame(perc::TrackingData::ControllerConnectedEventFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.controllerId << " connected";
        if (frame.status == perc::Status::SUCCESS)
        {
            raise_hardware_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
        }
        else
        {
            raise_error_notification(to_string() << "Connection to controller " << (int)frame.controllerId << " failed.");
        }
    }

    void tm2_sensor::onLocalizationDataEventFrame(perc::TrackingData::LocalizationDataFrame& frame)
    {
        LOG_DEBUG("T2xx: Loc_data fragment " << frame.chunkIndex  \
            << " size: " << std::dec << frame.length << " status : " << int(frame.status));

        if (Status::SUCCESS == frame.status)
        {
            _async_op_res_buffer.reserve(_async_op_res_buffer.size() + frame.length);
            auto start = (const char*)frame.buffer;
            _async_op_res_buffer.insert(_async_op_res_buffer.end(), start, start + frame.length);
        }
        else
            _async_op_status = _async_fail;

        if (!frame.moreData)
        {
            if (_async_progress == _async_op_status)
                _async_op_status = _async_success;
            _async_op.notify_one();
        }
    }

    void tm2_sensor::onRelocalizationEvent(perc::TrackingData::RelocalizationEvent& evt)
    {
        std::string msg = to_string() << "T2xx: Relocalization occurred. id: " << evt.sessionId <<  ", timestamp: " << double(evt.timestamp*0.000000001) << " sec";
        raise_relocalization_event(msg, evt.timestamp);
    }

    void tm2_sensor::enable_loopback(std::shared_ptr<playback_device> input)
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        if (_is_streaming || _is_opened)
            throw wrong_api_call_sequence_exception("T2xx: Cannot enter loopback mode while device is open or streaming");
        _loopback = input;
    }

    void tm2_sensor::disable_loopback()
    {
        std::lock_guard<std::mutex> lock(_tm_op_lock);
        _loopback.reset();
    }

    bool tm2_sensor::is_loopback_enabled() const
    {
        return _loopback != nullptr;
    }

    void tm2_sensor::handle_imu_frame(perc::TrackingData::TimestampedData& tm_frame_ts, unsigned long long frame_number, rs2_stream stream_type, int index, float3 imu_data, float temperature)
    {
        auto ts_double_nanos = duration<double, std::nano>(tm_frame_ts.timestamp);
        duration<double, std::milli> ts_ms(ts_double_nanos);
        auto sys_ts_double_nanos = duration<double, std::nano>(tm_frame_ts.systemTimestamp);
        duration<double, std::milli> system_ts_ms(sys_ts_double_nanos);
        auto arr_ts_double_nanos = duration<double, std::nano>(tm_frame_ts.arrivalTimeStamp);
        duration<double, std::milli> arrival_ts_ms(arr_ts_double_nanos);
        motion_frame_metadata motion_md{};
        motion_md.arrival_ts = tm_frame_ts.arrivalTimeStamp;
        motion_md.temperature = temperature;

        frame_additional_data additional_data(ts_ms.count(), frame_number, arrival_ts_ms.count(), sizeof(motion_md), (uint8_t*)&motion_md, system_ts_ms.count(), 0, 0, false);

        // Find the frame stream profile
        std::shared_ptr<stream_profile_interface> profile = nullptr;
        auto profiles = get_stream_profiles();
        for (auto&& p : profiles)
        {
            //TODO - assuming single profile per motion stream
            if (p->get_stream_type() == stream_type &&
                p->get_stream_index() == index)
            {
                profile = p;
                break;
            }
        }
        if (profile == nullptr)
        {
            LOG_WARNING("Dropped frame. No valid profile");
            return;
        }

        frame_holder frame = _source.alloc_frame(RS2_EXTENSION_MOTION_FRAME, 3 * sizeof(float), additional_data, true);
        if (frame.frame)
        {
            auto motion_frame = static_cast<librealsense::motion_frame*>(frame.frame);
            frame->set_timestamp(system_ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME);
            frame->set_stream(profile);
            auto data = reinterpret_cast<float*>(motion_frame->data.data());
            data[0] = imu_data[0];
            data[1] = imu_data[1];
            data[2] = imu_data[2];
        }
        else
        {
            LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
            return;
        }
        _source.invoke_callback(std::move(frame));
    }

    void tm2_sensor::raise_relocalization_event(const std::string& msg, double timestamp)
    {
        notification event{ RS2_NOTIFICATION_CATEGORY_POSE_RELOCALIZATION, 0, RS2_LOG_SEVERITY_INFO, msg };
        event.timestamp = timestamp;
        get_notifications_processor()->raise_notification(event);
    }

    void tm2_sensor::raise_hardware_event(const std::string& msg, const std::string& json_data, double timestamp)
    {
        notification controller_event{ RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT, 0, RS2_LOG_SEVERITY_INFO, msg };
        controller_event.serialized_data = json_data;
        controller_event.timestamp = timestamp;
        get_notifications_processor()->raise_notification(controller_event);
    }

    void tm2_sensor::raise_error_notification(const std::string& msg)
    {
        notification error{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, 0, RS2_LOG_SEVERITY_ERROR, msg };
        error.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        get_notifications_processor()->raise_notification(error);
    }
    void tm2_sensor::attach_controller(const std::array<uint8_t, 6>& mac_addr)
    {
        perc::TrackingData::ControllerDeviceConnect c(const_cast<uint8_t*>(mac_addr.data()), 15000);
        _dispatcher.invoke([this, c](dispatcher::cancellable_timer ct)
        {
            uint8_t controller_id = 0;
            auto status = _tm_dev->ControllerConnect(c, controller_id);
            if (status != Status::SUCCESS)
            {
                raise_error_notification(to_string() << "Failed to send connect to controller " << c.macAddress);
            }
            else
            {
                LOG_INFO("Successfully sent controller connect to " << buffer_to_string(c.macAddress));
            }
        });
    }

    void tm2_sensor::detach_controller(int id)
    {
        perc::Status status = _tm_dev->ControllerDisconnect(id);
        if (status != Status::SUCCESS)
        {
            raise_error_notification(to_string() << "Failed to disconnect to controller " << id);
        }
        else
        {
            std::string msg = to_string() << "Disconnected from controller #" << id;
            perc::TrackingData::ControllerDisconnectedEventFrame f;
            f.controllerId = id;
            raise_hardware_event(msg, controller_event_serializer::serialized_data(f), std::chrono::high_resolution_clock::now().time_since_epoch().count());
        }
    }

    std::string async_op_to_string(tm2_sensor::async_op_state val)
    {
        switch (val)
        {
            case tm2_sensor::_async_init:       return "Init";
            case tm2_sensor::_async_progress:   return "In Progress";
            case tm2_sensor::_async_success:    return "Success";
            case tm2_sensor::_async_fail:       return "Fail";
            default: return (to_string() << " Unsupported type: " << val);
        }
    }

    tm2_sensor::async_op_state tm2_sensor::perform_async_transfer(std::function<perc::Status()> transfer_activator,
        std::function<void()> on_success, const std::string& op_description) const
    {
        const uint32_t tm2_async_op_timeout_sec = 10;
        async_op_state res = async_op_state::_async_fail;
        auto ret = false;

        std::unique_lock<std::mutex> lock(_tm_op_lock);
        auto stat = transfer_activator();
        if (Status::SUCCESS == stat)
        {
            _async_op_status = _async_progress;
            LOG_INFO(op_description << " in progress");
            ret = _async_op.wait_for(lock, std::chrono::seconds(tm2_async_op_timeout_sec), [&]() { return _async_op_status & (_async_success | _async_fail); });
            if (!ret)
                LOG_WARNING(op_description << " aborted on timeout");
            else
            {
                // Perform user-defined action if operation ends successfully
                if (_async_success == _async_op_status)
                    on_success();

                if (_async_fail == _async_op_status)
                    LOG_ERROR(op_description << " aborted by device");
            }
            res = _async_op_status;
            _async_op_status = _async_init;
            lock.unlock();
            LOG_DEBUG(op_description << " completed with " << async_op_to_string(res));
        }
        else
            LOG_WARNING(op_description << " activation failed, status " << int(stat));

        return (res);
    }

    bool tm2_sensor::export_relocalization_map(std::vector<uint8_t>& lmap_buf) const
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");

        auto res = perform_async_transfer(
            [&]() { _async_op_res_buffer.clear(); return _tm_dev->GetLocalizationData(const_cast<tm2_sensor*>(this)); },
            [&](){ lmap_buf = this->_async_op_res_buffer; },
            "Export localization map");

        if (res != async_op_state::_async_success)
        {
            LOG_ERROR("Export localization map failed");
        }
        return (res == async_op_state::_async_success);
    }

    bool tm2_sensor::import_relocalization_map(const std::vector<uint8_t>& lmap_buf) const
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");

        auto res = perform_async_transfer(
            [&]() { return _tm_dev->SetLocalizationData(const_cast<tm2_sensor*>(this), uint32_t(lmap_buf.size()), lmap_buf.data()); },
            [&]() {}, "Import localization map");

        if (res != async_op_state::_async_success)
        {
            LOG_ERROR("Import localization map failed");
        }
        return (res == async_op_state::_async_success);
    }

    bool tm2_sensor::set_static_node(const std::string& guid, const float3& pos, const float4& orient_quat) const
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");

        perc::TrackingData::RelativePose rp;
        rp.translation  = { pos.x, pos.y, pos.z };
        rp.rotation     = { orient_quat.x, orient_quat.y, orient_quat.z, orient_quat.w };

        auto status = _tm_dev->SetStaticNode(guid.data(), rp);
        if (status == Status::SUCCESS)
            return true;
        if (status == Status::ERROR_FW_INTERNAL) // non-fatal; just means the confidence was not high enough
            return false;

        throw io_exception(to_string() << "Unexpected error setting static node, status = " << (uint32_t)status);
    }

    bool tm2_sensor::get_static_node(const std::string& guid, float3& pos, float4& orient_quat) const
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");

        perc::TrackingData::RelativePose rel_pose;
        auto status = _tm_dev->GetStaticNode(guid.data(), rel_pose);
        if (status == Status::ERROR_FW_INTERNAL) // non-fatal; just means the node is not available
            return false;
        if (status == perc::Status::SUCCESS)
        {
            pos[0] = rel_pose.translation.x;
            pos[1] = rel_pose.translation.y;
            pos[2] = rel_pose.translation.z;
            orient_quat[0] = rel_pose.rotation.i;
            orient_quat[1] = rel_pose.rotation.j;
            orient_quat[2] = rel_pose.rotation.k;
            orient_quat[3] = rel_pose.rotation.r;
            return true;
        }

        throw io_exception(to_string() << "Unexpected error getting static node, status = " << (uint32_t)status);
    }


    bool tm2_sensor::load_wheel_odometery_config(const std::vector<uint8_t>& odometry_config_buf) const
    {
        TrackingData::CalibrationData calibrationData;
        calibrationData.length = uint32_t(odometry_config_buf.size());
        calibrationData.buffer = (uint8_t*)odometry_config_buf.data();
        calibrationData.type = CalibrationTypeAppend;

        auto status = _tm_dev->SetCalibration(calibrationData);
        if (status != Status::SUCCESS)
        {
            LOG_ERROR("T2xx Load Wheel odometry calibration failed, status =" << (uint32_t)status);
        }
        return (status == Status::SUCCESS);
    }

    bool tm2_sensor::send_wheel_odometry(uint8_t wo_sensor_id, uint32_t frame_num, const float3& translational_velocity) const
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");

        perc::TrackingData::VelocimeterFrame vel_fr;
        vel_fr.sensorIndex  = wo_sensor_id;
        vel_fr.frameId      = frame_num;
        vel_fr.translationalVelocity = { translational_velocity.x, translational_velocity.y, translational_velocity.z };

        auto status = _tm_dev->SendFrame(vel_fr);
        if (status != Status::SUCCESS)
        {
            LOG_WARNING("Send Wheel odometry failed, status =" << (uint32_t)status);
        }
        return (status == Status::SUCCESS);
    }

    TrackingData::Temperature tm2_sensor::get_temperature()
    {
        if (!_tm_dev)
            throw wrong_api_call_sequence_exception("T2xx tracking device is not available");
        TrackingData::Temperature temperature;
        auto status = _tm_dev->GetTemperature(temperature);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to query T2xx tracking temperature option");
        }
        return temperature;
    }

    void tm2_sensor::set_exposure(float value)
    {
        if (!manual_exposure)
            throw std::runtime_error("To control exposure you must set sensor to manual exposure mode prior to streaming");
        SetManualExposure(_tm_dev, value, last_gain);
        last_exposure = value;
    }



    float tm2_sensor::get_exposure() const
    {
        return last_exposure;
    }

    void tm2_sensor::set_gain(float value)
    {
        if (!manual_exposure)
            throw std::runtime_error("To control gain you must set sensor to manual exposure mode prior to streaming");
        SetManualExposure(_tm_dev, last_exposure, value);
        last_gain = value;
    }

    float tm2_sensor::get_gain() const
    {
        return last_gain;
    }

    void tm2_sensor::set_manual_exposure(bool manual)
    {
        SetExposureMode(_tm_dev, manual);
        manual_exposure = true;
    }

    ///////////////
    // Device

    tm2_device::tm2_device(
            std::shared_ptr<perc::TrackingManager> manager,
            perc::TrackingDevice* dev,
            std::shared_ptr<context> ctx,
            const platform::backend_device_group& group) :
        device(ctx, group),
        _manager(manager),
        _dev(dev)
    {
        TrackingData::DeviceInfo info;
        auto status = dev->GetDeviceInfo(info);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to get device info");
        }

        std::string vendorIdStr = hexify(info.usbDescriptor.idVendor);
        std::string productIdStr = hexify(info.usbDescriptor.idProduct);

        register_info(RS2_CAMERA_INFO_NAME, tm2_device_name());
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, to_string() << std::hex << (info.serialNumber >> 16));
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, to_string() << info.version.fw.major << "." << info.version.fw.minor << "." << info.version.fw.patch << "." << info.version.fw.build);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, productIdStr);
        register_info(RS2_CAMERA_INFO_PRODUCT_LINE, "T200");

        std::string device_path = std::string("vid_") + vendorIdStr + std::string(" pid_") + productIdStr + std::string(" bus_") + std::to_string(info.usbDescriptor.bus) + std::string(" port_") + std::to_string(info.usbDescriptor.portChain[0]);
        for(int i=1; i<info.usbDescriptor.portChainDepth;i++)
        {
            device_path += "-" + std::to_string(info.usbDescriptor.portChain[i]);
        }
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, device_path);

        _sensor = std::make_shared<tm2_sensor>(this, dev);
        add_sensor(_sensor);

        _sensor->register_option(rs2_option::RS2_OPTION_ASIC_TEMPERATURE, std::make_shared<asic_temperature_option>(*_sensor));
        _sensor->register_option(rs2_option::RS2_OPTION_MOTION_MODULE_TEMPERATURE, std::make_shared<motion_temperature_option>(*_sensor));

        _sensor->register_option(rs2_option::RS2_OPTION_EXPOSURE, std::make_shared<exposure_option>(*_sensor));
        _sensor->register_option(rs2_option::RS2_OPTION_GAIN, std::make_shared<gain_option>(*_sensor));
        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_AUTO_EXPOSURE, std::make_shared<exposure_mode_option>(*_sensor));

        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_MAPPING,             std::make_shared<tracking_mode_option<perc::SIXDOF_MODE_ENABLE_MAPPING,              perc::SIXDOF_MODE_NORMAL,         false>>(*_sensor, "Use an on device map (recommended)"));
        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_RELOCALIZATION,      std::make_shared<tracking_mode_option<perc::SIXDOF_MODE_ENABLE_RELOCALIZATION,       perc::SIXDOF_MODE_ENABLE_MAPPING, false>>(*_sensor, "Use appearance based relocalization (depends on mapping)"));
        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_POSE_JUMPING,        std::make_shared<tracking_mode_option<perc::SIXDOF_MODE_DISABLE_JUMPING,             perc::SIXDOF_MODE_ENABLE_MAPPING,  true>>(*_sensor, "Allow pose jumping (depends on mapping)"));
        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_DYNAMIC_CALIBRATION, std::make_shared<tracking_mode_option<perc::SIXDOF_MODE_DISABLE_DYNAMIC_CALIBRATION, perc::SIXDOF_MODE_NORMAL,          true>>(*_sensor, "Enable dynamic calibration (recommended)"));
        _sensor->register_option(rs2_option::RS2_OPTION_ENABLE_MAP_PRESERVATION,    std::make_shared<tracking_mode_option<perc::SIXDOF_MODE_ENABLE_MAP_PRESERVATION,     perc::SIXDOF_MODE_ENABLE_MAPPING, false>>(*_sensor, "Preserve the map from the previous run as if it was loaded"));

        // Assing the extrinsic nodes to the default group
        auto tm2_profiles = _sensor->get_stream_profiles();
        for (auto && pf : tm2_profiles)
            register_stream_to_extrinsic_group(*pf, 0);

        //For manual testing: enable_loopback("C:\\dev\\recording\\tm2.bag");
    }

    tm2_device::~tm2_device()
    {
        for (auto&& d : get_context()->query_devices(RS2_PRODUCT_LINE_ANY_INTEL))
        {
            for (auto&& tmd : d->get_device_data().tm2_devices)
            {
                if (_dev == tmd.device_ptr)
                {
                    return;
                }
            }
        }
        _sensor->dispose();
    }
    /**
    * Enable loopback will replace the input and ouput of the tm2 sensor
    */
    void tm2_device::enable_loopback(const std::string& source_file)
    {
        auto ctx = get_context();
        std::shared_ptr<playback_device> raw_streams;
        try
        {
            raw_streams = std::make_shared<playback_device>(ctx, std::make_shared<ros_reader>(source_file, ctx));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create playback device from given file. File = \"" << source_file << "\". Exception: " << e.what());
            throw librealsense::invalid_value_exception("Failed to enable loopback");
        }
        _sensor->enable_loopback(raw_streams);
        update_info(RS2_CAMERA_INFO_NAME, to_string() << tm2_device_name() << " (Loopback - " << source_file << ")");
    }

    void tm2_device::disable_loopback()
    {
        _sensor->disable_loopback();
        update_info(RS2_CAMERA_INFO_NAME, tm2_device_name());
    }

    bool tm2_device::is_enabled() const
    {
        return _sensor->is_loopback_enabled();
    }

    void tm2_device::connect_controller(const std::array<uint8_t, 6>& mac_address)
    {
        _sensor->attach_controller(mac_address);
    }

    void tm2_device::disconnect_controller(int id)
    {
        _sensor->detach_controller(id);
    }

    void tm2_device::register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index)
    {
        //TM2 uses POSE sensor as reference sensor for extrinsics
        auto tm2_profiles = _sensor->get_stream_profiles();
        int ref_index = 0;
        for (int i = 0; i < tm2_profiles.size(); i++)
            if (tm2_profiles[i]->get_stream_type() == RS2_STREAM_POSE)
            {
                ref_index = i;
                break;
            }

        _extrinsics[stream.get_unique_id()] = std::make_pair(group_index, tm2_profiles[ref_index]);
    }

}
