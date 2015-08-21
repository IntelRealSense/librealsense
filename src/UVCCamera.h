#pragma once
#ifndef LIBREALSENSE_UVC_CAMERA_H
#define LIBREALSENSE_UVC_CAMERA_H

#include "rs-internal.h"

#ifdef USE_UVC_DEVICES
#include <libuvc/libuvc.h>

namespace rs
{
    inline void CheckUVC(const char * call, uvc_error_t status)
    {
        if (status < 0)
        {
            throw std::runtime_error(ToString() << call << "(...) returned " << uvc_strerror(status));
        }
    }

    struct ResolutionMode
    {
        int stream;                 // RS_DEPTH, RS_COLOR, etc.

        int width, height;          // Resolution visible to the library client
        int fps;                    // Framerate visible to the library client
        int format;                 // Format visible to the library client

        int uvcWidth, uvcHeight;    // Resolution advertised over UVC
        int uvcFps;                 // Framerate advertised over UVC
        uvc_frame_format uvcFormat; // Format advertised over UVC

        rs_intrinsics intrinsics;   // Image intrinsics
    };

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
        std::vector<ResolutionMode> modes;
        pose stream_poses[2];
        float depth_scale;
    };

    class UVCCamera : public rs_camera
    {
    protected: 
        class StreamInterface
        {
            uvc_device_handle_t * uvcHandle;
            uvc_stream_ctrl_t ctrl;
            ResolutionMode mode;

            volatile bool updated = false;
            std::vector<uint8_t> front, middle, back;
            std::mutex mutex;
        public:
            StreamInterface(uvc_device_t * device, int subdeviceNumber) { CheckUVC("uvc_open2", uvc_open2(device, &uvcHandle, subdeviceNumber)); }
            ~StreamInterface() { uvc_stop_streaming(uvcHandle); uvc_close(uvcHandle); }

            const ResolutionMode & get_mode() const { return mode; }
            template<class T> const T * get_image() const { return reinterpret_cast<const T *>(front.data()); }

            uvc_device_handle_t * get_handle() { return uvcHandle; }
            void set_mode(const ResolutionMode & mode);
            void start_streaming();
            void stop_streaming();
            bool update_image();
        };

        uvc_context_t * context;
        uvc_device_t * device;
        std::unique_ptr<StreamInterface> streams[2];

        std::string cameraName;
        CalibrationInfo calib;

        uvc_device_handle_t * GetHandleToAnyStream();
    public:
        UVCCamera(uvc_context_t * context, uvc_device_t * device);
        ~UVCCamera();

        const char * GetCameraName() const override final { return cameraName.c_str(); }

        void EnableStream(int stream, int width, int height, int fps, int format) override final;
        void StartStreaming() override final;
        void StopStreaming() override final;
        void WaitAllStreams() override final;

        const uint8_t * GetColorImage() const override final { return streams[RS_COLOR] ? streams[RS_COLOR]->get_image<uint8_t>() : nullptr; }
        const uint16_t * GetDepthImage() const override final { return streams[RS_DEPTH] ? streams[RS_DEPTH]->get_image<uint16_t>() : nullptr; }      
        float GetDepthScale() const override final { return calib.depth_scale; }

        rs_intrinsics GetStreamIntrinsics(int stream) const override final;
        rs_extrinsics GetStreamExtrinsics(int from, int to) const override final;

        virtual int GetStreamSubdeviceNumber(int stream) const = 0;
        virtual CalibrationInfo RetrieveCalibration(uvc_device_handle_t * handle) = 0;
        virtual void SetStreamIntent(bool depth, bool color) = 0;
    };
    
} // end namespace rs
#endif

#endif
