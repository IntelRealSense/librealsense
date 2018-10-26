// License: Apache 2.0. See LICENSE file in root directory.
//
// the FakePreview used to render RGB/Depth data with bag file
//
// so we could use FakePreview to tracking code in librealsense's internal
//
// size of surface view is hardcode to 640 x 480
//
// the bag file was recorded by realsense_viewer in Ubuntu 18.04.1 x86-64
// the stream's profile in bag file as following:
// profile of color stream is 1280 x 720
// profile of depth stream is 1280 x 720


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
#include "irsa_fake_preview.h"
#include "irsa_mgr.h"

#define LOG_TAG  "IRSA_NATIVE_FAKE_PREVIEW"



using namespace std::chrono;

namespace irsa {

static const int DEPTH_FIFO_CAPACITY = 6;
static const int PREVIEW_FIFO_CAPACITY = 30;
static const char *BAG_FILE="/sdcard/test.bag";


FakePreview::FakePreview(): _renderID(1), _fifoPreview(nullptr) {
    _depthWidth = 1280;
    _depthHeight = 720;
    _depthFPS = 6;

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);


    if (0 == access(BAG_FILE, F_OK))
        _deviceCounts = 1;
    else  {
        mgr->setInfo("pls check /sdcard/test.bag exist");
        _deviceCounts = 0;
    }
    LOGD("_deviceCounts %d\n", _deviceCounts);
}


FakePreview::~FakePreview() {
    LOGV("destructor");
    close();
}


void FakePreview::open() {
    LOGD("open");

    if (0 == _deviceCounts)
        return;

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);

    try {

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
    } catch (const rs2::error &e) {
        std::stringstream ss;
        ss << "RealSense error calling:" << e.get_failed_function() << e.get_failed_args()
           << e.what();
        LOGV("%s", ss.str().c_str());
        mgr->notify(IRSA_ERROR, IRSA_ERROR_PROBE_RS, (int)ss.str().c_str());
    } catch (const std::exception &e) {
        std::stringstream ss;
        ss << "RealSense error: " << e.what();
        LOGV("error_message :%s", ss.str().c_str());
        mgr->notify(IRSA_ERROR, IRSA_ERROR_PROBE_RS, (int)ss.str().c_str());
    }
}


void FakePreview::close() {
    LOGD("close");

    if (0 == _deviceCounts)
        return;

    if (_isRunning)
        stopPreview();
    if (_ctx != nullptr) {
        _ctx.reset();
        _ctx = nullptr;
    }
}


