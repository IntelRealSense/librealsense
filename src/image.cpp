#include "image.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace rsimpl
{
    void copy_strided_image(void * dest_image, int dest_stride, const void * source_image, int source_stride, int rows)
    {
        auto dest = reinterpret_cast<uint8_t *>(dest_image);
        auto source = reinterpret_cast<const uint8_t *>(source_image);
        for(int i=0; i<rows; ++i)
        {
            memcpy(dest, source, std::min(dest_stride, source_stride));
            dest += dest_stride;
            source += source_stride;
        }
    }

    static uint8_t clamp_byte(int v)
    {
        if(v < 0) return 0;
        if(v > 255) return 255;
        return v;
    }

    inline uint8_t yuv_to_r(int y, int u, int v) { return clamp_byte((128 + 298 * y           + 409 * v) >> 8); }
    inline uint8_t yuv_to_g(int y, int u, int v) { return clamp_byte((128 + 298 * y - 100 * u - 208 * v) >> 8); }
    inline uint8_t yuv_to_b(int y, int u, int v) { return clamp_byte((128 + 298 * y + 516 * u          ) >> 8); }

    void convert_yuyv_to_rgb(void * dest_image, int width, int height, const void * source_image)
    {
        auto dest = reinterpret_cast<uint8_t *>(dest_image);
        auto source = reinterpret_cast<const uint8_t *>(source_image);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x += 2)
            {
                const int y0 = source[0] - 16, u = source[1] - 128, y1 = source[2] - 16, v = source[3] - 128;

                *dest++ = yuv_to_r(y0, u, v);
                *dest++ = yuv_to_g(y0, u, v);
                *dest++ = yuv_to_b(y0, u, v);

                *dest++ = yuv_to_r(y1, u, v);
                *dest++ = yuv_to_g(y1, u, v);
                *dest++ = yuv_to_b(y1, u, v);

                source += 4;
            }
        }
    }

    void convert_yuyv_to_rgba(void * dest_image, int width, int height, const void * source_image)
    {
        auto dest = reinterpret_cast<uint8_t *>(dest_image);
        auto source = reinterpret_cast<const uint8_t *>(source_image);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x += 2)
            {
                const int y0 = source[0] - 16, u = source[1] - 128, y1 = source[2] - 16, v = source[3] - 128;

                *dest++ = yuv_to_r(y0, u, v);
                *dest++ = yuv_to_g(y0, u, v);
                *dest++ = yuv_to_b(y0, u, v);
                *dest++ = 255;

                *dest++ = yuv_to_r(y1, u, v);
                *dest++ = yuv_to_g(y1, u, v);
                *dest++ = yuv_to_b(y1, u, v);
                *dest++ = 255;

                source += 4;
            }
        }
    }

    void convert_yuyv_to_bgr(void * dest_image, int width, int height, const void * source_image)
    {
        auto dest = reinterpret_cast<uint8_t *>(dest_image);
        auto source = reinterpret_cast<const uint8_t *>(source_image);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x += 2)
            {
                const int y0 = source[0] - 16, u = source[1] - 128, y1 = source[2] - 16, v = source[3] - 128;

                *dest++ = yuv_to_b(y0, u, v);
                *dest++ = yuv_to_g(y0, u, v);
                *dest++ = yuv_to_r(y0, u, v);                

                *dest++ = yuv_to_b(y1, u, v);
                *dest++ = yuv_to_g(y1, u, v);
                *dest++ = yuv_to_r(y1, u, v);

                source += 4;
            }
        }
    }

    void convert_yuyv_to_bgra(void * dest_image, int width, int height, const void * source_image)
    {
        auto dest = reinterpret_cast<uint8_t *>(dest_image);
        auto source = reinterpret_cast<const uint8_t *>(source_image);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; x += 2)
            {
                const int y0 = source[0] - 16, u = source[1] - 128, y1 = source[2] - 16, v = source[3] - 128;

                *dest++ = yuv_to_b(y0, u, v);
                *dest++ = yuv_to_g(y0, u, v);
                *dest++ = yuv_to_r(y0, u, v);                
                *dest++ = 255;

                *dest++ = yuv_to_b(y1, u, v);
                *dest++ = yuv_to_g(y1, u, v);
                *dest++ = yuv_to_r(y1, u, v);
                *dest++ = 255;

                source += 4;
            }
        }
    }

    #pragma pack(push, 1)
    struct y12i_pixel
    { 
        uint8_t rl : 8, rh : 4, ll : 4, lh : 8; 
        int left() const { return lh << 4 | ll; }
        int right() const { return rh << 8 | rl; }
    };
    static_assert(sizeof(y12i_pixel) == 3, "packing error");
    #pragma pack(pop)
    
    void convert_y12i_to_y8_y8(void * dest_left, void * dest_right, int width, int height, const void * source_image, int source_stride)
    {
        auto left = reinterpret_cast<uint8_t *>(dest_left), right = reinterpret_cast<uint8_t *>(dest_right);
        for(int y=0; y<height; ++y)
        {
            auto src = reinterpret_cast<const y12i_pixel *>(reinterpret_cast<const char *>(source_image) + y*source_stride);
            for(int x=0; x<width; ++x)
            {
                // We have ten bit data, so multiply by 1/4 (roughly 255/1023)
                *right++ = src->right() >> 2;
                *left++ = src->left() >> 2;
                ++src;
            }
        }
    }

    void convert_y12i_to_y16_y16(void * dest_left, void * dest_right, int width, int height, const void * source_image, int source_stride)
    {
        auto left = reinterpret_cast<uint16_t *>(dest_left), right = reinterpret_cast<uint16_t *>(dest_right);
        for(int y=0; y<height; ++y)
        {
            auto src = reinterpret_cast<const y12i_pixel *>(reinterpret_cast<const char *>(source_image) + y*source_stride);
            for(int x=0; x<width; ++x)
            {
                // We have ten bit data, so multiply by 64 1/16 (roughly 65535/1023)
                *right++ = src->right() << 6 | src->right() >> 4;
                *left++ = src->left() << 6 | src->left() >> 4;
                ++src;
            }
        }
    }
}
