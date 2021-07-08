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

using SoftSensor      = std::shared_ptr<rs2::software_sensor>;

class slib {
public:
    slib() {};
   ~slib() {};

    static int get_bpp(rs2::stream_profile profile) {
        int bpp = 0;
        if (profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = profile.as<rs2::video_stream_profile>();
            switch(vsp.format()) {
            case RS2_FORMAT_Z16   : bpp = 2; break;
            case RS2_FORMAT_YUYV  : bpp = 2; break;
            case RS2_FORMAT_Y8    : bpp = 1; break;
            case RS2_FORMAT_UYVY  : bpp = 2; break;
            case RS2_FORMAT_RGB8  : bpp = 3; break;
            case RS2_FORMAT_BGR8  : bpp = 3; break;
            case RS2_FORMAT_RGBA8 : bpp = 4; break;
            case RS2_FORMAT_BGRA8 : bpp = 4; break;
            }
        }
        return bpp;
    };

    // converts from YUV to profile's video format
    // returnes new frame in case of conversion or 
    // NULL if no conversion was available
    static uint8_t* convert(rs2::stream_profile profile, uint8_t* data_in) {
        uint8_t* data_out = NULL;

        if (profile.is<rs2::video_stream_profile>()) {

            if (profile.format() == RS2_FORMAT_RGB8  || profile.format() == RS2_FORMAT_BGR8 || 
                profile.format() == RS2_FORMAT_RGBA8 || profile.format() == RS2_FORMAT_BGRA8) {

                rs2::video_stream_profile vsp = profile.as<rs2::video_stream_profile>();
                int bpp = get_bpp(profile);

                data_out = new uint8_t[vsp.width() * vsp.height() * bpp];

                // convert the format if necessary
                switch(vsp.format()) {
                case RS2_FORMAT_RGB8  :
                case RS2_FORMAT_BGR8  :
                    // perform the conversion
                    for (int y = 0; y < vsp.height(); y++) {
                        for (int x = 0; x < vsp.width(); x += 2) {                
                            {
                                uint8_t Y = data_in[y * vsp.width() * 2 + x * 2 + 0];
                                uint8_t U = data_in[y * vsp.width() * 2 + x * 2 + 1];
                                uint8_t V = data_in[y * vsp.width() * 2 + x * 2 + 3];

                                uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                                uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                                uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                                data_out[y * vsp.width() * bpp + x * bpp + 0] = R;
                                data_out[y * vsp.width() * bpp + x * bpp + 1] = G;
                                data_out[y * vsp.width() * bpp + x * bpp + 2] = B;
                            }

                            {
                                uint8_t Y = data_in[y * vsp.width() * 2 + x * 2 + 2];
                                uint8_t U = data_in[y * vsp.width() * 2 + x * 2 + 1];
                                uint8_t V = data_in[y * vsp.width() * 2 + x * 2 + 3];

                                uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                                uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                                uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                                data_out[y * vsp.width() * bpp + x * bpp + 3] = R;
                                data_out[y * vsp.width() * bpp + x * bpp + 4] = G;
                                data_out[y * vsp.width() * bpp + x * bpp + 5] = B;
                            }                        
                        }
                    }
                    break;
                case RS2_FORMAT_RGBA8 :
                case RS2_FORMAT_BGRA8 :
                    // perform the conversion
                    for (int y = 0; y < vsp.height(); y++) {
                        for (int x = 0; x < vsp.width(); x += 2) {                
                            {
                                uint8_t Y = data_in[y * vsp.width() * 2 + x * 2 + 0];
                                uint8_t U = data_in[y * vsp.width() * 2 + x * 2 + 1];
                                uint8_t V = data_in[y * vsp.width() * 2 + x * 2 + 3];

                                uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                                uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                                uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                                data_out[y * vsp.width() * bpp + x * bpp + 0] = R;
                                data_out[y * vsp.width() * bpp + x * bpp + 1] = G;
                                data_out[y * vsp.width() * bpp + x * bpp + 2] = B;
                                data_out[y * vsp.width() * bpp + x * bpp + 3] = 0xFF;
                            }

                            {
                                uint8_t Y = data_in[y * vsp.width() * 2 + x * 2 + 2];
                                uint8_t U = data_in[y * vsp.width() * 2 + x * 2 + 1];
                                uint8_t V = data_in[y * vsp.width() * 2 + x * 2 + 3];

                                uint8_t R = fmax(0, fmin(255, Y + 1.402 * (V - 128)));
                                uint8_t G = fmax(0, fmin(255, Y - 0.344 * (U - 128) - 0.714 * (V - 128)));
                                uint8_t B = fmax(0, fmin(255, Y + 1.772 * (U - 128)));

                                data_out[y * vsp.width() * bpp + x * bpp + 4] = R;
                                data_out[y * vsp.width() * bpp + x * bpp + 5] = G;
                                data_out[y * vsp.width() * bpp + x * bpp + 6] = B;
                                data_out[y * vsp.width() * bpp + x * bpp + 7] = 0xFF;
                            }                        
                        }
                    }
                    break;
                }
            }
        }

        return data_out;
    }

