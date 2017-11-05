#include <memory>
#include <thread>
#include "tm-device.h"
#include "TrackingData.h"
#include "stream.h"

using namespace perc;

namespace librealsense
{
    static rs2_format convertTm2PixelFormat(perc::PixelFormat format);
    static perc::PixelFormat convertToTm2PixelFormat(rs2_format format);

    class tm2_sensor : public sensor_base, public video_sensor_interface, public TrackingDevice::Listener
    {
    public:
        tm2_sensor(tm2_device* owner, perc::TrackingDevice* dev)
            : sensor_base("Tracking Module", owner), _tm_dev(dev),
              _fisheye_left(std::make_shared<stream>(RS2_STREAM_FISHEYE)),
              _fisheye_right(std::make_shared<stream>(RS2_STREAM_FISHEYE))             
        {}


        //sensor
        ////////
        stream_profiles init_stream_profiles() override
        {
            //get TM2 profiles
            auto status = _tm_dev->GetSupportedProfile(_tm_supported_profiles);
            if (status != Status::SUCCESS)
            {
                throw io_exception("Failed to get supported raw streams"); 
            }
            //extract video profiles
            std::vector<TrackingData::VideoProfile> video_profiles(_tm_supported_profiles.video, _tm_supported_profiles.video + VideoProfileMax);

            stream_profiles results;
            for (auto tm_profile : video_profiles)
            {
                platform::stream_profile p;
                p.width = tm_profile.profile.width;
                p.height = tm_profile.profile.height;
                p.fps = tm_profile.fps;  
                p.format = tm_profile.profile.pixelFormat; //save the TM2 native pixel format to use upon set profile

                auto profile = std::make_shared<video_stream_profile>(p);
                profile->set_dims(p.width, p.height);
                profile->set_stream_type(RS2_STREAM_FISHEYE);//TODO - assuming just FE 
                profile->set_stream_index(tm_profile.sensorIndex + 1); //TM2 sensor index represent a FE virtual sensor. 
                profile->set_format(convertTm2PixelFormat(tm_profile.profile.pixelFormat));
                profile->set_framerate(p.fps);
                profile->set_unique_id(tm_profile.sensorIndex);

                results.push_back(profile);

                //assign_stream(_fisheye_left, profile);             

                //TODO - need to register to have resolve work
                //native_pixel_format pf;
                //register_pixel_format(pf);
            }
            return results;           
        }

        void open(const stream_profiles& requests) override
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            if (_is_streaming)
                throw wrong_api_call_sequence_exception("open(...) failed. TM2 device is streaming!");
            else if (_is_opened)
                throw wrong_api_call_sequence_exception("open(...) failed. TM2 device is already opened!");
            
            _source.init(_metadata_parsers);
            _source.set_sensor(this->shared_from_this());

            //TODO - assume a single profile is supported, just validate this is what's requested. 
            //need to use sensor_base::resolve_requests?
            
            for (auto&& r : requests)
            {
                auto sp = to_profile(r.get());
                // need to set both L & R streams profile, due to TM2 FW limitation
                int stream_index = sp.index - 1;
                int pair_stream_index = (stream_index % 2) == 0 ? stream_index + 1 : stream_index - 1;
                // TODO - assuming a single profile per IR stream, for both L & R
                auto tm_profile = _tm_supported_profiles.video[stream_index];
                auto tm_profile_pair = _tm_supported_profiles.video[pair_stream_index];  
                if (tm_profile.fps == sp.fps && 
                    tm_profile.profile.height == sp.height && 
                    tm_profile.profile.width == sp.width && 
                    tm_profile.profile.pixelFormat == convertToTm2PixelFormat(sp.format))
                {
                    _tm_active_profiles.set(tm_profile, true, true);
                    _tm_active_profiles.set(tm_profile_pair, true, true);
                }
            }
            _is_opened = true;
        }

