// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsNetStream.h"
#include "RsNetCommon.h"

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

rs_inframe::rs_inframe(rs_net_stream* stream, uint32_t size) : m_stream(stream), m_size(0), m_offset(0), m_total_size(0) {
    // The size of the frame returned by LRS is 1K more than needed. 
    m_fb = new uint8_t[size + 1024];

    // init the frame if requested
    if (m_stream && m_stream->m_framebuf) {
        memcpy(m_fb, m_stream->m_framebuf.get(), m_stream->m_framebuf_size);
    }
}

uint32_t rs_inframe::copy_data(uint8_t* chunk) {
    chunk_header_t* ch = (chunk_header_t*)chunk;

    uint32_t ret = ch->size - CHUNK_HLEN;
    memcpy((void*)(m_fb + m_offset), (void*)(chunk + CHUNK_HLEN), ret);
    return ret;
}

bool rs_inframe::add_chunk(uint8_t* chunk) {
    chunk_header_t* ch = (chunk_header_t*)chunk;

    if (ch->offset < m_offset) return false; // received the chunk from the new frame

    // set the metadata
    if (m_stream) {
        m_stream->m_sw_sensor->set_metadata((rs2_frame_metadata_value)(ch->meta_id), (rs2_metadata_type)(ch->meta_data));
    }

    m_total_size += ch->size;
    m_offset      = ch->offset;

    uint32_t ret = copy_data(chunk);

    m_size        += ret;
    m_offset      += ret;       

    return true; 
}

int rs_inframe::decompress() {
    // save the frame if requested
    if (m_stream && m_stream->m_framebuf) {
        memcpy(m_stream->m_framebuf.get(), m_fb, m_stream->m_framebuf_size);
    }

    return 0;
}

rs_inframe_jpeg::rs_inframe_jpeg(rs_net_stream* stream, uint32_t width, uint32_t height) : rs_inframe(stream, width * height * 2) {
    m_width  = width;
    m_height = height;

    rst = 0;

    int w = 0;
    if (width / 16 * 16 == width) w = width;
    else w = (width / 16 + 1) * 16;
    
    int h = 0;
    if (height / 16 * 16 == height) h = height;
    else h = (height / 16 + 1) * 16;
    
    uint32_t rst_num = (w * h) / 256 / RSTPER; // MCU size = ((8*size_v) * (8*size_h)) = 256 because size_v = size_h = 2

    // restore the markers' array
    if (m_stream) {
        markers = m_stream->m_private;
    }

    if (markers.size() != rst_num) {
        LOG_DEBUG("Markers' array size is wrong - resizing.");
        markers.clear();
        markers.resize(rst_num);
        for (int i = 0; i < markers.size(); i++) {
            std::shared_ptr<uint8_t> marker(new u_int8_t[MARKERSZ]);
            markers[i] = marker;
            memset(markers[i].get(), 0, MARKERSZ);
        }
    }

    fb_size = m_width * m_height * 2;
    std::shared_ptr<uint8_t> fb(new uint8_t[fb_size]);
    fb_raw = fb;
}

uint8_t* rs_inframe_jpeg::store_marker(uint32_t number, uint8_t* begin, uint8_t* end) {
    int marker_size = end - begin;
    marker_t* m = (marker_t*)markers[rst].get();
    if (marker_size + DATASIZE < MARKERSZ) {
        m->size = marker_size;
        memcpy(m->data, begin, marker_size);
    } else {
        m->size = 0;
        LOG_ERROR("Marker size is too large:" << marker_size + DATASIZE);
    }
    return end;
}

uint32_t rs_inframe_jpeg::rs_inframe_jpeg::copy_data(uint8_t* chunk) {
    chunk_header_t* ch = (chunk_header_t*)chunk;

    // build the array of RST markers
    rst = ch->index; // the index of starting RST marker
    off = chunk + CHUNK_HLEN; // skip the header
    ptr = off;
    ptr++;

    // find next markers, remember ch->size does not contains header length
    while (ptr - chunk < ch->size + CHUNK_HLEN) {
        if (*ptr == 0xFF) {
            // marker detected
            switch(*(ptr + 1)) {
            case 0xD9: // EOI
            case 0xD0: 
            case 0xD1: 
            case 0xD2: 
            case 0xD3: 
            case 0xD4: 
            case 0xD5: 
            case 0xD6: 
            case 0xD7:
                off = store_marker(rst, off, ptr);
                rst++;
            }
        }
        ptr++;
    }

    // store the last one
    store_marker(rst, off, ptr);

    return (ch->size - CHUNK_HLEN);
}