    static std::string print_profile(rs2::stream_profile profile) {
        std::stringstream ss;
        ss << std::setw(10) << profile.stream_type() << std::setw(2);
        if (profile.stream_index()) ss << profile.stream_index();
        else ss << "";
        ss << std::setw(15) << rs2_format_to_string(profile.format());

        std::stringstream vss;
        if (profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = profile.as<rs2::video_stream_profile>();
            vss << vsp.width() << "x" << vsp.height();
        }
        
        ss << std::setw(10) << vss.str() << ":" << profile.fps() << "\t" << (profile.is_default() ? '*' : ' ');
        return ss.str();
    };

    static std::string print_stream(rs2_video_stream* pstream) {
        rs2_motion_stream* pmstream = (rs2_motion_stream*)pstream;

        std::stringstream ss;
        std::stringstream vss;

        switch (pstream->type) {
        case RS2_STREAM_DEPTH      :
        case RS2_STREAM_COLOR      :
        case RS2_STREAM_INFRARED   :
        case RS2_STREAM_CONFIDENCE :
            ss << std::setw(10) << pstream->type << std::setw(2);
            if (pstream->index) ss << pstream->index;
            else ss << "";
            ss << std::setw(15) << rs2_format_to_string(pstream->fmt);
            vss << pstream->width << "x" << pstream->height;
            ss << std::setw(10) << vss.str() << ":" << pstream->fps;
            break;
        case RS2_STREAM_GYRO       :
        case RS2_STREAM_ACCEL      :
            ss << std::setw(10) << pmstream->type << std::setw(2);
            if (pmstream->index) ss << pmstream->index;
            else ss << "";
            ss << std::setw(15) << rs2_format_to_string(pmstream->fmt);        
            ss << std::setw(10) << vss.str() << ":" << pmstream->fps;
            break;
        default:
            ss << "Unknown stream type";
        }        
        return ss.str();
    };

    static uint64_t profile2key(rs2::stream_profile profile, rs2_format format = RS2_FORMAT_ANY) {
        convert_t t;
        t.key = 0;

        t.values.def    = profile.is_default();
        t.values.type   = profile.stream_type();  
        t.values.index  = profile.stream_index(); 
        t.values.uid    = profile.unique_id();    
        t.values.fps    = profile.fps();
        if (format == RS2_FORMAT_ANY) {
            t.values.format = profile.format();
        } else {
            t.values.format = format;
        }

        if (profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = profile.as<rs2::video_stream_profile>();
            t.values.width  = vsp.width();        
            t.values.height = vsp.height();       
        } 

        return t.key;
    };

    static uint64_t stream2key(rs2_video_stream stream) {
        convert_t t;
        t.key = 0;

        t.values.def    = 0;
        t.values.type   = stream.type;  
        t.values.index  = stream.index; 
        t.values.uid    = stream.uid;    
        t.values.width  = stream.width;        
        t.values.height = stream.height;       
        t.values.fps    = stream.fps;          
        t.values.format = stream.fmt;

        return t.key;
    };

    static rs2_video_stream key2stream(uint64_t key) {
        convert_t t;
        streams_t s;

        t.key = key;
        memset((void*)&s, 0, sizeof(rs2_video_stream));

        s.video.type   = (rs2_stream)t.values.type;
        switch (s.video.type) {
        case RS2_STREAM_DEPTH      :
        case RS2_STREAM_COLOR      :
        case RS2_STREAM_INFRARED   :
        case RS2_STREAM_CONFIDENCE :
            s.video.index  = t.values.index;
            s.video.uid    = t.values.uid;
            s.video.width  = t.values.width;
            s.video.height = t.values.height;
            s.video.fps    = t.values.fps;
            s.video.fmt    = (rs2_format)t.values.format;
            s.video.bpp    = (s.video.type == RS2_STREAM_INFRARED) ? 1 : 2;
            break;
        case RS2_STREAM_GYRO       :
        case RS2_STREAM_ACCEL      :
            s.motion.index  = t.values.index;
            s.motion.uid    = t.values.uid;
            s.motion.fps    = t.values.fps;
            s.motion.fmt    = (rs2_format)t.values.format;
            break;
        default:
            LOG_ERROR("Unknown stream type");
        };

        return s.video;
    };

    static bool is_default(uint64_t key) {
        convert_t t;
        t.key = key;
        return t.values.def;
    };

private:
#pragma pack (push, 1)

    typedef union convert {
        uint64_t key;
        struct vals {
            uint8_t  def   : 1;
            uint8_t  type  : 5;
            uint8_t  index : 2;
            uint8_t  uid   ;
            uint16_t width ;
            uint16_t height;
            uint16_t fps   : 10;
            uint16_t format: 6;
        } values;
    } convert_t;

    typedef union streams {
        rs2_video_stream  video;
        rs2_motion_stream motion;
    } streams_t;

#pragma pack(pop)
};
