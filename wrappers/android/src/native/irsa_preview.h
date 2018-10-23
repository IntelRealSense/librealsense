// License: Apache 2.0. See LICENSE file in root directory.

#pragma once

#include <string>
#include <map>
#include <android/native_window_jni.h>

namespace irsa {

class IrsaPreview {

public:
    IrsaPreview();
    virtual ~IrsaPreview();

public:
    virtual int  getDeviceCounts() { return 0;}
    virtual void setRenderID(int renderID) {}
    virtual void setStreamFormat(int stream, int width, int height, int fps, int format){}
    virtual void setPreviewDisplay(void *surface) { }
    virtual void setPreviewDisplay(std::map<int, ANativeWindow *> &surfaceMap) {}
    virtual void open() {}
    virtual void close() {}
    virtual void startPreview() {}
    virtual void stopPreview() {}

private:
    IrsaPreview(const IrsaPreview &);
    IrsaPreview operator =(const IrsaPreview &);
};

}
