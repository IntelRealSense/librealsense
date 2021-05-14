// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "RsNetCommon.h"
#include "RsStreamLib.h"

#include <iostream>
#include <iomanip>
#include <queue>
#include <map>
#include <chrono>
#include <memory>
#include <utility>
#include <mutex>

#include <librealsense2/rs.hpp>

#include <zstd.h>
#include <zstd_errors.h>

#include <lz4.h>
#include <jpeg.h>

using FrameData        = std::shared_ptr<uint8_t[]>;
using FrameDataQ       = std::shared_ptr<std::queue<FrameData>>;
using StreamFrameDataQ = std::map<uint64_t, std::shared_ptr<std::queue<FrameData>>>;
using ConsumerQMap     = std::map<void*, StreamFrameDataQ>;
using ConsumerQPair    = std::pair<void*, StreamFrameDataQ>;

using MetaMap          = std::map<uint8_t, rs2_metadata_type>;

////////////////////////////////////////////////////////////

class frames_queue {
public:
    frames_queue(rs2::sensor sensor)
        : m_sensor(sensor) {};
   ~frames_queue() { 
        for (ConsumerQMap::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
            stop(it->first);
    }; // TODO: improve

    void addStream(rs2::stream_profile stream) {
        m_streams[slib::profile2key(stream)] = stream;
    };
    
    void delStream(rs2::stream_profile stream) {
        m_streams.erase(slib::profile2key(stream));
    };
    
    bool is_streaming(rs2::stream_profile stream) { 
        uint64_t key = slib::profile2key(stream);

        auto streams = m_sensor.get_active_streams();
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            if (key == slib::profile2key(*it)) return true;
        }

        return false;
    };
    
    std::string get_name() { return m_sensor.get_info(RS2_CAMERA_INFO_NAME); };

