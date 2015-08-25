#pragma once
#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "rs-internal.h"

#ifdef USE_UVC_DEVICES
#include <libuvc/libuvc.h>

namespace rs
{
    const int MAX_STREAMS = 4;
    
    inline void CheckUVC(const char * call, uvc_error_t status)
    {
        if (status < 0)
        {
            throw std::runtime_error(ToString() << call << "(...) returned " << uvc_strerror(status));
        }
    }

    struct StreamRequest
    {
        bool enabled;
        int width, height, format, fps;
    };

    struct StreamMode
    {
        int stream;                 // RS_DEPTH, RS_COLOR, RS_INFRARED, RS_INFRARED_2, etc.
        rs_intrinsics intrinsics;   // Image intrinsics (which includes resolution)
        int format, fps;            // Pixel format and framerate visible to the client library
    };

    struct SubdeviceMode
    {
        int subdevice;                      // 0, 1, 2, etc...
        int width, height;                  // Resolution advertised over UVC
        uvc_frame_format format;            // Pixel format advertised over UVC
        int fps;                            // Framerate advertised over UVC
        std::vector<StreamMode> streams;    // Modes for streams which can be supported by this device mode
        void (* unpacker)(void * dest[], const SubdeviceMode & mode, const uint8_t * frame);
    };

    void unpack_strided_image(void * dest[], const SubdeviceMode & mode, const uint8_t * frame);
    void unpack_rly12_to_y8(void * dest[], const SubdeviceMode & mode, const uint8_t * frame);
    void unpack_yuyv_to_rgb(void * dest[], const SubdeviceMode & mode, const uint8_t * frame);

    // World's tiniest linear algebra library
    struct float3 { float x,y,z; float & operator [] (int i) { return (&x)[i]; } };
    struct float3x3 { float3 x,y,z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
    struct pose { float3x3 orientation; float3 position; };
    inline float3 operator + (const float3 & a, const float3 & b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
    inline float3 operator * (const float3 & a, float b) { return {a.x*b, a.y*b, a.z*b}; }
    inline float3 operator * (const float3x3 & a, const float3 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline float3x3 operator * (const float3x3 & a, const float3x3 & b) { return {a*b.x, a*b.y, a*b.z}; }
    inline float3x3 transpose(const float3x3 & a) { return {{a.x.x,a.y.x,a.z.x}, {a.x.y,a.y.y,a.z.y}, {a.x.z,a.y.z,a.z.z}}; }
    inline float3 operator * (const pose & a, const float3 & b) { return a.orientation * b + a.position; }
    inline pose operator * (const pose & a, const pose & b) { return {a.orientation * b.orientation, a.position + a * b.position}; }
    inline pose inverse(const pose & a) { auto inv = transpose(a.orientation); return {inv, inv * a.position * -1}; }

    struct CalibrationInfo
    {
        std::vector<SubdeviceMode> modes;
        pose stream_poses[MAX_STREAMS];
        float depth_scale;
    };

    class UVCCamera : public rs_camera
    {
    protected: 
        class Subdevice;
        class Stream
        {
            friend class Subdevice;

            StreamMode mode;

            volatile bool updated = false;
            std::vector<uint8_t> front, middle, back;
            std::mutex mutex;
        public:
            const StreamMode & get_mode() const { return mode; }
            const void * get_image() const { return front.data(); }

            void set_mode(const StreamMode & mode);
            bool update_image();
        };

        class Subdevice
        {
            uvc_device_handle_t * uvcHandle;
            uvc_stream_ctrl_t ctrl;
            SubdeviceMode mode;

            std::vector<std::shared_ptr<Stream>> streams;

            void on_frame(uvc_frame_t * frame);
        public:
            Subdevice(uvc_device_t * device, int subdeviceNumber) { CheckUVC("uvc_open2", uvc_open2(device, &uvcHandle, subdeviceNumber)); }
            ~Subdevice() { uvc_stop_streaming(uvcHandle); uvc_close(uvcHandle); }

            uvc_device_handle_t * get_handle() { return uvcHandle; }
            void set_mode(const SubdeviceMode & mode, std::vector<std::shared_ptr<Stream>> streams);
            void start_streaming();
            void stop_streaming();
        };

        uvc_context_t * context;
        uvc_device_t * device;

        std::array<StreamRequest, MAX_STREAMS> requests;    // Indexed by RS_DEPTH, RS_COLOR, ...
        std::shared_ptr<Stream> streams[MAX_STREAMS];       // Indexed by RS_DEPTH, RS_COLOR, ...
        std::vector<std::unique_ptr<Subdevice>> subdevices; // Indexed by UVC subdevices number (0, 1, 2...)

        std::string cameraName;
        CalibrationInfo calib;

        uvc_device_handle_t * first_handle;
        bool isCapturing = false;
        
    public:
        UVCCamera(uvc_context_t * context, uvc_device_t * device);
        ~UVCCamera();

        const char * GetCameraName() const override final { return cameraName.c_str(); }

        void EnableStream(int stream, int width, int height, int fps, int format) override final;
        bool IsStreamEnabled(int stream) const override final { return (bool)streams[stream]; }
        void StartStreaming() override final;
        void StopStreaming() override final;
        void WaitAllStreams() override final;

        const void * GetImagePixels(int stream) const override final { return streams[stream] ? streams[stream]->get_image() : nullptr; }
        float GetDepthScale() const override final { return calib.depth_scale; }

        rs_intrinsics GetStreamIntrinsics(int stream) const override final;
        rs_extrinsics GetStreamExtrinsics(int from, int to) const override final;

        virtual CalibrationInfo RetrieveCalibration() = 0;
        virtual void SetStreamIntent() = 0;
    };
    
} // end namespace rs
#endif

#endif