int rs_inframe_jpeg::rs_inframe_jpeg::decompress() {
    // recreate missing headers
    u_char *p = fb_raw.get();

    /* convert from blocks to pixels */
    *p++ = 0xff;
    *p++ = 0xd8;            /* SOI */

    *p++ = 0xff;
    *p++ = 0xc0;            /* SOF */
    *p++ = 0;               /* length msb */
    *p++ = 17;              /* length lsb */
    *p++ = 8;               /* 8-bit precision */
    *p++ = m_height >> 8;   /* height msb */
    *p++ = m_height;        /* height lsb */
    *p++ = m_width >> 8;    /* width msb */
    *p++ = m_width;         /* width lsb */
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
        marker_t* m = (marker_t*)markers[i].get();
        if (m->size) {
            memcpy(p, m->data, m->size);
            p += m->size;
        } else {
            if (i != 0) {
                int val = 0xd0 | (i%8);
                *p++ = 0xff;
                *p++ = val;
            }

            for (int j = 0; j < 256 * RSTPER; j++) *p++ = 0;
        }
    }

    *p++ = 0xff;
    *p++ = 0xd9; /* SOS */

    // decompress the JPEG
    try {
        jpeg::decompress(fb_raw.get(), m_total_size + (p - fb_raw.get()), m_fb, fb_size);
        m_size = fb_size;

        // // zero the targer frame buffer for debugging purposes
        // memset(fb_raw.get(), 0, fb_size);

        // // clean the markers array for debugging purposes
        // for (int i = 0; i < markers.size(); i++) {
        //     marker_t* m = (marker_t*)markers[i].get();
        //     m->size = 0;
        // }

    } catch (...) {
        LOG_ERROR("Cannot decompress the frame, of size " << m_total_size << " to the buffer of " << fb_size);
        return 1;
    }

    // save the markers' array
    if (m_stream) {
        m_stream->m_private = markers;
    }

    return 0;
}

uint32_t rs_inframe_data::copy_data(uint8_t* chunk) {
    uint32_t ret = 0;
    chunk_header_t* ch = (chunk_header_t*)chunk;

    // handle uncompressed chunk
    if ((ch->status & 3) == 0) {
        ret = ch->size - CHUNK_HLEN;
        memcpy((void*)(m_fb + m_offset), (void*)(chunk + CHUNK_HLEN), ret);
    } else {
        try {
            ret = ZSTD_decompress((void*)(m_fb + m_offset), CHUNK_SIZE, (void*)(chunk + CHUNK_HLEN), ch->size - CHUNK_HLEN);
        }
        catch (...) {
            LOG_ERROR("Error decompressing frame.");
            ret = 0;
        }
        return ret;
    }
}

void rs_net_stream::doFrames() {
    std::string m_name = "* SENSOR *";

    uint32_t fps_frame_count = 0;
    auto beginning = std::chrono::system_clock::now();

    LOG_INFO(m_name << "/" << rs2_stream_to_string(m_profile.stream_type()) << "\t: SW device support thread started");

    int frame_count = 0; 
    bool prev_sensor_state = false;

    while (m_doFrames_run) {
        if (m_queue->empty()) continue;

        // log start time for stats
        auto start = std::chrono::system_clock::now();

        std::shared_ptr<rs_inframe> inframe;
        if (m_profile.stream_type() == RS2_STREAM_COLOR) {
            rs2::video_stream_profile vsp = m_profile.as<rs2::video_stream_profile>();
            inframe.reset(new rs_inframe_jpeg(this, vsp.width(), vsp.height()));
        } else {
            inframe.reset(new rs_inframe_data(this, m_framebuf_size));
        }

        bool chunk_done = true;
        do {
            auto chunk_data = m_queue->front();
            if (chunk_data) {
                chunk_done = inframe->add_chunk(chunk_data.get());
                if (chunk_done) m_queue->pop();
            }
        }
        while (chunk_done && m_doFrames_run);

        inframe->decompress();

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::chrono::duration<double> total_time = end - beginning;
        fps_frame_count++;
        double fps;
        if (total_time.count() > 0) fps = (double)fps_frame_count / (double)total_time.count();
        else fps = 0;

        std::stringstream ss_name;
        ss_name << "Frame '" << std::setiosflags(std::ios::left);
        ss_name << std::setw(13) << m_name << " / " << rs2_stream_to_string(m_profile.stream_type());
        if (m_profile.stream_index()) ss_name << " " << m_profile.stream_index();
        ss_name << "'";

        std::stringstream ss;
        ss << std::setiosflags(std::ios::left) << std::setw(35) << ss_name.str();
        ss << std::setiosflags(std::ios::right) << std::setiosflags(std::ios::fixed) << std::setprecision(2); 
        ss << " decompression time " << std::setw(7) << elapsed.count() * 1000 << "ms, ";
        // ss << "size " << std::setw(7) << total_size << " => " << std::setw(7) << size << ", ";
        ss << "FPS: " << std::setw(7) << fps;

        LOG_DEBUG(ss.str());
        
        if (total_time > std::chrono::seconds(1)) {
            beginning = std::chrono::system_clock::now();
            fps_frame_count = 0;
        }

        // convert the data if needed
        uint8_t* final_frame = slib::convert(m_profile, inframe->get_fb());
        if (final_frame) delete [] inframe->get_fb();
        else final_frame = inframe->get_fb();

        // send it into device
        if (m_profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = m_profile.as<rs2::video_stream_profile>();
            m_sw_sensor->on_video_frame(
                { 
                    (void*)final_frame, 
                    [] (void* f) { if (f) delete [] (uint8_t*)f; }, 
                    vsp.width() * slib::get_bpp(vsp),
                    slib::get_bpp(vsp),                                                  
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    vsp
                }
            );
        } else if (m_profile.is<rs2::motion_stream_profile>()){
            rs2::motion_stream_profile msp = m_profile.as<rs2::motion_stream_profile>();
            m_sw_sensor->on_motion_frame(
                { 
                    (void*)final_frame, 
                    [] (void* f) { if (f) delete [] (uint8_t*)f; }, 
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    msp
                }
            );
        } else throw std::runtime_error("Unknown profile on frame departure.");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10)); 
    LOG_INFO(m_name << "/" << rs2_stream_to_string(m_profile.stream_type()) << "\t: SW device support thread exited");
}
