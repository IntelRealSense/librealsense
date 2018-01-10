// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

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
                if (auto* vf = dynamic_cast<const video_frame*>(&frm))
                {
                    const video_frame_metadata* md = reinterpret_cast<const video_frame_metadata*>(frm.additional_data.metadata_blob.data());
                    return (rs2_metadata_type)(md->arrival_ts);
                }
                
                if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
                {
                    const motion_frame_metadata* md = reinterpret_cast<const motion_frame_metadata*>(frm.additional_data.metadata_blob.data());
                    return (rs2_metadata_type)(md->arrival_ts);
                }
                if (auto* pf = dynamic_cast<const pose_frame*>(&frm))
                {
                    const pose_frame_metadata* md = reinterpret_cast<const pose_frame_metadata*>(frm.additional_data.metadata_blob.data());
                    return (rs2_metadata_type)(md->arrival_ts);
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
        : sensor_base("Tracking Module", owner), _tm_dev(dev), _dispatcher(10)
    {
        register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_ACTUAL_EXPOSURE));
        register_metadata(RS2_FRAME_METADATA_TEMPERATURE    , std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_TEMPERATURE));
        //Replacing md parser for RS2_FRAME_METADATA_TIME_OF_ARRIVAL
        _metadata_parsers->operator[](RS2_FRAME_METADATA_TIME_OF_ARRIVAL) = std::make_shared<md_tm2_parser>(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);
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
        int uid = 0;

        //get TM2 profiles
        Status status = _tm_dev->GetSupportedProfile(_tm_supported_profiles);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to get supported raw streams");
        }
        //extract video profiles
        std::vector<TrackingData::VideoProfile> video_profiles(_tm_supported_profiles.video, _tm_supported_profiles.video + VideoProfileMax);
        for (auto tm_profile : video_profiles)
        {
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
            if (tm_profile.sensorIndex == 0)
            {
                profile->make_default();
            }
            stream_profile sp = { stream, profile->get_stream_index(), p.width, p.height, p.fps, profile->get_format() };
            auto intrinsics = get_intrinsics(sp);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            results.push_back(profile);

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
            profile->set_stream_index(tm_profile.sensorIndex + 1); // for nice presentation by the viewer - add 1 to stream index
            profile->set_format(format);
            profile->set_framerate(tm_profile.fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            if (tm_profile.sensorIndex == 0)
            {
                profile->make_default();
            }
            auto intrinsics = get_motion_intrinsics(*profile);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            results.push_back(profile);
        }

        //extract accelerometer profiles
        std::vector<TrackingData::AccelerometerProfile> accel_profiles(_tm_supported_profiles.accelerometer, _tm_supported_profiles.accelerometer + AccelerometerProfileMax);
        for (auto tm_profile : accel_profiles)
        {
            rs2_format format = RS2_FORMAT_MOTION_XYZ32F;
            auto profile = std::make_shared<motion_stream_profile>(platform::stream_profile{ 0, 0, tm_profile.fps, static_cast<uint32_t>(format) });
            profile->set_stream_type(RS2_STREAM_ACCEL);
            profile->set_stream_index(tm_profile.sensorIndex + 1); // for nice presentation by the viewer - add 1 to stream index
            profile->set_format(format);
            profile->set_framerate(tm_profile.fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            if (tm_profile.sensorIndex == 0)
            {
                profile->make_default();
            }
            auto intrinsics = get_motion_intrinsics(*profile);
            profile->set_intrinsics([intrinsics]() { return intrinsics; });
            results.push_back(profile);
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
            profile->set_stream_index(static_cast<uint32_t>(tm_profile.profileType) + 1);
            profile->set_format(format);
            profile->set_framerate(fps);
            profile->set_unique_id(environment::get_instance().generate_stream_id());
            if (tm_profile.profileType == SixDofProfile0)
            {
                profile->make_default();
            }
            results.push_back(profile);


            //TODO - do I need to define native_pixel_format for RS2_STREAM_POSE? and how to draw it?
        }

        return results;
    }

    void tm2_sensor::open(const stream_profiles& requests)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
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
        _is_opened = true;
        set_active_streams(requests);
    }

    void tm2_sensor::close()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
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
        auto time_of_arrival = get_md_or_default(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);


        //Pass frames to firmeware
        if (auto vframe = As<video_frame>(fref.frame))
        {
            TrackingData::VideoFrame f = {};
            f.sensorIndex = stream_index;
            f.frameId = vframe->additional_data.frame_number;
            f.profile.set(vframe->get_width(), vframe->get_height(), vframe->get_stride(), convertToTm2PixelFormat(vframe->get_stream()->get_format()));
            f.exposuretime = get_md_or_default(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
            f.frameLength = vframe->get_height()*vframe->get_stride()* (vframe->get_bpp() / 8);
            f.data = vframe->data.data();
            f.timestamp = to_nanos(vframe->additional_data.timestamp);
            f.systemTimestamp = to_nanos(vframe->additional_data.system_time);
            f.arrivalTimeStamp = time_of_arrival;
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
                f.systemTimestamp = to_nanos(mframe->additional_data.system_time);
                f.arrivalTimeStamp = time_of_arrival;
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
                f.systemTimestamp = to_nanos(mframe->additional_data.system_time);
                f.arrivalTimeStamp = time_of_arrival;
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
        std::lock_guard<std::mutex> lock(_configure_lock);
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
        std::lock_guard<std::mutex> lock(_configure_lock);
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
        const TrackingData::CameraIntrinsics tm_intrinsics{};
        int stream_index = profile.index - 1;
        //TODO - wait for TM2 intrinsics impl
        //TODO - assuming IR only
//             auto status = _tm_dev->GetCameraIntrinsics(tm_intrinsics, SET_SENSOR_ID(SensorType::Fisheye,stream_index));
//             if (status != Status::SUCCESS)
//             {
//                 throw io_exception("Failed to read TM2 intrinsics");
//             }

        result.width = tm_intrinsics.width;
        result.height = tm_intrinsics.height;
        result.ppx = tm_intrinsics.ppx;
        result.ppy = tm_intrinsics.ppy;
        result.fx = tm_intrinsics.fx;
        result.fy = tm_intrinsics.fy;
        result.model = convertTm2CameraModel(tm_intrinsics.distortionModel);
        librealsense::copy_array(result.coeffs, tm_intrinsics.coeffs);
        return result;
    }

    rs2_motion_device_intrinsic tm2_sensor::get_motion_intrinsics(const motion_stream_profile_interface& profile) const
    {
        rs2_motion_device_intrinsic result;
        const TrackingData::MotionIntrinsics tm_intrinsics{};
        int stream_index = profile.get_stream_index() - 1;
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
        //auto status = _tm_dev->GetMotionModuleIntrinsics(tm_intrinsics, SET_SENSOR_ID(type, stream_index));
        //             if (status != Status::SUCCESS)
        //             {
        //                 throw io_exception("Failed to read TM2 intrinsics");
        //             }
        librealsense::copy_2darray(result.data, tm_intrinsics.data);
        librealsense::copy_array(result.noise_variances, tm_intrinsics.noiseVariances);
        librealsense::copy_array(result.bias_variances, tm_intrinsics.biasVariances);

        return result;
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
        video_frame_metadata video_md = { 0 };
        video_md.arrival_ts = tm_frame.arrivalTimeStamp;
        video_md.exposure_time = tm_frame.exposuretime;

        frame_additional_data additional_data(ts_ms.count(), tm_frame.frameId, system_ts_ms.count(), sizeof(video_md), (uint8_t*)&video_md);

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
            frame->set_timestamp(ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK);
            frame->set_stream(profile);
            frame->set_sensor(this->shared_from_this()); //TODO? uvc doesn't set it?
            video->data.assign(tm_frame.data, tm_frame.data + (tm_frame.profile.height * tm_frame.profile.stride));
        }
        else
        {
            LOG_WARNING("Dropped frame. alloc_frame(...) returned nullptr");
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
        handle_imu_frame(tm_frame, tm_frame.frameId, RS2_STREAM_ACCEL, tm_frame.sensorIndex + 1, data, tm_frame.temperature);
    }

    void tm2_sensor::onGyroFrame(perc::TrackingData::GyroFrame& tm_frame)
    {
        if (!_is_streaming)
        {
            LOG_WARNING("Frame received with streaming inactive");
            return;
        }
        float3 data = { tm_frame.angularVelocity.x, tm_frame.angularVelocity.y, tm_frame.angularVelocity.z };
        handle_imu_frame(tm_frame, tm_frame.frameId, RS2_STREAM_GYRO, tm_frame.sensorIndex + 1, data, tm_frame.temperature);
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
        pose_frame_metadata frame_md = { 0 };
        frame_md.arrival_ts = tm_frame.arrivalTimeStamp;

        frame_additional_data additional_data(ts_ms.count(), frame_num++, system_ts_ms.count(), sizeof(frame_md), (uint8_t*)&frame_md);

        // Find the frame stream profile
        std::shared_ptr<stream_profile_interface> profile = nullptr;
        auto profiles = get_stream_profiles();
        for (auto&& p : profiles)
        {
            //TODO - assuming single profile per motion stream
            if (p->get_stream_type() == RS2_STREAM_POSE &&
                p->get_stream_index() == tm_frame.sourceIndex + 1)
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
            frame->set_timestamp(ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK);
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
        raise_controller_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.controllerId << " disconnected";
        raise_controller_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerFrame(perc::TrackingData::ControllerFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.sensorIndex << " button ["<< (int)frame.eventId << ", " << (int)frame.instanceId << "]";
        raise_controller_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
    }

    void tm2_sensor::onControllerConnectedEventFrame(perc::TrackingData::ControllerConnectedEventFrame& frame)
    {
        std::string msg = to_string() << "Controller #" << (int)frame.controllerId << " connected";
        if (frame.status == perc::Status::SUCCESS)
        {
            raise_controller_event(msg, controller_event_serializer::serialized_data(frame), frame.timestamp);
        }
        else
        {
            raise_error_notification(to_string() << "Connection to controller " << (int)frame.controllerId << " failed. (Statue: " << get_string(frame.status) << ")");
        }
    }

    void tm2_sensor::enable_loopback(std::shared_ptr<playback_device> input)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming || _is_opened)
            throw wrong_api_call_sequence_exception("Cannot enter loopback mode while device is open or streaming");
        _loopback = input;
    }

    void tm2_sensor::disable_loopback()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
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
        motion_frame_metadata motion_md = { 0 };
        motion_md.arrival_ts = tm_frame_ts.arrivalTimeStamp;
        motion_md.temperature = temperature;

        frame_additional_data additional_data(ts_ms.count(), frame_number, system_ts_ms.count(), sizeof(motion_md), (uint8_t*)&motion_md);

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
            frame->set_timestamp(ts_ms.count());
            frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK);
            frame->set_stream(profile);
            auto data = reinterpret_cast<float*>(motion_frame->data.data());
            data[0] = imu_data[0];
            data[1] = imu_data[1];
            data[2] = imu_data[2];
        }
        else
        {
            LOG_WARNING("Dropped frame. alloc_frame(...) returned nullptr");
            return;
        }
        _source.invoke_callback(std::move(frame));
    }
    
    void tm2_sensor::raise_controller_event(const std::string& msg, const std::string& json_data, double timestamp)
    {
        notification controller_event{ RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT, 0, RS2_LOG_SEVERITY_INFO, msg };
        controller_event.serialized_data = json_data;
        controller_event.timestamp = timestamp;
        get_notifications_proccessor()->raise_notification(controller_event);
    }
    
    void tm2_sensor::raise_error_notification(const std::string& msg)
    {
        notification error{ RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, 0, RS2_LOG_SEVERITY_ERROR, msg };
        error.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        get_notifications_proccessor()->raise_notification(error);
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
                raise_error_notification(to_string() << "Failed to send connect to controller " << c.macAddress << "(Status: " << get_string(status) << ")");
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
            raise_error_notification(to_string() << "Failed to disconnect to controller " << id << "(Status: " <<  get_string(status) << ")");
        }
        else
        {
            std::string msg = to_string() << "Disconnected from controller #" << id;
            perc::TrackingData::ControllerDisconnectedEventFrame f;
            f.controllerId = id;
            raise_controller_event(msg, controller_event_serializer::serialized_data(f), std::chrono::high_resolution_clock::now().time_since_epoch().count());
        }
    }

    ///////////////
    // Device

    tm2_device::tm2_device(
            std::shared_ptr<perc::TrackingManager> manager,
            perc::TrackingDevice* dev,
            std::shared_ptr<context> ctx,
            const platform::backend_device_group& group) :
        device(ctx, group),
        _dev(dev),
        _manager(manager)
    {
        TrackingData::DeviceInfo info;
        auto status = dev->GetDeviceInfo(info);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to get device info");
        }
        register_info(RS2_CAMERA_INFO_NAME, tm2_device_name());
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, to_string() << info.serialNumber);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, to_string() << info.fw.major << "." << info.fw.minor << "." << info.fw.patch << "." << info.fw.build);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, to_string() << info.usbDescriptor.idProduct);
        std::string device_path =
            std::string("vid_") + std::to_string(info.usbDescriptor.idVendor) +
            std::string(" pid_") + std::to_string(info.usbDescriptor.idProduct) +
            std::string(" bus_") + std::to_string(info.usbDescriptor.bus) +
            std::string(" port_") + std::to_string(info.usbDescriptor.port);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, device_path);

        _sensor = std::make_shared<tm2_sensor>(this, dev);
        add_sensor(_sensor);
        //For manual testing: enable_loopback("C:\\dev\\recording\\tm2.bag");
    }

    tm2_device::~tm2_device()
    {
        for (auto&& d : get_context()->query_devices())
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
}