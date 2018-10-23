// License: Apache 2.0. See LICENSE file in root directory.
//
// the RealsensePreview used to render RGB/Depth/IR data 


#include <string>
#include <thread>
#include <map>
#include <iostream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <memory>

#include <string.h>
#include <jni.h>
#include <android/native_window_jni.h>

#include "cde_log.h"
#include "jni_utils.h"
#include "util.h"
#include "fifo.h"
#include "irsa_event.h"
#include "irsa_rs_preview.h"
#include "irsa_mgr.h"

#define LOG_TAG  "IRSA_NATIVE_RS_PREVIEW"



using namespace std::chrono;

namespace irsa {

static std::chrono::steady_clock::time_point beginTime;
static const std::string no_camera_message = "No camera connected, please connect 1 or more";
static const std::string platform_camera_name = "Platform Camera";

static const int DEPTH_FIFO_CAPACITY = 6;
static const int RGB_FIFO_CAPACITY = 30;

static int probeRSDevice() {
    int deviceCounts = 0;
    std::shared_ptr<rs2::context> ctx; 

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);

    try {
        ctx = std::make_shared<rs2::context>();
        deviceCounts = 0;

        if (!ctx) {
            LOGD("init failed");
            return 0;
        }
        LOGD("query devices");
        for (auto &&dev : ctx->query_devices()) {
            LOGD("find new device %d", deviceCounts);
            deviceCounts++;
        }
        return deviceCounts;
    } catch (const rs2::error &e) {
        std::stringstream ss;
        ss << "RealSense error calling:" << e.get_failed_function() << e.get_failed_args()
           << e.what();
        LOGV("%s", ss.str().c_str());
        //the initialization isn't completed at the moment
        //mgr->notify(IRSA_ERROR, IRSA_ERROR_PROBE_RS, (int)ss.str().c_str());
        mgr->setInfo(ss.str());
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "RealSense error: " << e.what();
        LOGV("error_message :%s", ss.str().c_str());
        //the initialization isn't completed at the moment
        //mgr->notify(IRSA_ERROR, IRSA_ERROR_PROBE_RS, (int)ss.str().c_str());
        mgr->setInfo(ss.str());
    }
    return deviceCounts;
}


RealsensePreview::RealsensePreview(): _renderID(1) {
    //pls modify here accordingly
    _depthWidth = 1280;
    _depthHeight = 720;
    _depthFPS = 6;
    _deviceCounts = probeRSDevice();
    LOGD("_deviceCounts %d\n", _deviceCounts);
}


RealsensePreview::~RealsensePreview() { 
    LOGV("destructor");
    close();
}


void RealsensePreview::open() {
    LOGD("open");
    _ctx = std::make_shared<rs2::context>();

    std::stringstream ss;
    rs2_error *e = nullptr;
    ss << "librealsense2 VERSION: " << api_version_to_string(rs2_get_api_version(&e)) << std::endl;

    for (auto sensor : _ctx->query_all_sensors()) {
        auto name = sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME);
        auto sn = sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER);
        auto fw = sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_FIRMWARE_VERSION);

        LOGD("NAME: %s SN: %s FW: %s", name, sn, fw);

        ss << "DEVICE NAME: " << name << " SN: " << sn << " FW: " << fw << std::endl;
        for (auto profile : sensor.get_stream_profiles()) {
            auto vprofile = profile.as<rs2::video_stream_profile>();
            LOGD("PROFILE %d = %s, %s, %d, %d, %d", \
                     profile.unique_id(), \
                     vprofile.stream_name().c_str(), \
                     rs2_format_to_string(vprofile.format()), \
                     vprofile.width(), \
                     vprofile.height(), \
                     vprofile.fps());
            ss << "PROFILE " << profile.unique_id() << " = " << vprofile.stream_name() << ", "
                   << rs2_format_to_string(vprofile.format()) << ", " << vprofile.width() << ", "
                   << vprofile.height() << ", " << vprofile.fps() << std::endl;
        }
    }

    for (auto &&dev : _ctx->query_devices()) {
        _deviceCounts++;
        LOGD("find new device %d", _deviceCounts);
    }
}


void RealsensePreview::close() {
    LOGD("close");
    if (_isRunning)
        stopPreview();
    if (_ctx != nullptr) {
        _ctx.reset();
        _ctx = nullptr;
    }
}


int RealsensePreview::getDeviceCounts() {
    if (0 == _deviceCounts) {
#ifndef __SMART_POINTER__
        IrsaMgr *mgr = IrsaMgr::getInstance();
#else
        std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
        CHECK(mgr != nullptr);
        mgr->notify(IRSA_ERROR, IRSA_ERROR_PROBE_RS, (size_t)mgr->getInfo().c_str());
    }
    return _deviceCounts;
}


