// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <memory>
#include <thread>
#include "tm-device.h"
#include "TrackingData.h"
#include "stream.h"
#include "tm-conversions.h"

using namespace perc;
using namespace std::chrono;

namespace librealsense
{
    struct frame_metadata
    {
        int64_t arrival_ts;
    };

    struct video_frame_metadata
    {
        frame_metadata frame_md;
        uint32_t exposure_time;
    };

    struct motion_frame_metadata
    {
        frame_metadata frame_md;
        float temperature;
    };


    class md_tm2_exposure_time_parser : public md_attribute_parser_base
    {
    public:
        rs2_metadata_type get(const frame& frm) const override
        {
            if (auto* vf = dynamic_cast<const video_frame*>(&frm))
            {
                const video_frame_metadata* video_md = reinterpret_cast<const video_frame_metadata*>(frm.additional_data.metadata_blob.data());
                return (rs2_metadata_type)(video_md->exposure_time);
            }
            return 0;
        }

        bool supports(const frame& frm) const override
        {
            if (auto* vf = dynamic_cast<const video_frame*>(&frm))
            {
                return true;
            }
            return false;
        }
    };

    class md_tm2_temperature_parser : public md_attribute_parser_base
    {
    public:
        //TODO: move the md creation here
        rs2_metadata_type get(const frame& frm) const override
        {
            if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
            {
                const motion_frame_metadata* motion_md = reinterpret_cast<const motion_frame_metadata*>(frm.additional_data.metadata_blob.data());
                return (rs2_metadata_type)(motion_md->temperature);
            }
            return 0;
        }

        bool supports(const frame& frm) const override
        {
            if (auto* mf = dynamic_cast<const motion_frame*>(&frm))
            {
                return true;
            }
            return false;
        }
    };

    tm2_sensor::tm2_sensor(tm2_device* owner, perc::TrackingDevice* dev)
        : sensor_base("Tracking Module", owner), _tm_dev(dev)
    {
        register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, std::make_shared<md_tm2_exposure_time_parser>());
        register_metadata(RS2_FRAME_METADATA_TEMPERATURE, std::make_shared<md_tm2_temperature_parser>());
    }

    tm2_sensor::~tm2_sensor()
    {
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

        for (auto&& r : requests)
        {
            auto sp = to_profile(r.get());
            int stream_index = sp.index - 1;

            switch (r->get_stream_type())
            {
            case RS2_STREAM_FISHEYE:
            {
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
    }

    void tm2_sensor::close()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. TM2 device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. TM2 device was not opened!");

        //reset active profiles
        _tm_active_profiles.reset();

        _is_opened = false;
    }

    void tm2_sensor::start(frame_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device is already streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device was not opened!");

        _source.set_callback(callback);

        auto status = _tm_dev->Start(this, &_tm_active_profiles);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to start TM2 camera");
        }
        _is_streaming = true;
    }

    void tm2_sensor::stop()
    {
        std::lock_guard<std::mutex> lock(_configure_lock);
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. TM2 device is not streaming!");

        auto status = _tm_dev->Stop();
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to stop TM2 camera");
        }

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
        video_md.frame_md.arrival_ts = tm_frame.arrivalTimeStamp;
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
        frame_metadata frame_md = { 0 };
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
            info->confidence = tm_frame.confidence;
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

    }
    void tm2_sensor::onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame)
    {

    }
    void tm2_sensor::onControllerFrame(perc::TrackingData::ControllerFrame& frame)
    {

    }

    void tm2_sensor::handle_imu_frame(perc::TrackingData::TimestampedData& tm_frame_ts, unsigned long long frame_number, rs2_stream stream_type, int index, float3 imu_data, float temperature)
    {
        auto ts_double_nanos = duration<double, std::nano>(tm_frame_ts.timestamp);
        duration<double, std::milli> ts_ms(ts_double_nanos);
        auto sys_ts_double_nanos = duration<double, std::nano>(tm_frame_ts.systemTimestamp);
        duration<double, std::milli> system_ts_ms(sys_ts_double_nanos);
        motion_frame_metadata motion_md = { 0 };
        motion_md.frame_md.arrival_ts = tm_frame_ts.arrivalTimeStamp;
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

    tm2_device::tm2_device(
        std::shared_ptr<perc::TrackingManager> manager,
        perc::TrackingDevice* dev,
        std::shared_ptr<context> ctx,
        const platform::backend_device_group& group)
        : device(ctx, group),
        _dev(dev), _manager(manager)
    {
        TrackingData::DeviceInfo info;
        auto status = _dev->GetDeviceInfo(info);
        if (status != Status::SUCCESS)
        {
            throw io_exception("Failed to get device info");
        }
        register_info(RS2_CAMERA_INFO_NAME, "Intel RealSense T260");
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, to_string() << info.serialNumber);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, to_string() << info.fw.major << "." << info.fw.minor << "." << info.fw.patch << "." << info.fw.build);
        register_info(RS2_CAMERA_INFO_PRODUCT_ID, to_string() << info.usbDescriptor.idProduct);
        std::string device_path =
            std::string("vid_") + std::to_string(info.usbDescriptor.idVendor) +
            std::string(" pid_") + std::to_string(info.usbDescriptor.idProduct) +
            std::string(" bus_") + std::to_string(info.usbDescriptor.bus) +
            std::string(" port_") + std::to_string(info.usbDescriptor.port);
        register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, device_path);

        add_sensor(std::make_shared<tm2_sensor>(this, _dev));

    }

    void tm2_device::enable_loopback(const std::string& source_file)
    {
        auto ctx = get_context();
        try
        {
            //playback_device d(ctx, std::make_shared<ros_reader>(file, ctx));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create playback device from given file. File = \"" << source_file << "\". Exception: " << e.what());
            throw librealsense::invalid_value_exception("Failed to enable loopback");
        }
        //auto sensor = get_sensor(0);
    }

    void tm2_device::disble_loopback()
    {

    }
}