int FakePreview::getDeviceCounts() {
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


void FakePreview::setRenderID(int renderID) {
    _renderID = renderID;

    _beginTime = std::chrono::steady_clock::now();
    _polledFrameCounts      = 0;
    _renderedFrameCounts    = 0;
    _workedFrameCounts      = 0;
}


void FakePreview::setPreviewDisplay(std::map<int, ANativeWindow *> &surfaceMap) {
    LOGD("setPreviewDisplay");
    _surfaceMap = surfaceMap;
    for (auto &itr : _surfaceMap) {
        ANativeWindow_setBuffersGeometry(itr.second, _previewWidth, _previewHeight, WINDOW_FORMAT_RGBX_8888);
    }

    //for optimization purpose
    _renderSurface = _surfaceMap.at(0);
}


bool FakePreview::enableDevices() {
    std::lock_guard<std::mutex> lock(_mutex);

    if (0 == _deviceCounts)
        return false;

    rs2::pipeline pipe;
    rs2::config cfg;
    LOGV("open bag file");
    cfg.enable_device_from_file(BAG_FILE);
    LOGV("after open bag file");

#if 0
    cfg.enable_stream(RS2_STREAM_COLOR, _previewWidth, _previewHeight, RS2_FORMAT_RGB8, _previewFPS);
    cfg.enable_stream(RS2_STREAM_DEPTH, _depthWidth, _depthHeight, RS2_FORMAT_Z16, _depthFPS);
    //cfg.enable_stream(RS2_STREAM_INFRARED, _depthWidth, _depthHeight, RS2_FORMAT_Y8, _depthFPS);
    cfg.enable_stream(RS2_STREAM_INFRARED, _depthWidth, _depthHeight, RS2_FORMAT_RGB8, _depthFPS);
#endif

    if (!cfg.can_resolve(pipe)) {
        LOGD("stream profile can't be resolved");
        #ifndef __SMART_POINTER__
            IrsaMgr *mgr = IrsaMgr::getInstance();
        #else
            std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
        #endif
        CHECK(mgr != nullptr);
        mgr->notify(IRSA_ERROR, IRSA_ERROR_PREVIEW, (size_t)"one of depth/color/ir's stream profile can't be resolved");
        return false;
    }

    std::string serial_number = "fake";
    rs2::pipeline_profile profile = pipe.start(cfg);
    _devices.emplace(serial_number, view_port{pipe, cfg, profile});

    //hardcode
    _cameraDataSize = _depthWidth * _depthHeight * 5 + _previewWidth * _previewHeight * 3 + 1;
    LOGD("_cameraDataSize %d\n", _cameraDataSize);
    LOGD("serial_number %s", serial_number.c_str());

    return true;
}


void FakePreview::disableDevices() {
    std::lock_guard<std::mutex> lock(_mutex);

    if (0 == _deviceCounts)
        return;

    for (auto &view : _devices) {
        view.second.config.disable_all_streams();
        view.second.pipe.stop();
    }

    _devices.clear();
    LOGV("destroy camera data fifo");
    _fifoPreview->destroy(_fifoPreview);
    _fifoPreview = nullptr;
}


void FakePreview::startPreview() {
    LOGD("start");

    if (0 == _deviceCounts)
        return;

    CHECK(_ctx != nullptr);

    if (!enableDevices())
        return;

    _threadPoll   = std::thread(&FakePreview::threadPoll, this);
    _threadRender = std::thread(&FakePreview::threadRender, this);
    _isRunning = true;
}


void FakePreview::stopPreview() {
    LOGD("Stop");
    if (0 == _deviceCounts)
        return;

    if (!_isRunning)
        return;
    _isRunning = false;

    if (_threadRender.joinable())
        _threadRender.join();


    if (_threadPoll.joinable())
        _threadPoll.join();


    disableDevices();
}


void FakePreview::threadPoll() {
    LOGD("device counts %d", _deviceCounts);
    LOGD("_surfacemap size %d\n", _surfaceMap.size());
    for (auto surface : _surfaceMap) {
        LOGD("surface index %d, surface %p\n", surface.first, surface.second);
    }

    _beginTime              = std::chrono::steady_clock::now();
    _polledFrameCounts      = 0;
    _renderedFrameCounts    = 0;
    _workedFrameCounts      = 0;

    rs2::frame color;
    rs2::frame depth;
    rs2::frame ir;
    rs2::frame colorize_depth;
    while (_isRunning) {
        int device_id = 0;

        for (auto &&view : _devices) {
            rs2::frameset frameset;
            if (view.second.pipe.poll_for_frames(&frameset)) {

                auto a = std::chrono::steady_clock::now();
                if (1 == _renderID) {
                    color = frameset.get_color_frame();
                    auto profile = color.get_profile().as<rs2::video_stream_profile>();
                    //1280,720, 5(RS2_FORMAT_RGB8)
                    LOGD("w %d, h %d, format %x\n", profile.width(), profile.height(), profile.format());
                    //we need allocate _fifoPreview dynamiclly
                    if (_fifoPreview == nullptr) {
                        LOGD("allocate memory for preview fifo\n");
                        _fifoPreview     = fifo_new(PREVIEW_FIFO_CAPACITY, profile.width() * profile.height() * 3);
                        _realColorWidth  = profile.width();
                        _realColorHeight = profile.height();
                    }
                }
                depth = frameset.get_depth_frame();
                if (depth) {
                    auto profile = depth.get_profile().as<rs2::video_stream_profile>();
                    //1280, 720, 1(RS2_FORMAT_Z16)
                    LOGD("w %d, h %d, format %x\n", profile.width(), profile.height(), profile.format());
                    if (_fifoPreview == nullptr) {
                        LOGD("allocate memory for preview fifo\n");
                        _fifoPreview     = fifo_new(PREVIEW_FIFO_CAPACITY, profile.width() * profile.height() * 3);
                        _realColorWidth  = profile.width();
                        _realColorHeight = profile.height();
                    }
                }
                ir = frameset.get_infrared_frame();
                if (ir) {
                    auto profile = ir.get_profile().as<rs2::video_stream_profile>();
                    //LOGD("w %d, h %d, format %x\n", profile.width(), profile.height(), profile.format());
                    if (_fifoPreview == nullptr) {
                        LOGD("allocate memory for preview fifo\n");
                        _fifoPreview     = fifo_new(PREVIEW_FIFO_CAPACITY, profile.width() * profile.height() * 3);
                        _realColorWidth  = profile.width();
                        _realColorHeight = profile.height();
                    }
                }
                if (0 == _renderID) {
                    colorize_depth = _colorizer.process(depth);
                }

                buf_element_t *frame_q = _fifoPreview->buffer_try_alloc(_fifoPreview);
                if (frame_q) {
                    if ((1 == _renderID) && color) {
                        auto vframe = color.as<rs2::video_frame>();
                        memcpy(frame_q->mem,
                               reinterpret_cast<const uint8_t *>(vframe.get_data()),
                               _realColorWidth * _realColorHeight * 3);

                        _fifoPreview->put(_fifoPreview, frame_q);
                    } else if ((0 == _renderID) && colorize_depth) {
                        auto vframe = colorize_depth.as<rs2::video_frame>();
                        memcpy(frame_q->mem,
                               reinterpret_cast<const uint8_t *>(vframe.get_data()),
                               _depthWidth * _depthHeight * 3);

                        _fifoPreview->put(_fifoPreview, frame_q);
                    } else if ((2 == _renderID) && ir) {
                        auto vframe = ir.as<rs2::video_frame>();
                        memcpy(frame_q->mem,
                               reinterpret_cast<const uint8_t *>(vframe.get_data()),
                               _depthWidth * _depthHeight * 3);

                        _fifoPreview->put(_fifoPreview, frame_q);
                    } else {
                        frame_q->free_buffer(frame_q);
                    }
                } else {
                    LOGV("dropping previewed frame");
                }


                auto b = std::chrono::steady_clock::now();
                std::chrono::microseconds delta = std::chrono::duration_cast<microseconds>(b - a);
                //LOGD("duration of poll is %d microseconds\n", delta.count());

                ++_polledFrameCounts;

                auto endTime = std::chrono::steady_clock::now();
                std::chrono::milliseconds duration = std::chrono::duration_cast<milliseconds>(
                        endTime - _beginTime);
                //LOGD("poll fps:%d", static_cast<int>(_polledFrameCounts * 1.f / duration.count() * 1000));

            } else {
                //LOGD("can't got frame from librealsense\n");
            }
            device_id++;
        }
    }
}


void FakePreview::threadRender() {

#ifndef __SMART_POINTER__
    IrsaMgr *mgr = IrsaMgr::getInstance();
#else
    std::shared_ptr<IrsaMgr> mgr = IrsaMgr::getInstance();
#endif
    CHECK(mgr != nullptr);
    int fps = 0;
    char szInfo[128];

    buf_element_t *frame_p = nullptr;
    while (_isRunning) {
        if (_fifoPreview == nullptr) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            continue;
        }

        frame_p = _fifoPreview->try_get(_fifoPreview);
        if (frame_p) {
            auto a = std::chrono::steady_clock::now();

            if (1 == _renderID) {
                ANativeWindow_Buffer buffer = {};
                ARect rect = {0, 0, 640, 480};
                if (ANativeWindow_lock(_renderSurface, &buffer, &rect) == 0) {
                    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
                    const uint8_t *in = static_cast<const uint8_t *>(frame_p->mem);
                    memset(out, 0, 640 * 480 * 4);
                    for (int y = 0; y < 360; y++) {
                        for (int x = 0; x < 640; x++) {
                            out[(y + 60) * 640 * 4 + x * 4 + 0] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 0];
                            out[(y + 60) * 640 * 4 + x * 4 + 1] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 1];
                            out[(y + 60) * 640 * 4 + x * 4 + 2] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 2];
                            out[(y + 60) * 640 * 4 + x * 4 + 3] = 0xFF;
                        }
                    }
                    ANativeWindow_unlockAndPost(_renderSurface);
                } else {
                    LOGD("ANativeWindow_lock FAILED");
                }
            }

            else if ((0 == _renderID) || ( 2 == _renderID)) {
                ANativeWindow_Buffer buffer = {};
                ARect rect = {0, 0, 640, 480};
                if (ANativeWindow_lock(_renderSurface, &buffer, &rect) == 0) {
                    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
                    const uint8_t *in = static_cast<const uint8_t *>(frame_p->mem);
                    memset(out, 0, 640 * 480 * 4);
                    for (int y = 0; y < 360; y++) {
                        for (int x = 0; x < 640; x++) {
                            out[(y + 60) * 640 * 4 + x * 4 + 0] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 0];
                            out[(y + 60) * 640 * 4 + x * 4 + 1] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 1];
                            out[(y + 60) * 640 * 4 + x * 4 + 2] = in[(y * 2) * 1280 * 3 + x * 2 * 3 + 2];
                            out[(y + 60) * 640 * 4 + x * 4 + 3] = 0xFF;
                        }
                    }
                    ANativeWindow_unlockAndPost(_renderSurface);
                } else {
                    LOGD("ANativeWindow_lock FAILED");
                }
            }


            auto b = std::chrono::steady_clock::now();
            std::chrono::microseconds delta = std::chrono::duration_cast<microseconds>(b - a);
            //LOGD("duration of renderFrame is %d microseconds\n", delta.count());

            ++_renderedFrameCounts;

            auto endTime = std::chrono::steady_clock::now();
            std::chrono::milliseconds duration = std::chrono::duration_cast<milliseconds>(endTime - _beginTime);
            fps =  static_cast<int>(_renderedFrameCounts * 1.f / duration.count() * 1000);
            //LOGD("render fps:%d", fps);
            if (0 == _renderedFrameCounts % 5) {
                snprintf(szInfo, 128, "renderedFrame %d, render FPS %d", _renderedFrameCounts, fps);
                mgr->notify(IRSA_INFO, IRSA_INFO_PREVIEW, (size_t)szInfo);
            }
            frame_p->free_buffer(frame_p);
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(100));
        }
    }
}

}
