// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include <BasicUsageEnvironment.hh>

#include "RsSensor.h"

#include <librealsense2/rs.hpp>

///
#include <thread>
#include <chrono>
#include <iostream>

class RsDeviceSource : public FramedSource
{
public:
    static RsDeviceSource* createNew(UsageEnvironment& t_env, frames_queue* pfq, rs2::stream_profile stream) {
        return new RsDeviceSource(t_env, pfq, stream);
    };

    virtual char const* MIMEtype() const { return "video/RS"; };

protected:
    RsDeviceSource(UsageEnvironment& t_env, frames_queue* pfq, rs2::stream_profile stream) 
        : FramedSource(t_env), m_queue(pfq), m_stream(stream) 
    {
        m_queue->addStream(m_stream);
        std::cout << "Source started for stream: " << slib::print_profile(m_stream) << std::endl;
    };

    virtual ~RsDeviceSource() 
    {
        m_queue->delStream(m_stream);
        std::cout << "Source stopped for stream: " << slib::print_profile(m_stream) << std::endl;
    };

protected:
    virtual void doStopGettingFrames() { FramedSource::doStopGettingFrames(); m_queue->stop((void*)this); };

private:
    virtual void doGetNextFrame() {
        if (isCurrentlyAwaitingData()) {
            FrameData frame = m_queue->get_frame((void*)this, m_stream);
            if (frame != nullptr) {
                // we have got the data
                gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
                fFrameSize = ((chunk_header_t*)frame.get())->size;
                memcpy(fTo, frame.get(), fFrameSize);                
                afterGetting(this); // After delivering the data, inform the reader that it is now available:
            } else {
                // return here after 1ms
                nextTask() = envir().taskScheduler().scheduleDelayedTask(1000, (TaskFunc*)waitForFrame, this);
            }
        } else {
            std::cout << "Attempting to read the frame out of band" << std::endl;
        }
    };

    static void waitForFrame(RsDeviceSource* t_deviceSource) {
        //// std::cout << "RsDeviceSource::waitForFrame" << std::endl;
        t_deviceSource->doGetNextFrame();
    };


private:
    frames_queue*       m_queue;
    rs2::stream_profile m_stream;
};
