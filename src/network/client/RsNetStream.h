// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <option.h>
#include <software-device.h>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>
#include <sstream>

#include "RsStreamLib.h"
#include "RsSafeQueue.h"

class rs_net_stream; // forward

class rs_inframe {
public:
    rs_inframe(rs_net_stream* stream, uint32_t size);
   ~rs_inframe() {};

    uint8_t* get_fb() { return m_fb; };

private:
    virtual uint32_t copy_data(uint8_t* chunk);

public:
    bool add_chunk(uint8_t* chunk);
    virtual int decompress();

protected:
    rs_net_stream* m_stream;

    uint8_t* m_fb;

    uint32_t m_size;
    uint32_t m_offset;
    uint32_t m_total_size;
};

class rs_inframe_jpeg : public rs_inframe {

#define RSTPER   1
#define HEAD_LEN 41
#define MARKERSZ (256 * (RSTPER + 1))
#define DATASIZE (sizeof(uint32_t))

typedef struct marker {
    uint32_t size;
    uint8_t  data[MARKERSZ - sizeof(uint32_t)];
} marker_t;

public:
    rs_inframe_jpeg(rs_net_stream* stream, uint32_t width, uint32_t height);
    ~rs_inframe_jpeg() {};

private:
    virtual uint32_t copy_data(uint8_t* chunk);
    uint8_t* store_marker(uint32_t number, uint8_t* begin, uint8_t* end); // returns end

public:
    virtual int decompress();

private:
    uint32_t m_width;
    uint32_t m_height;

    uint32_t  rst;
    uint8_t*  off;
    uint8_t*  ptr;

    std::vector<std::shared_ptr<uint8_t>> markers;

    uint32_t fb_size;
    std::shared_ptr<uint8_t> fb_raw;
};

class rs_inframe_data : public rs_inframe {
public:
    rs_inframe_data(rs_net_stream* stream, uint32_t size) : rs_inframe(stream, size) {};
   ~rs_inframe_data() {};

private:
    virtual uint32_t copy_data(uint8_t* chunk);
};

class rs_net_stream {

friend class rs_inframe;
friend class rs_inframe_jpeg;
friend class rs_inframe_data;

public:
    rs_net_stream(SoftSensor sw_sensor, rs2::stream_profile sp) : m_profile(sp), m_sw_sensor(sw_sensor) {
        m_framebuf_size = 32; // default size for motion module frames

        if (m_profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = m_profile.as<rs2::video_stream_profile>();
            int bpp = vsp.stream_type() == RS2_STREAM_INFRARED ? 1 : 2;
            m_framebuf_size = vsp.width() * vsp.height() * bpp;
        } 

        std::shared_ptr<uint8_t> framebuf(new uint8_t[m_framebuf_size]);
        m_framebuf = framebuf;

        std::shared_ptr<SafeQueue> net_queue(new SafeQueue);
        m_queue = net_queue;
    };

    ~rs_net_stream() {};

    void start() {
        m_doFrames_run = true;
        m_doFrames = std::thread( [&](){ doFrames(); });
    };

    void stop() {
        m_doFrames_run = false;
        if (m_doFrames.joinable()) m_doFrames.join();
    };

    rs2::stream_profile        get_profile() { return m_profile; };
    std::shared_ptr<SafeQueue> get_queue()   { return m_queue;   };

private:
    bool                        m_doFrames_run;
    std::thread                 m_doFrames;
    void doFrames();

    SoftSensor                  m_sw_sensor;

    rs2::stream_profile         m_profile;
    std::shared_ptr<SafeQueue>  m_queue;

    // Buffer to compose the frame from incoming chunks
    std::shared_ptr<uint8_t>    m_framebuf; 
    uint32_t                    m_framebuf_size;

    // Markers' array to initialize the new frames
    std::vector<std::shared_ptr<uint8_t>>       m_private;
};
