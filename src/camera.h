#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include "types.h"

#include <cassert>      // For assert
#include <cstring>      // For memcpy
#include <sstream>      // For ostringstream
#include <memory>       // For shared_ptr
#include <vector>       // For vector
#include <thread>       // For thread
#include <mutex>        // For mutex
#include <array>

namespace rsimpl
{
	struct ToString
	{
		std::ostringstream ss;
		template<class T> ToString & operator << (const T & val) { ss << val; return *this; }
		operator std::string() const { return ss.str(); }
	};

    inline void CheckUVC(const char * call, uvc_error_t status)
    {
        if (status < 0)
        {
            throw std::runtime_error(ToString() << call << "(...) returned " << uvc_strerror(status));
        }
    }

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
        std::vector<rs_intrinsics> intrinsics;
        pose stream_poses[RS_STREAM_NUM];
        float depth_scale;
    }; 
}

struct rs_error
{
	std::string function;
	std::string message;
};

struct rs_camera
{
protected:
    class Subdevice;

    class Stream
    {
        friend class Subdevice;

        rsimpl::StreamMode mode;

        volatile bool updated = false;
        std::vector<uint8_t> front, middle, back;
        std::mutex mutex;
    public:
        const rsimpl::StreamMode & get_mode() const { return mode; }
        const void * get_image() const { return front.data(); }

        void set_mode(const rsimpl::StreamMode & mode);
        bool update_image();
    };

    class Subdevice
    {
        uvc_device_handle_t * uvcHandle;
        uvc_stream_ctrl_t ctrl;
        rsimpl::SubdeviceMode mode;

        std::vector<std::shared_ptr<Stream>> streams;

        void on_frame(uvc_frame_t * frame);
    public:
        Subdevice(uvc_device_t * device, int subdeviceNumber) { rsimpl::CheckUVC("uvc_open2", uvc_open2(device, &uvcHandle, subdeviceNumber)); }
        ~Subdevice() { uvc_stop_streaming(uvcHandle); uvc_close(uvcHandle); }

        uvc_device_handle_t * get_handle() { return uvcHandle; }
        void set_mode(const rsimpl::SubdeviceMode & mode, std::vector<std::shared_ptr<Stream>> streams);
        void start_streaming();
        void stop_streaming();
    };

    uvc_context_t * context;
    uvc_device_t * device;
    const rsimpl::StaticCameraInfo camera_info;

    std::array<rsimpl::StreamRequest, RS_STREAM_NUM> requests;    // Indexed by RS_DEPTH, RS_COLOR, ...
    std::shared_ptr<Stream> streams[RS_STREAM_NUM];               // Indexed by RS_DEPTH, RS_COLOR, ...
    std::vector<std::unique_ptr<Subdevice>> subdevices;           // Indexed by UVC subdevices number (0, 1, 2...)

    std::string cameraName;
    rsimpl::CalibrationInfo calib;

    uvc_device_handle_t * first_handle;
    bool isCapturing = false;
  
public:
    rs_camera(uvc_context_t * context, uvc_device_t * device, const rsimpl::StaticCameraInfo & camera_info);
    ~rs_camera();

    const char * GetCameraName() const { return cameraName.c_str(); }

    void EnableStream(rs_stream stream, int width, int height, rs_format format, int fps);
    bool IsStreamEnabled(rs_stream stream) const { return (bool)streams[stream]; }

    void start_capture();
    void stop_capture();
    void wait_all_streams();

    rs_format get_image_format(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_mode().format; }
    const void * get_image_pixels(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_image(); }
    float get_depth_scale() const { return calib.depth_scale; }

    rs_intrinsics GetStreamIntrinsics(rs_stream stream) const;
    rs_extrinsics GetStreamExtrinsics(rs_stream from, rs_stream to) const;

    virtual void EnableStreamPreset(rs_stream stream, rs_preset preset) = 0;
    virtual rsimpl::CalibrationInfo RetrieveCalibration() = 0;
    virtual void SetStreamIntent() = 0;
};

struct rs_context
{
    uvc_context_t * privateContext;
	std::vector<std::shared_ptr<rs_camera>> cameras;
    
	rs_context();
	~rs_context();

	void QueryDeviceList();
};

#endif