void RealsensePreview::setPreviewDisplay(std::map<int, ANativeWindow *> &surfaceMap) {
    LOGD("setPreviewDisplay");
    _surfaceMap = surfaceMap;
    for (auto &itr : _surfaceMap) {
        ANativeWindow_setBuffersGeometry(itr.second, _rgbWidth, _rgbHeight, WINDOW_FORMAT_RGBX_8888);
    }
    
    //for optimization purpose
    _renderSurface = _surfaceMap.at(0);
}


void RealsensePreview::enableDevices() {
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto &&dev : _ctx->query_devices()) {
        std::string serial_number(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

        if (_devices.find(serial_number) != _devices.end()) {
            LOGD("already enabled ");
            continue;
        }


        // Ignoring platform cameras (webcams, etc..)
        if (platform_camera_name == dev.get_info(RS2_CAMERA_INFO_NAME)) {
            continue;
        }

        LOGD("camera name: %s", dev.get_info(RS2_CAMERA_INFO_NAME));
        LOGD("Java layer's width %d height %d fps %d", _rgbWidth, _rgbHeight, _rgbFPS);

        rs2::pipeline p;
        rs2::config cfg;
        cfg.enable_device(serial_number);

        cfg.enable_stream(RS2_STREAM_COLOR, _rgbWidth, _rgbHeight, RS2_FORMAT_RGB8, _rgbFPS);
        cfg.enable_stream(RS2_STREAM_DEPTH, _depthWidth, _depthHeight, RS2_FORMAT_Z16, _depthFPS);
        //cfg.enable_stream(RS2_STREAM_INFRARED, _depthWidth, _depthHeight, RS2_FORMAT_Y8, _depthFPS);
        cfg.enable_stream(RS2_STREAM_INFRARED, _depthWidth, _depthHeight, RS2_FORMAT_RGB8, _depthFPS);

        //hardcode 
        _cameraDataSize = _depthWidth * _depthHeight * 5 + _rgbWidth * _rgbHeight * 3 + 1;
        LOGD("_cameraDataSize %d\n", _cameraDataSize);

        rs2::pipeline_profile profile = p.start(cfg);
        LOGD("serial_number %s", serial_number.c_str());
        _devices.emplace(serial_number, view_port{p, cfg, profile});

        LOGD("allocate memory for RGB fifo and Depth fifo \n");
        _framesIRDepth = fifo_new(DEPTH_FIFO_CAPACITY, _cameraDataSize);
        _framesRGB     = fifo_new(RGB_FIFO_CAPACITY, _rgbWidth * _rgbHeight * 3);
    }
}



void RealsensePreview::disableDevices() {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto &view : _devices) {
        view.second.config.disable_all_streams();
        view.second.pipe.stop();
    }

    _devices.clear();
    LOGV("destroy camera data fifo");
    _framesIRDepth->destroy(_framesIRDepth);
    _framesRGB->destroy(_framesRGB);
}


void RealsensePreview::startPreview() {
    LOGD("start");
    CHECK(_ctx != nullptr);

    enableDevices();

    _threadPoll   = std::thread(&RealsensePreview::threadPoll, this);
    _threadWorker = std::thread(&RealsensePreview::threadWorker, this);
    _threadRender = std::thread(&RealsensePreview::threadRender, this);
    _isRunning = true;
}


void RealsensePreview::stopPreview() {
    LOGD("Stop");
    _isRunning = false;

    if (_threadRender.joinable())
        _threadRender.join();

    if (_threadWorker.joinable())
        _threadWorker.join();

    if (_threadPoll.joinable())
        _threadPoll.join();


    disableDevices();
}


