// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <httplib.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "RsNetDevice.h"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

#include <zstd.h>
#include <zstd_errors.h>

#include <lz4.h>
#include <jpeg.h>

#include <stdlib.h>
#include <math.h>

using namespace std::placeholders;

void rs_net_sensor::doRTP() {
    std::stringstream ss;
    ss << std::setiosflags(std::ios::left) << std::setw(14) << m_name << ": RTP support thread started" << std::endl;
    std::cout << ss.str();

    TaskScheduler* scheduler = BasicTaskScheduler::createNew(/* 1000 */); // Check this later
    m_env = BasicUsageEnvironment::createNew(*scheduler);

    // Start the watch thread  
    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);

    // Start the scheduler
    m_env->taskScheduler().doEventLoop(&m_eventLoopWatchVariable);

    std::cout << m_name << " : RTP support thread exited" << std::endl;
}

void rs_net_sensor::doControl() {
    bool streaming = is_streaming();
    if (streaming != m_streaming) {
        // sensor state changed
        m_streaming = streaming;
        if (is_streaming()) {
            std::cout << "Sensor enabled\n";

            // Create RTSP client
            RTSPClient::responseBufferSize = 100000;
            m_rtspClient = RSRTSPClient::createNew(*m_env, m_mrl.c_str());
            if (m_rtspClient == NULL) {
                std::cout << "Failed to create a RTSP client for URL '" << m_mrl << "': " << m_env->getResultMsg() << std::endl;
                throw std::runtime_error("Cannot create RTSP client");
            }
            std::cout << "Connected to " << m_mrl << std::endl;

            // Prepare profiles list and allocate the queues
            m_streams.clear();
            // m_dev_flag = true;
            for (auto stream_profile : m_sw_sensor->get_active_streams()) {
                rs_net_stream* net_stream = new rs_net_stream(stream_profile);
                uint64_t key = slib::profile2key(net_stream->profile);
                m_streams[key] = net_stream;
            }
            
            // Start playing streams
            m_rtspClient->playStreams(m_streams);

            // Start SW device thread
            m_dev_flag = true;
            for (auto ks : m_streams) {
                rs_net_stream* net_stream = ks.second;
                net_stream->thread = std::thread( [&](){ doDevice(ks.first); });
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            }
            // m_dev = std::thread( [&](){ doDevice(); });
        } else {
            std::cout << "Sensor disabled\n";

            // Stop SW device thread
            m_dev_flag = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            for (auto ks : m_streams) {
                auto net_stream = ks.second;
                if (net_stream->thread.joinable()) net_stream->thread.join();
            }

            // disable running RTP sessions
            m_rtspClient->shutdownStream();
            m_rtspClient = NULL;

            // remove the queues and their content
            for (auto ks : m_streams) {
                auto net_stream = ks.second;
                while (!net_stream->queue->empty()) {
                    delete net_stream->queue->front();
                    net_stream->queue->pop();
                }
                // delete net_stream->queue;
                delete net_stream;
            }
            m_streams.clear();
        }
    }

    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);
}

// uint32_t chunks_allocated = 0;

