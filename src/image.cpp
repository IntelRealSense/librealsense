#include "image.h"
#include <cstring>
#include <algorithm>

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

void convert_yuyv_to_rgb(uint8_t * dest, int width, int height, const uint8_t * source)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; x += 2)
        {
            int y0 = source[0], u = source[1], y1 = source[2], v = source[3];

            int c = y0 - 16, d = u - 128, e = v - 128;
            *dest++ = clamp_byte((298 * c + 409 * e + 128) >> 8);           // r
            *dest++ = clamp_byte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            *dest++ = clamp_byte((298 * c + 516 * d + 128) >> 8);           // b

            c = y1 - 16;
            *dest++ = clamp_byte((298 * c + 409 * e + 128) >> 8);           // r
            *dest++ = clamp_byte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            *dest++ = clamp_byte((298 * c + 516 * d + 128) >> 8);           // b

            source += 4;
        }
    }
}