    FrameData get_frame(void* consumer, rs2::stream_profile stream)  {
        static std::map<uint64_t, uint32_t> frame_count; 
        uint64_t key = slib::profile2key(stream);

        if (m_queues.find(consumer) == m_queues.end()) {
            // new consumer - allocate the queue for it
            StreamFrameDataQ sfdq;
            for (auto kp : m_streams) {
                FrameDataQ q(new std::queue<FrameData>);
                sfdq[kp.first] = q;
            }
            m_queues.insert(ConsumerQPair(consumer, sfdq));
        }

        if (!is_streaming(stream)) {
            // create vector of profiles and open the sensor
            std::vector<rs2::stream_profile> profiles;
            for (auto kp : m_streams) {
                profiles.emplace_back(kp.second);
                frame_count[kp.first] = 0;
            }
            m_sensor.open(profiles);

            auto callback = [&](const rs2::frame& frame) {
                MetaMap meta_attrs;

                // Record all the available metadata attributes
                for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++) {
                    if (frame.supports_frame_metadata((rs2_frame_metadata_value)i)) {
                        meta_attrs.emplace(std::make_pair(i, frame.get_frame_metadata((rs2_frame_metadata_value)i)));
                    }
                }
                MetaMap::iterator meta_attr = meta_attrs.begin();

                rs2::stream_profile frame_profile = frame.get_profile();
                uint64_t profile_key = slib::profile2key(frame_profile);
                static std::chrono::system_clock::time_point beginning = std::chrono::system_clock::now();

                auto start = std::chrono::system_clock::now();

                uint8_t* data = NULL;;
                uint32_t size = 0;

                uint32_t offset = 0;
                uint32_t out_size = 0;

                if (frame_profile.stream_type() == RS2_STREAM_COLOR) {
                    // convert the YUYV encoded frame to the JPEG encoded frame
                    data = new uint8_t[frame.get_data_size()];
                    size = jpeg::compress((uint8_t*)frame.get_data(), frame_profile.as<rs2::video_stream_profile>().width(), frame_profile.as<rs2::video_stream_profile>().height(), data, frame.get_data_size());

                    uint8_t * ptr = data;
                    uint8_t * off = ptr;
                    uint8_t * rst = NULL;

                    uint32_t marks = 0;
                    uint32_t total_marks = 0;
                    uint32_t start_marks = 0;

                    // find the SOS marker
                    do {
                        if ( (*ptr == 0xFF) && (*(ptr + 1) == 0xDA) ) break;
                    } while (++ptr - data < size);

                    if (ptr - data == size) {
                        // no SOS marker found
                        LOG_ERROR("No SOS marker found");
                    } else {
                        // SOS found, jump to the data
                        ptr = ptr + *(ptr+3) + 2;
                        rst = off = ptr++;
                        chunk_header_t ch = {0};
                        do {
                            if (*ptr == 0xFF) {
                                // marker detected
                                switch (*(ptr + 1)) {
                                case 0xD9:
                                case 0xD0: 
                                case 0xD1: 
                                case 0xD2: 
                                case 0xD3: 
                                case 0xD4: 
                                case 0xD5: 
                                case 0xD6: 
                                case 0xD7: 
                                    if ((ch.size + (ptr - off) > CHUNK_SIZE) || (*(ptr + 1) == 0xD9)) {
                                        // chunk is ready - combine and send it
                                        ch.offset = rst - data;
                                        rst = off;

                                        ch.index = start_marks;
                                        start_marks = total_marks;
                                        ch.status = ch.status | 0x3; // set lower two bits - JPEG compression

                                        ch.meta_id = meta_attr->first;
                                        ch.meta_data = meta_attr->second;
                                        meta_attr++;
                                        if (meta_attr == meta_attrs.end()) meta_attr = meta_attrs.begin();

                                        FrameData chunk(new uint8_t[ch.size + CHUNK_HLEN]);
                                        chunk_header_t* chp = (chunk_header_t*)chunk.get();
                                        *chp = ch;
                                        memcpy((void*)(chunk.get() + CHUNK_HLEN), (void*)(data + ch.offset), ch.size);
                                        out_size  += ch.size;

                                        // push the chunk to queues
                                        for (ConsumerQMap::iterator it = m_queues.begin(); it != m_queues.end(); ++it) 
                                        {
                                            std::lock_guard<std::mutex> lck (m_queues_mutex);
                                            it->second[profile_key]->push(chunk);
                                        }

                                        ch.size = 0;
                                    }

                                    total_marks++;
                                    ch.size = ch.size + (ptr - off);
                                    off = ptr;

                                    if (*(ptr + 1) == 0xD9) {
                                        // last chunk of the frame - send it
                                        ch.offset = rst - data;
                                        rst = off;

                                        ch.index = start_marks;
                                        start_marks = total_marks;
                                        ch.status = ch.status | 0x3; // set lower two bits - JPEG compression

                                        ch.meta_id = meta_attr->first;
                                        ch.meta_data = meta_attr->second;
                                        meta_attr++;
                                        if (meta_attr == meta_attrs.end()) meta_attr = meta_attrs.begin();

                                        FrameData chunk(new uint8_t[ch.size + CHUNK_HLEN]);
                                        chunk_header_t* chp = (chunk_header_t*)chunk.get();
                                        *chp = ch;
                                        memcpy((void*)(chunk.get() + CHUNK_HLEN), (void*)(data + ch.offset), ch.size);
                                        out_size  += ch.size;

                                        // push the chunk to queues
                                        for (ConsumerQMap::iterator it = m_queues.begin(); it != m_queues.end(); ++it) 
                                        {
                                            std::lock_guard<std::mutex> lck (m_queues_mutex);
                                            it->second[profile_key]->push(chunk);
                                        }

                                        ch.size = 0;
                                    }

                                    break;
                                }
                            }
                        } while (++ptr - data < size);
                    }
                    size = frame.get_data_size();
                    // free the JPEG encoded frame
                    delete [] data;
                } else {
                    data = (uint8_t*)frame.get_data();
                    size = frame.get_data_size();

                    // slice frame into chunks
                    while (offset < size) {
                        FrameData chunk(new uint8_t[CHUNK_SIZE + CHUNK_HLEN]);
                        chunk_header_t* ch = (chunk_header_t*)chunk.get();
                        ch->offset = offset;
                        ch->size   = sizeof(chunk_header_t);
                        int ret = 0;
                        int csz = size - offset > CHUNK_SIZE ? CHUNK_SIZE : size - offset;

                        #ifdef COMPRESSION_ENABLED
                            #ifdef COMPRESSION_ZSTD                    
                                ret = ZSTD_compress((void*)(chunk.get() + CHUNK_HLEN), CHUNK_SIZE, (void*)(data + offset), csz, 1);
                                // if the compressed chunk sometimes bigger than original just copy uncompressed data
                                if (ret < 0) {
                                    ch->status = ch->status & 0xFC; // clean lower two bits - no compression
                                    memcpy((void*)(chunk.get() + CHUNK_HLEN), (void*)(data + offset), csz);
                                    ret = csz;
                                } else {
                                    ch->status = (ch->status & 0xFC) | 1; // set lower bit - ZSTD compression
                                }

                            #else
                                // ch->size  += LZ4_compress_fast((const char*)(data + offset), (char*)(chunk.get() + CHUNK_HLEN), csz, CHUNK_SIZE, 10);
                                ret = LZ4_compress_default((const char*)(data + offset), (char*)(chunk.get() + CHUNK_HLEN), csz, CHUNK_SIZE);
                            #endif
                        #else
                            memcpy((void*)(chunk.get() + CHUNK_HLEN), (void*)(data + offset), csz);
                            ret = csz;
                            ch->status = ch->status & 0xFC; // clean lower two bits - no compression
                        #endif                
                            
                        ch->size  += ret;
                        out_size  += ch->size;
                        offset    += CHUNK_SIZE;

                        ch->meta_id = meta_attr->first;
                        ch->meta_data = meta_attr->second;
                        meta_attr++;
                        if (meta_attr == meta_attrs.end()) meta_attr = meta_attrs.begin();

                        // push the chunk to queues
                        for (ConsumerQMap::iterator it = m_queues.begin(); it != m_queues.end(); ++it) 
                        {
                            std::lock_guard<std::mutex> lck (m_queues_mutex);
                            it->second[profile_key]->push(chunk);
                        }
                    }
                }

                auto end = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                std::chrono::duration<double> total_time = end - beginning;
                frame_count[profile_key]++;
                double fps = 0;
                if (total_time.count() > 0) fps = (double)frame_count[profile_key] / (double)total_time.count();

                std::stringstream ss_name;
                ss_name << "Frame '" << std::setiosflags(std::ios::left);
                ss_name << std::setw(13) << get_name() << " / " << rs2_stream_to_string(frame_profile.stream_type());
                if (frame_profile.stream_index()) ss_name << " " << frame_profile.stream_index();
                ss_name << "'";

                std::stringstream ss;
                ss << std::setiosflags(std::ios::left) << std::setw(35) << ss_name.str();
                ss << std::setiosflags(std::ios::right) << std::setiosflags(std::ios::fixed) << std::setprecision(2); 
                ss << "   compression time " << std::setw(7) << elapsed.count() * 1000 << "ms, ";
                ss << "size " << std::setw(7) << size << " => " << std::setw(7) << out_size << " (" << (float)(size) / (float)out_size << "), ";
                ss << "FPS: " << std::setw(7) << fps;

                LOG_DEBUG(ss.str());
                
                if (total_time > std::chrono::seconds(1)) {
                    beginning = std::chrono::system_clock::now();
                    for (auto kp : frame_count) {
                        frame_count[kp.first] = 0;
                    }
                }
            };
            m_sensor.start(callback);            
        }

        FrameData frame = nullptr;
        if (!m_queues[consumer][key]->empty()) 
        {
            std::lock_guard<std::mutex> lck (m_queues_mutex);
            frame = m_queues[consumer][key]->front();
            m_queues[consumer][key]->pop(); 
        }

        return frame;
    };

    void stop(void* consumer) { 
        m_queues.erase(consumer);
        if (m_queues.empty()) {
            if (m_sensor.get_active_streams().size() > 0) m_sensor.stop();
            m_sensor.close();
        }
    };

private:
    rs2::sensor                             m_sensor;
    std::map<uint64_t, rs2::stream_profile> m_streams;

    std::mutex m_queues_mutex;
    ConsumerQMap  m_queues;
};
