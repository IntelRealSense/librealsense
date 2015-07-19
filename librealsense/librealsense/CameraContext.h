#ifndef CAMERA_CONTEXT_H
#define CAMERA_CONTEXT_H

#include <stdint.h>
#include <string>
#include <vector>

enum PixelFormat
{
    // ...
};

struct StreamInfo
{
    int width;
    int height;
    int fps;
    PixelFormat fmt;
};

struct StreamHandle
{
    // ...
};

class CameraContext
{
    // Initialize libusb context
    CameraContext();
    
    // Stops open streams, closes devices, closes libusb
    ~CameraContext();
    
    // Probes currently connected USB3 cameras and returns an array of device ids
    static std::vector<int> QueryDeviceList();
    
    // Gets informational data about a device idx in QueryDeviceList(). F200 / R200.
    void GetDeviceInfo(int dev);
    
    // Opens the device from idx. Returns true if the device was opened.
    bool OpenDevice(int dev);
    
    // Finds supported info
    std::vector<StreamInfo> QuerySupportedStreams(int dev);
    
    // Configure stream in a specific mode. Returns true if the stream was configured.
    bool ConfigureStream(int dev, StreamInfo inf);
    
    // Starts the stream.
    const StreamHandle & StartStream(int dev, StreamInfo inf);
    
    // Stops the stream
    void StopStream(int dev);

};

bool QueryCalibrationExtrinsics(const StreamHandle & h);

bool QueryCalibrationIntrinsics(const StreamHandle & h);

int GetStreamStatus(const StreamHandle & h);

int GetCameraSerial(const StreamHandle & h);

// Etc
float GetStreamGain(const StreamHandle & h);
bool SetStreamGain(float gain, const StreamHandle & h);

#endif
