#include "image.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

#pragma pack(push, 1) // All structs in this file are assumed to be byte-packed
namespace rsimpl
{
    //////////////////////////////
    // Naive unpacking routines //
    //////////////////////////////
    
    void unpack_subrect(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].width <= mode.width && mode.streams[0].height <= mode.height);

        auto in = reinterpret_cast<const uint8_t *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);

        const size_t in_stride = get_image_size(mode.width, 1, mode.streams[0].format), out_stride = get_image_size(mode.streams[0].width, 1, mode.streams[0].format);
        for(int i=0; i<mode.streams[0].height; ++i)
        {
            memcpy(out, in, out_stride);
            out += out_stride;
            in += in_stride;            
        }
    }

    /////////////////////////////
    // YUY2 unpacking routines //
    /////////////////////////////

    template<class UNPACK> void unpack_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode, UNPACK unpack)
    {
        assert(mode.fourcc == 'YUY2' && mode.streams.size() == 1 && mode.streams[0].width == mode.width && mode.streams[0].height == mode.height);
        auto in = reinterpret_cast<const uint8_t *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);        
        for (int y = 0; y < mode.height; ++y)
        {
            for (int x = 0; x < mode.width; x += 2)
            {
                unpack(out, in[0]-16, in[1]-128, in[2]-16, in[3]-128);
                in += 4;
            }
        }     
    }

    inline uint8_t clamp_byte(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
    inline uint8_t yuv_to_r(int y, int u, int v) { return clamp_byte((128 + 298 * y           + 409 * v) >> 8); }
    inline uint8_t yuv_to_g(int y, int u, int v) { return clamp_byte((128 + 298 * y - 100 * u - 208 * v) >> 8); }
    inline uint8_t yuv_to_b(int y, int u, int v) { return clamp_byte((128 + 298 * y + 516 * u          ) >> 8); }
    
    void unpack_rgb_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_RGB);
        unpack_from_yuy2(dest, source, mode, [](uint8_t * & out, int y0, int u, int y1, int v)
        {
            *out++ = yuv_to_r(y0, u, v);
            *out++ = yuv_to_g(y0, u, v);
            *out++ = yuv_to_b(y0, u, v);

            *out++ = yuv_to_r(y1, u, v);
            *out++ = yuv_to_g(y1, u, v);
            *out++ = yuv_to_b(y1, u, v);
        });  
    }
    
    void unpack_rgba_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_RGBA);
        unpack_from_yuy2(dest, source, mode, [](uint8_t * & out, int y0, int u, int y1, int v)
        {
            *out++ = yuv_to_r(y0, u, v);
            *out++ = yuv_to_g(y0, u, v);
            *out++ = yuv_to_b(y0, u, v);
            *out++ = 255;

            *out++ = yuv_to_r(y1, u, v);
            *out++ = yuv_to_g(y1, u, v);
            *out++ = yuv_to_b(y1, u, v);
            *out++ = 255;
        });
    }

    void unpack_bgr_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_BGR);
        unpack_from_yuy2(dest, source, mode, [](uint8_t * & out, int y0, int u, int y1, int v)
        {
            *out++ = yuv_to_b(y0, u, v);
            *out++ = yuv_to_g(y0, u, v);
            *out++ = yuv_to_r(y0, u, v);

            *out++ = yuv_to_b(y1, u, v);
            *out++ = yuv_to_g(y1, u, v);
            *out++ = yuv_to_r(y1, u, v);
        });    
    }

    void unpack_bgra_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].format == RS_FORMAT_BGRA);
        unpack_from_yuy2(dest, source, mode, [](uint8_t * & out, int y0, int u, int y1, int v)
        {
            *out++ = yuv_to_b(y0, u, v);
            *out++ = yuv_to_g(y0, u, v);
            *out++ = yuv_to_r(y0, u, v);
            *out++ = 255;

            *out++ = yuv_to_b(y1, u, v);
            *out++ = yuv_to_g(y1, u, v);
            *out++ = yuv_to_r(y1, u, v);
            *out++ = 255;
        });   
    }

    //////////////////////////////////////
    // 2-in-1 format splitting routines //
    //////////////////////////////////////

    template<class SOURCE, class SPLIT_A, class SPLIT_B> void split_frame(void * dest[], const subdevice_mode & mode, const SOURCE * source, SPLIT_A split_a, SPLIT_B split_b)
    {
        assert(mode.streams.size() == 2 && mode.streams[0].width == mode.streams[1].width && mode.streams[0].height == mode.streams[1].height && mode.streams[0].width <= mode.width && mode.streams[0].height <= mode.height);
        auto a = reinterpret_cast<decltype(split_a(SOURCE())) *>(dest[0]);
        auto b = reinterpret_cast<decltype(split_b(SOURCE())) *>(dest[1]);
        for(int y=0; y<mode.streams[0].height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; ++x)
            {
                *a++ = split_a(*source);
                *b++ = split_b(*source++);
            }
            source += mode.width - mode.streams[0].width;
        }    
    }

    struct y12i_pixel
    { 
        uint8_t rl : 8, rh : 4, ll : 4, lh : 8; 
        int l() const { return lh << 4 | ll; }
        int r() const { return rh << 8 | rl; }
    };
    static_assert(sizeof(y12i_pixel) == 3, "packing error");

    void unpack_y8_y8_from_y12i(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'Y12I' && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y8 && mode.streams[1].format == RS_FORMAT_Y8);        
        split_frame(dest, mode, reinterpret_cast<const y12i_pixel *>(source),
            [](const y12i_pixel & p) -> uint8_t { return p.l() >> 2; },  // We want to convert 10-bit data to 8-bit data
            [](const y12i_pixel & p) -> uint8_t { return p.r() >> 2; }); // Multiply by 1/4 to efficiently approximate 255/1023
    }

    void unpack_y16_y16_from_y12i(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'Y12I' && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y16 && mode.streams[1].format == RS_FORMAT_Y16);
        split_frame(dest, mode, reinterpret_cast<const y12i_pixel *>(source),
            [](const y12i_pixel & p) -> uint16_t { return p.l() << 6 | p.l() >> 4; },  // We want to convert 10-bit data to 16-bit data
            [](const y12i_pixel & p) -> uint16_t { return p.r() << 6 | p.r() >> 4; }); // Multiply by 64 1/16 to efficiently approximate 65535/1023
    }

    struct inri_pixel 
    { 
        uint16_t z16; 
        uint8_t y8; 
    };
    static_assert(sizeof(y12i_pixel) == 3, "packing error");

    void unpack_z16_y8_from_inri(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'INRI' && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Z16 && mode.streams[1].format == RS_FORMAT_Y8);
        split_frame(dest, mode, reinterpret_cast<const inri_pixel *>(source),
            [](const inri_pixel & p) -> uint16_t { return p.z16; },
            [](const inri_pixel & p) -> uint8_t { return p.y8; });
    }
}
#pragma pack(pop)
