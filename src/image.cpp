#include "image.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

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
    // Y12I unpacking routines //
    /////////////////////////////

    #pragma pack(push, 1)
    struct y12i_pixel
    { 
        uint8_t rl : 8, rh : 4, ll : 4, lh : 8; 
        int left() const { return lh << 4 | ll; }
        int right() const { return rh << 8 | rl; }
    };
    static_assert(sizeof(y12i_pixel) == 3, "packing error");
    #pragma pack(pop)

    void unpack_y8_y8_from_y12i (void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'Y12I' && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y8 && mode.streams[1].format == RS_FORMAT_Y8);
        assert(mode.streams[0].width == mode.streams[1].width && mode.streams[0].width <= mode.width);
        assert(mode.streams[0].height == mode.streams[1].height && mode.streams[0].height <= mode.height);

        auto in = reinterpret_cast<const y12i_pixel *>(source);
        auto left = reinterpret_cast<uint8_t *>(dest[0]), right = reinterpret_cast<uint8_t *>(dest[1]);

        for(int y=0; y<mode.streams[0].height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; ++x)
            {
                // We have ten bit data, so multiply by 1/4 (roughly 255/1023)
                *right++ = in->right() >> 2;
                *left++ = in->left() >> 2;
                ++in;
            }
            in += mode.width - mode.streams[0].width;
        }
    }

    void unpack_y16_y16_from_y12i   (void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'Y12I' && mode.streams.size() == 2 && mode.streams[0].format == RS_FORMAT_Y16 && mode.streams[1].format == RS_FORMAT_Y16);
        assert(mode.streams[0].width == mode.streams[1].width && mode.streams[0].width <= mode.width);
        assert(mode.streams[0].height == mode.streams[1].height && mode.streams[0].height <= mode.height);

        auto in = reinterpret_cast<const y12i_pixel *>(source);
        auto left = reinterpret_cast<uint16_t *>(dest[0]), right = reinterpret_cast<uint16_t *>(dest[1]);

        for(int y=0; y<mode.streams[0].height; ++y)
        {
            for(int x=0; x<mode.streams[0].width; ++x)
            {
                // We have ten bit data, so multiply by 64 1/16 (roughly 65535/1023)
                *right++ = in->right() << 6 | in->right() >> 4;
                *left++ = in->left() << 6 | in->left() >> 4;
                ++in;
            }
            in += mode.width - mode.streams[0].width;
        }
    }

    /////////////////////////////
    // YUY2 unpacking routines //
    /////////////////////////////

    #pragma pack(push, 1)
    struct yuy2_double_pixel
    {
        uint8_t bytes[4];
        int y0() const { return bytes[0] - 16; }
        int y1() const { return bytes[2] - 16; }
        int u() const { return bytes[1] - 128; }
        int v() const { return bytes[3] - 128; }
    };
    static_assert(sizeof(yuy2_double_pixel) == 4, "packing error");
    #pragma pack(pop)

    inline uint8_t clamp_byte(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
    inline uint8_t yuv_to_r(int y, int u, int v) { return clamp_byte((128 + 298 * y           + 409 * v) >> 8); }
    inline uint8_t yuv_to_g(int y, int u, int v) { return clamp_byte((128 + 298 * y - 100 * u - 208 * v) >> 8); }
    inline uint8_t yuv_to_b(int y, int u, int v) { return clamp_byte((128 + 298 * y + 516 * u          ) >> 8); }

    void unpack_rgb_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'YUY2' && mode.streams.size() == 1 && mode.streams[0].width == mode.width && mode.streams[0].height == mode.height && mode.streams[0].format == RS_FORMAT_RGB);

        auto in = reinterpret_cast<const yuy2_double_pixel *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);
        
        for (int y = 0; y < mode.height; ++y)
        {
            for (int x = 0; x < mode.width; x += 2)
            {
                *out++ = yuv_to_r(in->y0(), in->u(), in->v());
                *out++ = yuv_to_g(in->y0(), in->u(), in->v());
                *out++ = yuv_to_b(in->y0(), in->u(), in->v());

                *out++ = yuv_to_r(in->y1(), in->u(), in->v());
                *out++ = yuv_to_g(in->y1(), in->u(), in->v());
                *out++ = yuv_to_b(in->y1(), in->u(), in->v());
                
                ++in;
            }
        }    
    }

    void unpack_rgba_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'YUY2' && mode.streams.size() == 1 && mode.streams[0].width == mode.width && mode.streams[0].height == mode.height && mode.streams[0].format == RS_FORMAT_RGBA);

        auto in = reinterpret_cast<const yuy2_double_pixel *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);
        
        for (int y = 0; y < mode.height; ++y)
        {
            for (int x = 0; x < mode.width; x += 2)
            {
                *out++ = yuv_to_r(in->y0(), in->u(), in->v());
                *out++ = yuv_to_g(in->y0(), in->u(), in->v());
                *out++ = yuv_to_b(in->y0(), in->u(), in->v());
                *out++ = 255;

                *out++ = yuv_to_r(in->y1(), in->u(), in->v());
                *out++ = yuv_to_g(in->y1(), in->u(), in->v());
                *out++ = yuv_to_b(in->y1(), in->u(), in->v());
                *out++ = 255;
                
                ++in;
            }
        }    
    }

    void unpack_bgr_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'YUY2' && mode.streams.size() == 1 && mode.streams[0].width == mode.width && mode.streams[0].height == mode.height && mode.streams[0].format == RS_FORMAT_BGR);

        auto in = reinterpret_cast<const yuy2_double_pixel *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);
        
        for (int y = 0; y < mode.height; ++y)
        {
            for (int x = 0; x < mode.width; x += 2)
            {
                *out++ = yuv_to_b(in->y0(), in->u(), in->v());
                *out++ = yuv_to_g(in->y0(), in->u(), in->v());
                *out++ = yuv_to_r(in->y0(), in->u(), in->v());
                
                *out++ = yuv_to_b(in->y1(), in->u(), in->v());
                *out++ = yuv_to_g(in->y1(), in->u(), in->v());
                *out++ = yuv_to_r(in->y1(), in->u(), in->v());
                
                ++in;
            }
        }    
    }

    void unpack_bgra_from_yuy2(void * dest[], const void * source, const subdevice_mode & mode)
    {
        assert(mode.fourcc == 'YUY2' && mode.streams.size() == 1 && mode.streams[0].width == mode.width && mode.streams[0].height == mode.height && mode.streams[0].format == RS_FORMAT_BGRA);

        auto in = reinterpret_cast<const yuy2_double_pixel *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);
        
        for (int y = 0; y < mode.height; ++y)
        {
            for (int x = 0; x < mode.width; x += 2)
            {
                *out++ = yuv_to_b(in->y0(), in->u(), in->v());
                *out++ = yuv_to_g(in->y0(), in->u(), in->v());
                *out++ = yuv_to_r(in->y0(), in->u(), in->v());
                *out++ = 255;
                
                *out++ = yuv_to_b(in->y1(), in->u(), in->v());
                *out++ = yuv_to_g(in->y1(), in->u(), in->v());
                *out++ = yuv_to_r(in->y1(), in->u(), in->v());
                *out++ = 255;
                
                ++in;
            }
        }    
    }
}
