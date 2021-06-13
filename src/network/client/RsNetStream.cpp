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

class rs_inframe {
public:
    rs_inframe(uint32_t size) : m_size(0), m_offset(0), m_total_size(0) 
    {
        // The size of the frame returned by LRS is 1K more than needed. 
        m_fb = new uint8_t[size + 1024];
    };

   ~rs_inframe() {};

    uint8_t* get_fb() {
        return m_fb;
    };

private:
    virtual uint32_t copy_data(uint8_t* chunk) {
        chunk_header_t* ch = (chunk_header_t*)chunk;

        uint32_t ret = ch->size - CHUNK_HLEN;
        memcpy((void*)(m_fb + m_offset), (void*)(chunk + CHUNK_HLEN), ret);
        return ret;
    };

public:
    bool add_chunk(uint8_t* chunk) {
        chunk_header_t* ch = (chunk_header_t*)chunk;

        if (ch->offset < m_offset) return false; // received the chunk from the new frame

        // m_sw_sensor->set_metadata((rs2_frame_metadata_value)(ch->meta_id), (rs2_metadata_type)(ch->meta_data));

        m_total_size += ch->size;
        m_offset      = ch->offset;

        uint32_t ret = copy_data(chunk);

        m_size        += ret;
        m_offset      += ret;       

        return true; 
    };

    virtual int decompress() { return 0; };

protected:
    uint8_t* m_fb;

    uint32_t m_size;
    uint32_t m_offset;
    uint32_t m_total_size;
};

class rs_inframe_jpeg : public rs_inframe {

#define RSTPER   1
#define HEAD_LEN 41

public:
    rs_inframe_jpeg(uint32_t width, uint32_t height) : rs_inframe(width * height * 2) {
        m_width  = width;
        m_height = height;

        rst = 0;

        sos = NULL;
        sos_flag = false;

        int w = 0;
        if (width / 16 * 16 == width) w = width;
        else w = (width / 16 + 1) * 16;
        
        int h = 0;
        if (height / 16 * 16 == height) h = height;
        else h = (height / 16 + 1) * 16;
        
        uint32_t rst_num = (w * h) / 256 / RSTPER; // MCU size = ((8*size_v) * (8*size_h)) = 256 because size_v = size_h = 2

        markers.resize(rst_num);
        for (int i = 0; i < rst_num; i++) markers[i] = 0;
    };

   ~rs_inframe_jpeg() {};

private:
    virtual uint32_t copy_data(uint8_t* chunk) {
        chunk_header_t* ch = (chunk_header_t*)chunk;
        uint32_t ret = ch->size - CHUNK_HLEN;

        // build the array of RST markers
        rst = ch->index; // the index of starting RST marker
        off = chunk + CHUNK_HLEN; // skip the header
        ptr = off;
        ptr++;
        marker_buffer = NULL;

        // find next markers
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

        return ret;
    };

public:
    virtual int decompress() {
        uint32_t fb_size = m_width * m_height * 2;
        uint8_t* fb_raw = new uint8_t[fb_size];

        // recreate missing headers

        u_char *p = fb_raw;
        u_char *start = p;

        /* convert from blocks to pixels */
        int w = m_width;
        int h = m_height;

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
            jpeg::decompress(fb_raw, m_total_size + (p - start), m_fb, fb_size);
            m_size = fb_size;
            memset(fb_raw, 0, fb_size);

            // // clean the markers array for debugging purposes
            // for (int i = 0; i < markers.size(); i++) {
            //     if (markers[i]) {
            //         delete [] markers[i];
            //         markers[i] = NULL;
            //     }
            // }

        } catch (...) {
            LOG_ERROR("Cannot decompress the frame, of size " << m_total_size << " to the buffer of " << fb_size);
            return 1;
        }

        return 0;
    };

private:
    uint32_t m_width;
    uint32_t m_height;

    uint32_t  rst;
    uint8_t*  off;
    uint8_t*  ptr;
    uint8_t*  sos;
    bool sos_flag;

    uint8_t*  marker_buffer;
    uint8_t*  marker_data;
    uint32_t* marker_size;

    std::vector<uint8_t*> markers;
};

class rs_inframe_data : public rs_inframe {
public:
    rs_inframe_data(uint32_t size) : rs_inframe(size) {};
   ~rs_inframe_data() {};

private:
    virtual uint32_t copy_data(uint8_t* chunk) {
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
    };
};

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

        // rs_inframe inframe = rs_inframe(m_framebuf_size, 0);
        std::shared_ptr<rs_inframe> inframe;
        if (m_profile.stream_type() == RS2_STREAM_COLOR) {
            rs2::video_stream_profile vsp = m_profile.as<rs2::video_stream_profile>();
            inframe.reset(new rs_inframe_jpeg(vsp.width(), vsp.height()));
        } else {
            inframe.reset(new rs_inframe_data(m_framebuf_size));
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
