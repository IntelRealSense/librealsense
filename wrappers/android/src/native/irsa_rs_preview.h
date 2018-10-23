// License: Apache 2.0. See LICENSE file in root directory.
//
// the RealsensePreview used to render RGB/Depth/IR data 

#pragma once

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

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

    void setRenderID(int renderID) {
        _renderID = renderID;
    }

    void setStreamFormat(int stream, int width, int height, int fps, int format) {
        _rgbWidth  = width;
        _rgbHeight = height;
        _rgbFPS    = fps;
    }

    void setPreviewDisplay(std::map<int, ANativeWindow *> &surfaceMap);

    void open();
    void close();
    void startPreview();
    void stopPreview();

private:
    void enableDevices();
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

    //RGB profile
    int _rgbWidth;
    int _rgbHeight;
    int _rgbFPS;

    //IR & Depth profile
    int _depthWidth;
    int _depthHeight;
    int _depthFPS;

    int _cameraDataSize;

    int _renderedFrameCounts;
    int _polledFrameCounts;
    int _workedFrameCounts;

    std::mutex _mutex;
    std::map<std::string, view_port> _devices;
    std::map<int, ANativeWindow *> _surfaceMap;

    ANativeWindow *_renderSurface;

    fifo_t *_framesIRDepth;
    fifo_t *_framesRGB;
};


}