void rs_net_sensor::doDevice(uint64_t key) {

    uint32_t fps_frame_count = 0;
    auto beginning = std::chrono::system_clock::now();

#define RSTPER 1
    uint32_t rst_num = 0;

    uint32_t frame_size;
    rs_net_stream* net_stream = m_streams[key];
    if (net_stream->profile.is<rs2::video_stream_profile>()) {
        rs2::video_stream_profile vsp = net_stream->profile.as<rs2::video_stream_profile>();
        uint32_t bpp = (vsp.stream_type() == RS2_STREAM_INFRARED) ? 1 : 2; // IR:1 COLOR and DEPTH:2
        frame_size = vsp.width() * vsp.height() * bpp;

        int w = 0;
        if (vsp.width() / 16 * 16 == vsp.width()) w = vsp.width();
        else w = (vsp.width() / 16 + 1) * 16;
        
        int h = 0;
        if (vsp.height() / 16 * 16 == vsp.height()) h = vsp.height();
        else h = (vsp.height() / 16 + 1) * 16;
        
        rst_num = (w * h) / 256 / RSTPER; // MCU size = ((8*size_v) * (8*size_h)) = 256 because size_v = size_h = 2
    } else if (net_stream->profile.is<rs2::motion_stream_profile>()) {
        frame_size = 32;
    } else throw std::runtime_error("Unknown profile on SW device support thread start.");

    std::vector<uint8_t*> markers(rst_num);
    for (int i = 0; i < rst_num; i++) markers[i] = 0;

    // rs2_video_stream s = slib::key2stream(key);
    std::cout << m_name << "/" << rs2_stream_to_string(net_stream->profile.stream_type()) << "\t: SW device support thread started" << std::endl;

    int frame_count = 0; 
    bool prev_sensor_state = false;

    while (m_dev_flag) {
        if (net_stream->queue->empty()) continue;

        auto start = std::chrono::system_clock::now();

        uint32_t size = 0;
        uint32_t offset = 0;
        uint32_t total_size = 0;

        uint32_t  rst = 0;
        uint8_t*  off;
        uint8_t*  ptr;
        uint8_t*  sos = NULL;
        bool sos_flag = false;

        uint8_t*  marker_buffer;
        uint8_t*  marker_data;
        uint32_t* marker_size;
        
        while (offset < frame_size) {
            uint8_t* data = 0;
            do {
                if (!m_dev_flag) goto out;
                data = net_stream->queue->front();
            } while (data == 0);
            chunk_header_t* ch = (chunk_header_t*)data;

            if (ch->offset < offset) break;

            m_sw_sensor->set_metadata((rs2_frame_metadata_value)(ch->meta_id), (rs2_metadata_type)(ch->meta_data));

            total_size += ch->size;
            offset = ch->offset;
            int ret = 0;
            switch (ch->status & 3) {
            case 0:
                ret = ch->size - CHUNK_HLEN;
                memcpy((void*)(net_stream->m_frame_raw + offset), (void*)(data + CHUNK_HLEN), ret);
                break;
            case 1: 
                ret = ZSTD_decompress((void*)(net_stream->m_frame_raw + offset), CHUNK_SIZE, (void*)(data + CHUNK_HLEN), ch->size - CHUNK_HLEN); 
                break;
            case 2: 
                ret = LZ4_decompress_safe((const char*)(data + CHUNK_HLEN), (char*)(net_stream->m_frame_raw + offset), ch->size - CHUNK_HLEN, CHUNK_SIZE);
                ret = ch->size - CHUNK_HLEN;
                break;
            case 3:
#define HEAD_LEN 41
                // std::cout << "JPEG not implemented yet" << std::endl;
                ret = ch->size - CHUNK_HLEN;

                // build the array of RST markers
                rst = ch->index; // the index of starting RST marker
                off = data + CHUNK_HLEN; // skip the header
                ptr = off;
                ptr++;
                marker_buffer = NULL;

                // find next markers
                while (ptr - data < ch->size + CHUNK_HLEN) {
                    if (*ptr == 0xFF) {
                        // marker detected
                        switch(*(ptr + 1)) {
                        case 0xD9: //std::cout << " EOI : " << std::endl;
                        case 0xD0: 
                        case 0xD1: 
                        case 0xD2: 
                        case 0xD3: 
                        case 0xD4: 
                        case 0xD5: 
                        case 0xD6: 
                        case 0xD7: 
                            marker_buffer = new uint8_t[ptr - off  + sizeof(uint32_t)];
                            marker_size = (uint32_t*)marker_buffer;
                            marker_data = marker_buffer + sizeof(uint32_t);

                            *marker_size = ptr - off;
                            memcpy(marker_data, off, ptr - off);

                            if (markers[rst]) delete [] markers[rst];
                            markers[rst] = marker_buffer;
                            off = ptr;
                            rst++;
                        }
                    }
                    ptr++;
                }

                marker_buffer = new uint8_t[ptr - off  + sizeof(uint32_t)];
                marker_size = (uint32_t*)marker_buffer;
                marker_data = marker_buffer + sizeof(uint32_t);

                *marker_size = ptr - off;
                memcpy(marker_data, off, ptr - off);

                if (markers[rst]) delete [] markers[rst];
                markers[rst] = marker_buffer;
                off = ptr;
                rst++;

                break;
            }
            size += ret;
            // offset += CHUNK_SIZE;
            offset += ret;

            net_stream->queue->pop();
            delete [] data;
        } 

        uint8_t* frame_raw = new uint8_t[frame_size];
        if (net_stream->profile.stream_type() == RS2_STREAM_COLOR) {
            rs2::video_stream_profile vsp = net_stream->profile.as<rs2::video_stream_profile>();
            // recreate missing headers

            u_char *p = net_stream->m_frame_raw;
            u_char *start = p;

            /* convert from blocks to pixels */
            int w = vsp.width();
            int h = vsp.height();

            *p++ = 0xff;
            *p++ = 0xd8;            /* SOI */

            *p++ = 0xff;
            *p++ = 0xc0;            /* SOF */
            *p++ = 0;               /* length msb */
            *p++ = 17;              /* length lsb */
            *p++ = 8;               /* 8-bit precision */
            *p++ = h >> 8;          /* height msb */
            *p++ = h;               /* height lsb */
            *p++ = w >> 8;          /* width msb */
            *p++ = w;               /* wudth lsb */
            *p++ = 3;               /* number of components */
            *p++ = 0;               /* comp 0, should be 1? */ 
            *p++ = 0x22;            /* hsamp = 2, vsamp = 2 */
            *p++ = 0;               /* quant table 0 */
            *p++ = 1;               /* comp 1, should be 2? */
            *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
            *p++ = 1;               /* quant table 1 */
            *p++ = 2;               /* comp 2, should be 3? */
            *p++ = 0x11;            /* hsamp = 1, vsamp = 1 */
            *p++ = 1;               /* quant table 1 */

            u_short dri = RSTPER;
            *p++ = 0xff;
            *p++ = 0xdd;            /* DRI */
            *p++ = 0x0;             /* length msb */
            *p++ = 4;               /* length lsb */
            *p++ = dri >> 8;        /* dri msb */
            *p++ = dri & 0xff;      /* dri lsb */

            *p++ = 0xff;
            *p++ = 0xda;            /* SOS */
            *p++ = 0;               /* length msb */
            *p++ = 12;              /* length lsb */
            *p++ = 3;               /* 3 components */
            *p++ = 0;               /* comp 0 */
            *p++ = 0;               /* huffman table 0 */
            *p++ = 1;               /* comp 1 */
            *p++ = 0x11;            /* huffman table 1 */
            *p++ = 2;               /* comp 2 */
            *p++ = 0x11;            /* huffman table 1 */
            *p++ = 0;               /* first DCT coeff */
            *p++ = 63;              /* last DCT coeff */
            *p++ = 0;               /* sucessive approx. */

            for (int i = 0; i < markers.size(); i++) {
                marker_buffer = markers[i];
                if (marker_buffer == NULL) {
                    if (i != 0) {
                        int val = 0xd0 | (i%8);
                        *p++ = 0xff;
                        *p++ = val;
                    }

                    for (int j = 0; j < 256 * RSTPER; j++) *p++ = 0;
                } else {
                    marker_size = (uint32_t*)marker_buffer;
                    marker_data = marker_buffer + sizeof(uint32_t);

                    memcpy(p, marker_data, *marker_size);
                    p += *marker_size;
                }
            }

            *p++ = 0xff;
            *p++ = 0xd9; /* SOS */

            // decompress the JPEG
            try {
                jpeg::decompress(net_stream->m_frame_raw, total_size + (p - start), frame_raw, frame_size);
                size = frame_size;
                memset(net_stream->m_frame_raw, 0, frame_size);

                // // clean the markers array for debugging purposes
                // for (int i = 0; i < markers.size(); i++) {
                //     if (markers[i]) {
                //         delete [] markers[i];
                //         markers[i] = NULL;
                //     }
                // }

            } catch (...) {
                std::cout << "Cannot decompress the frame, of size " << total_size << " to the buffer of " << frame_size << std::endl;
            }
        } else {
            memcpy(frame_raw, net_stream->m_frame_raw, frame_size);
        }

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::chrono::duration<double> total_time = end - beginning;
        fps_frame_count++;
        double fps;
        if (total_time.count() > 0) fps = (double)fps_frame_count / (double)total_time.count();
        else fps = 0;

        std::stringstream ss_name;
        ss_name << "Frame '" << std::setiosflags(std::ios::left);
        ss_name << std::setw(13) << m_name << " / " << rs2_stream_to_string(net_stream->profile.stream_type());
        if (net_stream->profile.stream_index()) ss_name << " " << net_stream->profile.stream_index();
        ss_name << "'";

        std::stringstream ss;
        ss << std::setiosflags(std::ios::left) << std::setw(35) << ss_name.str();
        ss << std::setiosflags(std::ios::right) << std::setiosflags(std::ios::fixed) << std::setprecision(2); 
        ss << " decompression time " << std::setw(7) << elapsed.count() * 1000 << "ms, ";
        ss << "size " << std::setw(7) << total_size << " => " << std::setw(7) << size << ", ";
        ss << "FPS: " << std::setw(7) << fps << std::endl;

        std::cout << ss.str();
        
        if (total_time > std::chrono::seconds(1)) {
            beginning = std::chrono::system_clock::now();
            fps_frame_count = 0;
        }

        // std::cout << "Chunks: " << chunks_allocated << "\n"; 

        // send it into device
        uint8_t* frame_raw_converted = frame_raw;
        if (net_stream->profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = net_stream->profile.as<rs2::video_stream_profile>();

            // set bpp
            int bpp = 1;
            switch(vsp.format()) {
            case RS2_FORMAT_Z16   : bpp = 2; break;
            case RS2_FORMAT_YUYV  : bpp = 2; break;
            case RS2_FORMAT_Y8    : bpp = 1; break;
            case RS2_FORMAT_UYVY  : bpp = 2; break;
            case RS2_FORMAT_RGB8  : bpp = 3; break;
            case RS2_FORMAT_BGR8  : bpp = 3; break;
            case RS2_FORMAT_RGBA8 : bpp = 4; break;
            case RS2_FORMAT_BGRA8 : bpp = 4; break;
            default: bpp = 0;
            }

            // convert the format if necessary
            switch(vsp.format()) {
            case RS2_FORMAT_RGB8  :
            case RS2_FORMAT_BGR8  :
                frame_raw_converted = new uint8_t[vsp.width() * vsp.height() * bpp];
                // perform the conversion
                for (int y = 0; y < vsp.height(); y++) {
                    for (int x = 0; x < vsp.width(); x += 2) {                
                        {
                            uint8_t Y = frame_raw[y * vsp.width() * 2 + x * 2 + 0];
                            uint8_t U = frame_raw[y * vsp.width() * 2 + x * 2 + 1];
                            uint8_t V = frame_raw[y * vsp.width() * 2 + x * 2 + 3];

                            uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                            uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                            uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 0] = R;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 1] = G;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 2] = B;
                        }

                        {
                            uint8_t Y = frame_raw[y * vsp.width() * 2 + x * 2 + 2];
                            uint8_t U = frame_raw[y * vsp.width() * 2 + x * 2 + 1];
                            uint8_t V = frame_raw[y * vsp.width() * 2 + x * 2 + 3];

                            uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                            uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                            uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 3] = R;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 4] = G;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 5] = B;
                        }                        
                    }
                }
                delete [] frame_raw;
                break;
            case RS2_FORMAT_RGBA8 :
            case RS2_FORMAT_BGRA8 :
                frame_raw_converted = new uint8_t[vsp.width() * vsp.height() * bpp];
                // perform the conversion
                for (int y = 0; y < vsp.height(); y++) {
                    for (int x = 0; x < vsp.width(); x += 2) {                
                        {
                            uint8_t Y = frame_raw[y * vsp.width() * 2 + x * 2 + 0];
                            uint8_t U = frame_raw[y * vsp.width() * 2 + x * 2 + 1];
                            uint8_t V = frame_raw[y * vsp.width() * 2 + x * 2 + 3];

                            uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                            uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                            uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 0] = R;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 1] = G;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 2] = B;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 3] = 0xFF;
                        }

                        {
                            uint8_t Y = frame_raw[y * vsp.width() * 2 + x * 2 + 2];
                            uint8_t U = frame_raw[y * vsp.width() * 2 + x * 2 + 1];
                            uint8_t V = frame_raw[y * vsp.width() * 2 + x * 2 + 3];

                            uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                            uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                            uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 4] = R;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 5] = G;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 6] = B;
                            frame_raw_converted[y * vsp.width() * bpp + x * bpp + 7] = 0xFF;
                        }                        
                    }
                }
                delete [] frame_raw;
                break;
            }

            // send the frame
            m_sw_sensor->on_video_frame(
                { 
                    (void*)frame_raw_converted, 
                    [] (void* f) { delete [] (uint8_t*)f; }, 
                    vsp.width() * bpp,
                    bpp,                                                  
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    vsp
                }
            );
        } else if (net_stream->profile.is<rs2::motion_stream_profile>()){
            rs2::motion_stream_profile msp = net_stream->profile.as<rs2::motion_stream_profile>();
            m_sw_sensor->on_motion_frame(
                { 
                    (void*)frame_raw_converted, 
                    [] (void* f) { delete [] (uint8_t*)f; }, 
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    msp
                }
            );
        } else throw std::runtime_error("Unknown profile on frame departure.");
    }

out:
    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10)); 
    std::cout << m_name << "/" << rs2_stream_to_string(net_stream->profile.stream_type()) << "\t: SW device support thread exited" << std::endl;
}