void RealsensePreview::threadPoll() {
    LOGD("device counts %d", _deviceCounts);
    LOGD("_surfacemap size %d\n", _surfaceMap.size());
    for (auto surface : _surfaceMap) {
        LOGD("surface index %d, surface %p\n", surface.first, surface.second);
    }

    beginTime = std::chrono::steady_clock::now();
    _polledFrameCounts      = 0;
    _renderedFrameCounts    = 0;
    _workedFrameCounts      = 0;

    while (_isRunning) {
        int device_id = 0;

        for (auto &&view : _devices) {
            rs2::frameset frameset;
            if (view.second.pipe.poll_for_frames(&frameset)) {

                auto a = std::chrono::steady_clock::now();

                auto color = frameset.get_color_frame();
                auto depth = frameset.get_depth_frame();
                auto ir = frameset.get_infrared_frame();
                //auto colorize_depth = _colorizer.process(depth);

                buf_element_t *frame_q = _framesRGB->buffer_try_alloc(_framesRGB);
                if (frame_q) {
                    if (color) {
                        auto vframe = color.as<rs2::video_frame>();
                        memcpy(frame_q->mem,
                                reinterpret_cast<const uint8_t *>(vframe.get_data()),
                                _rgbWidth * _rgbHeight * 3);

                        _framesRGB->put(_framesRGB, frame_q);
                    } else {
                        frame_q->free_buffer(frame_q);
                    }
                } else {
                    LOGD("dropping color frame");
                }


                auto b = std::chrono::steady_clock::now();
                std::chrono::microseconds delta = std::chrono::duration_cast<microseconds>(b - a);
                LOGD("duration of poll is %d microseconds\n", delta.count());

                ++_polledFrameCounts;

                auto endTime = std::chrono::steady_clock::now();
                std::chrono::milliseconds duration = std::chrono::duration_cast<milliseconds>(
                        endTime - beginTime);
                LOGD("poll fps:%d", static_cast<int>(_polledFrameCounts * 1.f / duration.count() * 1000));

                buf_element_t *frame_p = _framesIRDepth->buffer_try_alloc(_framesIRDepth);
                if (frame_p) {
                    uint8_t *_cameraData = frame_p->mem;
                    if (ir) {
                        memcpy(_cameraData, reinterpret_cast<const uint8_t *>(ir.get_data()),
                                _depthWidth * _depthHeight * 3);
                    }

                    if (depth) {
                        memcpy(_cameraData + _depthWidth * _depthHeight * 3,
                                reinterpret_cast<const uint8_t *>(depth.get_data()),
                                _depthWidth * _depthHeight * 2);
                    }

                    if (color) {
                        //memcpy(_cameraData + _depthWidth * _depthHeight * 5, reinterpret_cast<const uint8_t *>(color.get_data()), _rgbWidth * _rgbHeight * 3);
                    }
                    _framesIRDepth->put(_framesIRDepth, frame_p);
                } else {
                    LOGD("dropping ir&depth frame");
                }
            } else {
                //LOGD("can't got frame from librealsense\n");
            }
            device_id++;
        }
    }
}


void RealsensePreview::threadRender() {
    char szInfo[128];
    int fps = 0;

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);

    buf_element_t *frame_p = nullptr;
    while (_isRunning) {
        frame_p = _framesRGB->try_get(_framesRGB);
        if (frame_p) {
            auto a = std::chrono::steady_clock::now();
            if (1) {
                //hardcode for RGB8 optimization purpose
                ANativeWindow_Buffer buffer = {};
                ARect rect = {0, 0, 640, 480};
                if (ANativeWindow_lock(_renderSurface, &buffer, &rect) == 0) {
                    //hardcode for optimization purpose:640 480 640 * 3
                    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
                    const uint8_t *in = static_cast<const uint8_t *>(frame_p->mem);
#if 1
                    for (int y = 0; y < 480; ++y) {
                        for (int x = 0; x < 640; ++x) {
                            out[y * 640 * 4 + x * 4 + 0] = in[y * 1920 + x * 3 + 0];
                            out[y * 640 * 4 + x * 4 + 1] = in[y * 1920 + x * 3 + 1];
                            out[y * 640 * 4 + x * 4 + 2] = in[y * 1920 + x * 3 + 2];
                            out[y * 640 * 4 + x * 4 + 3] = 0xFF;
                        }
                    }
#endif
                    ANativeWindow_unlockAndPost(_renderSurface);
                } else {
                    LOGD("ANativeWindow_lock FAILED");
                }
            }

            auto b = std::chrono::steady_clock::now();
            std::chrono::microseconds delta = std::chrono::duration_cast<microseconds>(b - a);
            LOGD("duration of renderFrame is %d microseconds\n", delta.count());

            ++_renderedFrameCounts;

            auto endTime = std::chrono::steady_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<milliseconds>(endTime - beginTime);
            fps = static_cast<int>(_renderedFrameCounts * 1.f / duration.count() * 1000);
            LOGD("render fps:%d", fps);

            if (0 == _renderedFrameCounts % 2) {
                snprintf(szInfo, 128, "renderedFrames %d, render fps %d", _renderedFrameCounts, fps);
                mgr->notify(IRSA_INFO, IRSA_INFO_PREVIEW, (size_t)szInfo);
            }

            frame_p->free_buffer(frame_p);
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }
    }
}


void RealsensePreview::threadWorker() {
    LOGD("device counts %d", _deviceCounts);
    AutoJNIEnv env;

    buf_element_t *frame_p = nullptr;
    while (_isRunning) {
        frame_p = _framesIRDepth->try_get(_framesIRDepth);
        if (frame_p) {
            uint8_t *_cameraData = frame_p->mem;
            auto a = std::chrono::steady_clock::now();
            //doing your proprietary/business code here
            //_doWork(_cameraData, _cameraDataSize);
            auto b = std::chrono::steady_clock::now();
            std::chrono::microseconds delta = std::chrono::duration_cast<microseconds>(b - a);
            LOGD("duration of worker is %d microseconds\n", delta.count());
            frame_p->free_buffer(frame_p);

            _workedFrameCounts++;
            auto endTime = std::chrono::steady_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<milliseconds>(
                        endTime - beginTime);
            LOGD("worker fps:%d", static_cast<int>(_workedFrameCounts * 1.f / duration.count() * 1000));
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }
    }
}

}