        void close() override
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            if (_is_streaming)
                throw wrong_api_call_sequence_exception("close() failed. TM2 device is streaming!");
            else if (!_is_opened)
                throw wrong_api_call_sequence_exception("close() failed. TM2 device was not opened!");

            //reset active profile
            std::vector<TrackingData::VideoProfile> video_profiles(_tm_active_profiles.video, _tm_active_profiles.video + VideoProfileMax);
            for (auto tm_profile : video_profiles)
            {
                _tm_active_profiles.set(tm_profile, false, false);
            }

            _is_opened = false;
        }

        void start(frame_callback_ptr callback) override
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            if (_is_streaming)
                throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device is already streaming!");
            else if (!_is_opened)
                throw wrong_api_call_sequence_exception("start_streaming(...) failed. TM2 device was not opened!");

            _source.set_callback(callback);
            
            _tm_dev->Start(this, &_tm_active_profiles);

            _is_streaming = true;
        }

        void stop() override
        {
            std::lock_guard<std::mutex> lock(_configure_lock);
            if (!_is_streaming)
                throw wrong_api_call_sequence_exception("stop_streaming() failed. TM2 device is not streaming!");

            _tm_dev->Stop();

            _is_streaming = false;            
        }

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override
        {
            rs2_intrinsics result;

            return result;
        }

        ~tm2_sensor() 
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

 
        // Tracking listener
        ////////////////////
        void onPoseFrame(perc::TrackingData::PoseFrame& pose) override {}

        void onVideoFrame(perc::TrackingData::VideoFrame& tm_frame) override 
        {           
            if (!_is_streaming)
            {
                LOG_WARNING("Frame received with streaming inactive");
                return;
            }

            auto bpp = get_image_bpp(convertTm2PixelFormat(tm_frame.profile.pixelFormat));
            frame_additional_data additional_data(tm_frame.timestamp,
                                                  tm_frame.frameId,
                                                  tm_frame.systemTimestamp,
                                                  0, //TODO - need to add metadata
                                                  nullptr);

            // Find the frame stream profile
            std::shared_ptr<stream_profile_interface> profile = nullptr;
            auto profiles = get_stream_profiles();
            for (auto&& p : profiles)
            {
                //TODO - no way to match resolution??
                if ((p->get_stream_index() == tm_frame.sensorIndex + 1) &&
                    p->get_format() == convertTm2PixelFormat(tm_frame.profile.pixelFormat) &&
                    p->get_stream_type() == RS2_STREAM_FISHEYE)  //TODO - assuming just FE 
                {
                    profile = p;
                    break;
                }
            }            
            //TODO - extension_type param assumes not depth
            frame_holder frame = _source.alloc_frame(RS2_EXTENSION_VIDEO_FRAME, tm_frame.profile.height * tm_frame.profile.stride, additional_data, true);
            if (frame.frame)
            {
                auto video = (video_frame*)(frame.frame);
                video->assign(tm_frame.profile.width, tm_frame.profile.height, tm_frame.profile.stride, bpp);
                frame->set_timestamp(tm_frame.timestamp);
                frame->set_timestamp_domain(RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME);//TODO - is this correct?
                frame->set_stream(profile);
                //frame->set_sensor(this); //TODO? uvc doesn't set it?
                video->data.assign(tm_frame.data, tm_frame.data + (tm_frame.profile.height * tm_frame.profile.stride));
            }
            else
            {
                LOG_INFO("Dropped frame. alloc_frame(...) returned nullptr");
                return;
            }             
             //frame->attach_continuation([]() {frame.release()});
             _source.invoke_callback(std::move(frame));
        }
        
        void onAccelerometerFrame(perc::TrackingData::AccelerometerFrame& frame) override {}
        void onGyroFrame(perc::TrackingData::GyroFrame& frame) override {}
        void onControllerDiscoveryEventFrame(perc::TrackingData::ControllerDiscoveryEventFrame& frame) override {}
        void onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame) override {}
        void onControllerFrame(perc::TrackingData::ControllerFrame& frame) override {}


    private:
        perc::TrackingDevice* _tm_dev;
        std::shared_ptr<stream> _fisheye_left, _fisheye_right;
        std::mutex _configure_lock;
        TrackingData::Profile _tm_supported_profiles;
        TrackingData::Profile _tm_active_profiles;
        std::thread _process_thread;
    };
    

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
        register_info(RS2_CAMERA_INFO_NAME, "Tracking Module 2");
        register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, to_string() << info.serialNumber);
        register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, to_string() << info.fw.major << "." << info.fw.minor << "." << info.fw.patch << "." << info.fw.build);

        add_sensor(std::make_shared<tm2_sensor>(this, _dev)); 

	}






    
    std::vector<std::pair<perc::PixelFormat, rs2_format>> tm2_formats_map = 
    {
        { perc::PixelFormat::ANY,           RS2_FORMAT_ANY},
        { perc::PixelFormat::Z16,           RS2_FORMAT_Z16 },
        { perc::PixelFormat::DISPARITY16,   RS2_FORMAT_DISPARITY16 },
        { perc::PixelFormat::XYZ32F,        RS2_FORMAT_XYZ32F },
        { perc::PixelFormat::YUYV,          RS2_FORMAT_YUYV },
        { perc::PixelFormat::RGB8,          RS2_FORMAT_RGB8 },
        { perc::PixelFormat::BGR8,          RS2_FORMAT_BGR8 },
        { perc::PixelFormat::RGBA8,         RS2_FORMAT_RGBA8 },
        { perc::PixelFormat::BGRA8,         RS2_FORMAT_BGRA8 },
        { perc::PixelFormat::Y8,            RS2_FORMAT_Y8 },
        { perc::PixelFormat::Y16,           RS2_FORMAT_Y16 },
        { perc::PixelFormat::RAW8,          RS2_FORMAT_RAW8 },
        { perc::PixelFormat::RAW10,         RS2_FORMAT_RAW10 },
        { perc::PixelFormat::RAW16,         RS2_FORMAT_RAW16 }
    };
        
    static rs2_format convertTm2PixelFormat(perc::PixelFormat format)
    {
        //         switch (format)
        //         {
        //         case perc::PixelFormat::ANY: return RS2_FORMAT_ANY;
        //         case perc::PixelFormat::Z16: return RS2_FORMAT_Z16;
        //         case perc::PixelFormat::DISPARITY16: return RS2_FORMAT_DISPARITY16;
        //         case perc::PixelFormat::XYZ32F: return RS2_FORMAT_XYZ32F;
        //         case perc::PixelFormat::YUYV: return RS2_FORMAT_YUYV;
        //         case perc::PixelFormat::RGB8: return RS2_FORMAT_RGB8;
        //         case perc::PixelFormat::BGR8: return RS2_FORMAT_BGR8;
        //         case perc::PixelFormat::RGBA8: return RS2_FORMAT_RGBA8;
        //         case perc::PixelFormat::BGRA8: return RS2_FORMAT_BGRA8;
        //         case perc::PixelFormat::Y8: return RS2_FORMAT_Y8;
        //         case perc::PixelFormat::Y16: return RS2_FORMAT_Y16;
        //         case perc::PixelFormat::RAW8: return RS2_FORMAT_RAW8;
        //         case perc::PixelFormat::RAW10: return RS2_FORMAT_RAW10;
        //         case perc::PixelFormat::RAW16: return RS2_FORMAT_RAW16;
        //         default: return RS2_FORMAT_ANY;
        //         }
        for (auto m : tm2_formats_map)
        {
            if (m.first == format)
            {
                return m.second;
            }
        }
        throw invalid_value_exception("Invalid TM2 pixel format");
    }

    static perc::PixelFormat convertToTm2PixelFormat(rs2_format format)
    {
        for (auto m : tm2_formats_map)
        {
            if (m.second == format)
            {
                return m.first;
            }
        }
        throw invalid_value_exception("No matching TM2 pixel format");
    }




}