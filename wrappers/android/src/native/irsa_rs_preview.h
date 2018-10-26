// License: Apache 2.0. See LICENSE file in root directory.
//
// the RealsensePreview used to render RGB/Depth/IR data

#pragma once

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>

#include <inttypes.h>
#include <android/native_window_jni.h>

#include "util.h"
#include "fifo.h"
#include "cde_log.h"
#include "irsa_preview.h"

namespace irsa {

class RealsensePreview : public IrsaPreview {

    struct view_port {
        rs2::pipeline           pipe;
        rs2::config             config;
        rs2::pipeline_profile   profile;
    };

public:
    RealsensePreview();
    virtual ~RealsensePreview();

    int getDeviceCounts();

    void setRenderID(int renderID);

    void setStreamFormat(int stream, int width, int height, int fps, int format) {
        _previewWidth  = width;
        _previewHeight = height;
        _previewFPS    = fps;
    }

    void setPreviewDisplay(std::map<int, ANativeWindow *> &surfaceMap);

    void open();
    void close();
    void startPreview();
    void stopPreview();

private:
    bool enableDevices();
    void disableDevices();

    void threadPoll();
    void threadRender();
    void threadWorker();

private:
    std::shared_ptr<rs2::context> _ctx;
    rs2::colorizer _colorizer;

    std::thread _threadPoll;
    std::thread _threadWorker;
    std::thread _threadRender;

    bool _isRunning;
    int _deviceCounts;
    int _renderID;

    //profile of preview stream
    int _previewWidth;
    int _previewHeight;
    int _previewFPS;

    //profile of IR & Depth stream
    int _depthWidth;
    int _depthHeight;
    int _depthFPS;

    int _cameraDataSize;

    int _renderedFrameCounts;
    int _polledFrameCounts;
    int _workedFrameCounts;

    std::chrono::steady_clock::time_point _beginTime;

    std::mutex _mutex;
    std::map<std::string, view_port> _devices;
    std::map<int, ANativeWindow *> _surfaceMap;



    ANativeWindow *_renderSurface;

    fifo_t *_fifoIRDepth;
    fifo_t *_fifoPreview;
};


}
