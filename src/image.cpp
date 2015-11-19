#include "image.h"
#include "../include/librealsense/rsutil.h" // For projection/deprojection logic

#include <cstring> // For memcpy
#include <cmath>
#include <algorithm>

#pragma pack(push, 1) // All structs in this file are assumed to be byte-packed
namespace rsimpl
{
    struct yuy2_macropixel { uint8_t y0,u,y1,v; };
    static_assert(sizeof(yuy2_macropixel) == 4, "packing error");

    struct y12i_pixel
    { 
        uint8_t rl : 8, rh : 4, ll : 4, lh : 8; 
        int l() const { return lh << 4 | ll; }
        int r() const { return rh << 8 | rl; }
    };
    static_assert(sizeof(y12i_pixel) == 3, "packing error");

    struct inri_pixel { uint16_t z16; uint8_t y8; };
    static_assert(sizeof(inri_pixel) == 3, "packing error");

    ////////////////////////////
    // Image size computation //
    ////////////////////////////

    size_t get_image_size(int width, int height, rs_format format)
    {
        switch(format)
        {
        case RS_FORMAT_Z16: return width * height * 2;
        case RS_FORMAT_DISPARITY16: return width * height * 2;
        case RS_FORMAT_YUYV: assert(width % 2 == 0); return width * height * 2;
        case RS_FORMAT_RGB8: return width * height * 3;
        case RS_FORMAT_BGR8: return width * height * 3;
        case RS_FORMAT_RGBA8: return width * height * 4;
        case RS_FORMAT_BGRA8: return width * height * 4;
        case RS_FORMAT_Y8: return width * height;
        case RS_FORMAT_Y16: return width * height * 2;
        case RS_FORMAT_RAW10: assert(width % 4 == 0); return width * 5/4 * height;
        default: assert(false); return 0;
        }    
    }

    const native_pixel_format pf_rw10        = {'RW10', 1, 1, 1};
    const native_pixel_format pf_yuy2        = {'YUY2', 1, 2, sizeof(yuy2_macropixel)};
    const native_pixel_format pf_y8          = {'Y8  ', 1, 1, sizeof(uint8_t)};
    const native_pixel_format pf_y8i         = {'Y8I ', 1, 1, sizeof(uint8_t)*2};
    const native_pixel_format pf_y16         = {'Y16 ', 1, 1, sizeof(uint16_t)};
    const native_pixel_format pf_y12i        = {'Y12I', 1, 1, sizeof(y12i_pixel)};
    const native_pixel_format pf_z16         = {'Z16 ', 1, 1, sizeof(uint16_t)};
    const native_pixel_format pf_invz        = {'INVZ', 1, 1, sizeof(uint16_t)};
    const native_pixel_format pf_f200_invi   = {'INVI', 1, 1, sizeof(uint8_t)};
    const native_pixel_format pf_f200_inzi   = {'INZI', 1, 1, sizeof(inri_pixel)};
    const native_pixel_format pf_sr300_invi  = {'INVI', 1, 1, sizeof(uint16_t)};
    const native_pixel_format pf_sr300_inzi  = {'INZI', 2, 1, sizeof(uint16_t)};

    //////////////////////////////
    // Naive unpacking routines //
    //////////////////////////////
    
    void unpack_subrect(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        assert(mode.streams.size() == 1);
        const size_t in_stride = mode.pf->get_image_size(mode.width, 1), out_stride = get_image_size(mode.streams[0].width, 1, mode.streams[0].format);
        auto out = dest[0];
        for(int i=0; i<std::min(mode.height, mode.streams[0].height); ++i)
        {
            memcpy(out, source, std::min(in_stride, out_stride));
            out += out_stride;
            source += in_stride;            
        }
    }

    template<class SOURCE, class UNPACK> void unpack_pixels(byte * const dest[], const subdevice_mode & mode, const SOURCE * source, rs_format format, UNPACK unpack)
    {
        assert(mode.streams.size() == 1 && mode.streams[0].width <= mode.width && mode.streams[0].height <= mode.height && mode.streams[0].format == format);
        auto out = reinterpret_cast<decltype(unpack(SOURCE())) *>(dest[0]);
        for(int y = 0; y < mode.streams[0].height; ++y)
        {
            for(int x = 0; x < mode.streams[0].width; ++x)
            {
                *out++ = unpack(*source++);
            }
            source += mode.width - mode.streams[0].width;
        }       
    }

    void unpack_y16_from_y8    (byte * const d[], const byte * s, const subdevice_mode & m) { unpack_pixels(d, m, reinterpret_cast<const uint8_t  *>(s), RS_FORMAT_Y16, [](uint8_t  pixel) -> uint16_t { return pixel | pixel << 8; }); }
    void unpack_y16_from_y16_10(byte * const d[], const byte * s, const subdevice_mode & m) { unpack_pixels(d, m, reinterpret_cast<const uint16_t *>(s), RS_FORMAT_Y16, [](uint16_t pixel) -> uint16_t { return pixel << 6; }); }
    void unpack_y8_from_y16_10 (byte * const d[], const byte * s, const subdevice_mode & m) { unpack_pixels(d, m, reinterpret_cast<const uint16_t *>(s), RS_FORMAT_Y8,  [](uint16_t pixel) -> uint8_t  { return pixel >> 2; }); }

    /////////////////////////////
    // YUY2 unpacking routines //
    /////////////////////////////

    template<class UNPACK> void unpack_from_yuy2(byte * const dest[], const byte * source, const subdevice_mode & mode, rs_format format, UNPACK unpack)
    {
        assert(mode.pf == &pf_yuy2 && mode.streams.size() == 1 && mode.streams[0].width <= mode.width && mode.streams[0].height <= mode.height && mode.streams[0].format == format);
        auto in = reinterpret_cast<const yuy2_macropixel *>(source);
        auto out = reinterpret_cast<decltype(unpack(0,0,0)) *>(dest[0]);
        for(int y = 0; y < mode.streams[0].height; ++y)
        {
            for(int x = 0; x < mode.streams[0].width; x+=2)
            {
                *out++ = unpack(in->y0 - 16, in->u - 128, in->v - 128);
                *out++ = unpack(in->y1 - 16, in->u - 128, in->v - 128);
                ++in;
            }
            in += mode.width - mode.streams[0].width;
        }     
    }

    inline uint8_t clamp_byte(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
    inline uint8_t yuv_to_r(int y, int u, int v) { return clamp_byte((128 + 298 * y           + 409 * v) >> 8); }
    inline uint8_t yuv_to_g(int y, int u, int v) { return clamp_byte((128 + 298 * y - 100 * u - 208 * v) >> 8); }
    inline uint8_t yuv_to_b(int y, int u, int v) { return clamp_byte((128 + 298 * y + 516 * u          ) >> 8); }
    
    struct byte3 { uint8_t a,b,c; }; struct byte4 { uint8_t a,b,c,d; };
    void unpack_rgb_from_yuy2 (byte * const d[], const byte * s, const subdevice_mode & m) { unpack_from_yuy2(d, s, m, RS_FORMAT_RGB8,  [](int y, int u, int v) { return byte3{yuv_to_r(y, u, v), yuv_to_g(y, u, v), yuv_to_b(y, u, v)     }; }); }
    void unpack_rgba_from_yuy2(byte * const d[], const byte * s, const subdevice_mode & m) { unpack_from_yuy2(d, s, m, RS_FORMAT_RGBA8, [](int y, int u, int v) { return byte4{yuv_to_r(y, u, v), yuv_to_g(y, u, v), yuv_to_b(y, u, v), 255}; }); }
    void unpack_bgr_from_yuy2 (byte * const d[], const byte * s, const subdevice_mode & m) { unpack_from_yuy2(d, s, m, RS_FORMAT_BGR8,  [](int y, int u, int v) { return byte3{yuv_to_b(y, u, v), yuv_to_g(y, u, v), yuv_to_r(y, u, v)     }; }); }
    void unpack_bgra_from_yuy2(byte * const d[], const byte * s, const subdevice_mode & m) { unpack_from_yuy2(d, s, m, RS_FORMAT_BGRA8, [](int y, int u, int v) { return byte4{yuv_to_b(y, u, v), yuv_to_g(y, u, v), yuv_to_r(y, u, v), 255}; }); }

    //////////////////////////////////////
    // 2-in-1 format splitting routines //
    //////////////////////////////////////

    template<class SOURCE, class SPLIT_A, class SPLIT_B> void split_frame(byte * const dest[], const subdevice_mode & mode, const SOURCE * source, const native_pixel_format & pf, rs_format format_a, rs_format format_b, SPLIT_A split_a, SPLIT_B split_b)
    {
        assert(mode.pf == &pf && mode.streams.size() == 2 && mode.streams[0].format == format_a && mode.streams[1].format == format_b
            && mode.streams[0].width == mode.streams[1].width && mode.streams[0].height == mode.streams[1].height && mode.streams[0].width <= mode.width && mode.streams[0].height <= mode.height);
        auto a = reinterpret_cast<decltype(split_a(SOURCE())) *>(dest[0]);
        auto b = reinterpret_cast<decltype(split_b(SOURCE())) *>(dest[1]);
        for(int y = 0; y < mode.streams[0].height; ++y)
        {
            for(int x = 0; x < mode.streams[0].width; ++x)
            {
                *a++ = split_a(*source);
                *b++ = split_b(*source++);
            }
            source += mode.width - mode.streams[0].width;
        }    
    }

    void unpack_y8_y8_from_y8i(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        struct y8i_pixel { uint8_t l, r; };
        split_frame(dest, mode, reinterpret_cast<const y8i_pixel *>(source), pf_y8i, RS_FORMAT_Y8, RS_FORMAT_Y8,
            [](const y8i_pixel & p) -> uint8_t { return p.l; },
            [](const y8i_pixel & p) -> uint8_t { return p.r; });
    }

    void unpack_y16_y16_from_y12i_10(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        split_frame(dest, mode, reinterpret_cast<const y12i_pixel *>(source), pf_y12i, RS_FORMAT_Y16, RS_FORMAT_Y16,
            [](const y12i_pixel & p) -> uint16_t { return p.l() << 6 | p.l() >> 4; },  // We want to convert 10-bit data to 16-bit data
            [](const y12i_pixel & p) -> uint16_t { return p.r() << 6 | p.r() >> 4; }); // Multiply by 64 1/16 to efficiently approximate 65535/1023
    }

    void unpack_z16_y8_from_f200_inzi(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        split_frame(dest, mode, reinterpret_cast<const inri_pixel *>(source), pf_f200_inzi, RS_FORMAT_Z16, RS_FORMAT_Y8,
            [](const inri_pixel & p) -> uint16_t { return p.z16; },
            [](const inri_pixel & p) -> uint8_t { return p.y8; });
    }

    void unpack_z16_y16_from_f200_inzi(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        split_frame(dest, mode, reinterpret_cast<const inri_pixel *>(source), pf_f200_inzi, RS_FORMAT_Z16, RS_FORMAT_Y16,
            [](const inri_pixel & p) -> uint16_t { return p.z16; },
            [](const inri_pixel & p) -> uint16_t { return p.y8 | p.y8 << 8; });
    }

    void unpack_z16_y8_from_sr300_inzi(byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        auto in = reinterpret_cast<const uint16_t *>(source);
        auto out_depth = reinterpret_cast<uint16_t *>(dest[0]);
        auto out_ir = reinterpret_cast<uint8_t *>(dest[1]);            
        for(int i=0, n=mode.width*mode.height; i<n; ++i) *out_ir++ = *in++ >> 2;
        memcpy(out_depth, in, mode.width*mode.height*2);
    }

    void unpack_z16_y16_from_sr300_inzi (byte * const dest[], const byte * source, const subdevice_mode & mode)
    {
        auto in = reinterpret_cast<const uint16_t *>(source);
        auto out_depth = reinterpret_cast<uint16_t *>(dest[0]);
        auto out_ir = reinterpret_cast<uint16_t *>(dest[1]);            
        for(int i=0, n=mode.width*mode.height; i<n; ++i) *out_ir++ = *in++ << 6;
        memcpy(out_depth, in, mode.width*mode.height*2);
    }

    /////////////////////
    // Image alignment //
    /////////////////////

    #pragma pack(push, 1)
    template<int N> struct bytes { char b[N]; };
    #pragma pack(pop)

    template<class GET_DEPTH, class TRANSFER_PIXEL> void align_images(const rs_intrinsics & depth_intrin, const rs_extrinsics & depth_to_other, const rs_intrinsics & other_intrin, GET_DEPTH get_depth, TRANSFER_PIXEL transfer_pixel)
    {
        // Iterate over the pixels of the depth image    
        for(int depth_y = 0, depth_pixel_index = 0; depth_y < depth_intrin.height; ++depth_y)
        {
            for(int depth_x = 0; depth_x < depth_intrin.width; ++depth_x, ++depth_pixel_index)
            {
                // Skip over depth pixels with the value of zero, we have no depth data so we will not write anything into our aligned images
                if(float depth = get_depth(depth_pixel_index))
                {
                    // Determine the corresponding pixel location in our color image
                    float depth_pixel[2] = {(float)depth_x, (float)depth_y}, depth_point[3], other_point[3], other_pixel[2];
                    rs_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, depth);
                    rs_transform_point_to_point(other_point, &depth_to_other, depth_point);
                    rs_project_point_to_pixel(other_pixel, &other_intrin, other_point);
                
                    // If the location is outside the bounds of the image, skip to the next pixel
                    const int other_x = (int)std::round(other_pixel[0]), other_y = (int)std::round(other_pixel[1]);
                    if(other_x < 0 || other_y < 0 || other_x >= other_intrin.width || other_y >= other_intrin.height)
                    {
                        continue;
                    }

                    // Transfer data from original images into corresponding aligned images
                    transfer_pixel(depth_pixel_index, other_y * other_intrin.width + other_x);
                }
            }
        }    
    }

    void align_z_to_color(byte * z_aligned_to_color, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, const rs_extrinsics & z_to_color, const rs_intrinsics & color_intrin)
    {
        auto out_z = (uint16_t *)(z_aligned_to_color);
        align_images(z_intrin, z_to_color, color_intrin, 
            [z_pixels, z_scale](int z_pixel_index) { return z_scale * z_pixels[z_pixel_index]; },
            [out_z, z_pixels](int z_pixel_index, int color_pixel_index) { out_z[color_pixel_index] = z_pixels[z_pixel_index]; });
    }

    void align_disparity_to_color(byte * disparity_aligned_to_color, const uint16_t * disparity_pixels, float disparity_scale, const rs_intrinsics & disparity_intrin, const rs_extrinsics & disparity_to_color, const rs_intrinsics & color_intrin)
    {
        auto out_disparity = (uint16_t *)(disparity_aligned_to_color);
        align_images(disparity_intrin, disparity_to_color, color_intrin, 
            [disparity_pixels, disparity_scale](int disparity_pixel_index) { return disparity_scale / disparity_pixels[disparity_pixel_index]; },
            [out_disparity, disparity_pixels](int disparity_pixel_index, int color_pixel_index) { out_disparity[color_pixel_index] = disparity_pixels[disparity_pixel_index]; });
    }

    template<int N, class GET_DEPTH> void align_color_to_depth_bytes(byte * color_aligned_to_depth, GET_DEPTH get_depth, const rs_intrinsics & depth_intrin, const rs_extrinsics & depth_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels)
    {
        auto in_color = (const bytes<N> *)(color_pixels);
        auto out_color = (bytes<N> *)(color_aligned_to_depth);
        align_images(depth_intrin, depth_to_color, color_intrin, get_depth,
            [out_color, in_color](int depth_pixel_index, int color_pixel_index) { out_color[depth_pixel_index] = in_color[color_pixel_index]; });
    }

    template<class GET_DEPTH> void align_color_to_depth(byte * color_aligned_to_depth, GET_DEPTH get_depth, const rs_intrinsics & depth_intrin, const rs_extrinsics & depth_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels, rs_format color_format)
    {
        switch(color_format)
        {
        case RS_FORMAT_Y8: 
            return align_color_to_depth_bytes<1>(color_aligned_to_depth, get_depth, depth_intrin, depth_to_color, color_intrin, color_pixels);
        case RS_FORMAT_Y16: case RS_FORMAT_Z16: 
            return align_color_to_depth_bytes<2>(color_aligned_to_depth, get_depth, depth_intrin, depth_to_color, color_intrin, color_pixels);
        case RS_FORMAT_RGB8: case RS_FORMAT_BGR8: 
            return align_color_to_depth_bytes<3>(color_aligned_to_depth, get_depth, depth_intrin, depth_to_color, color_intrin, color_pixels);
        case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8: 
            return align_color_to_depth_bytes<4>(color_aligned_to_depth, get_depth, depth_intrin, depth_to_color, color_intrin, color_pixels);
        default: 
            assert(false); // NOTE: rs_align_color_to_depth_bytes<2>(...) is not appropriate for RS_FORMAT_YUYV/RS_FORMAT_RAW10 images, no logic prevents U/V channels from being written to one another
        }
    }

    void align_color_to_z(byte * color_aligned_to_z, const uint16_t * z_pixels, float z_scale, const rs_intrinsics & z_intrin, const rs_extrinsics & z_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels, rs_format color_format)
    {
        align_color_to_depth(color_aligned_to_z, [z_pixels, z_scale](int z_pixel_index) { return z_scale * z_pixels[z_pixel_index]; }, z_intrin, z_to_color, color_intrin, color_pixels, color_format);
    }

    void align_color_to_disparity(byte * color_aligned_to_disparity, const uint16_t * disparity_pixels, float disparity_scale, const rs_intrinsics & disparity_intrin, const rs_extrinsics & disparity_to_color, const rs_intrinsics & color_intrin, const byte * color_pixels, rs_format color_format)
    {
        align_color_to_depth(color_aligned_to_disparity, [disparity_pixels, disparity_scale](int disparity_pixel_index) { return disparity_scale / disparity_pixels[disparity_pixel_index]; }, disparity_intrin, disparity_to_color, color_intrin, color_pixels, color_format);
    }

    /////////////////////////
    // Image rectification //
    /////////////////////////

    std::vector<int> compute_rectification_table(const rs_intrinsics & rect_intrin, const rs_extrinsics & rect_to_unrect, const rs_intrinsics & unrect_intrin)
    {   
        std::vector<int> rectification_table;
        rectification_table.resize(rect_intrin.width * rect_intrin.height);
        align_images(rect_intrin, rect_to_unrect, unrect_intrin, [](int) { return 1.0f; },
            [&rectification_table](int rect_pixel_index, int unrect_pixel_index) { rectification_table[rect_pixel_index] = unrect_pixel_index; });
        return rectification_table;
    }

    template<class T> void rectify_image_pixels(T * rect_pixels, const std::vector<int> & rectification_table, const T * unrect_pixels)
    {
        for(auto entry : rectification_table) *rect_pixels++ = unrect_pixels[entry];
    }

    void rectify_image(byte * rect_pixels, const std::vector<int> & rectification_table, const byte * unrect_pixels, rs_format format)
    {
        switch(format)
        {
        case RS_FORMAT_Y8: 
            return rectify_image_pixels((bytes<1> *)rect_pixels, rectification_table, (const bytes<1> *)unrect_pixels);
        case RS_FORMAT_Y16: case RS_FORMAT_Z16: 
            return rectify_image_pixels((bytes<2> *)rect_pixels, rectification_table, (const bytes<2> *)unrect_pixels);
        case RS_FORMAT_RGB8: case RS_FORMAT_BGR8: 
            return rectify_image_pixels((bytes<3> *)rect_pixels, rectification_table, (const bytes<3> *)unrect_pixels);
        case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8: 
            return rectify_image_pixels((bytes<4> *)rect_pixels, rectification_table, (const bytes<4> *)unrect_pixels);
        default: 
            assert(false); // NOTE: rectify_image_pixels(...) is not appropriate for RS_FORMAT_YUYV images, no logic prevents U/V channels from being written to one another
        }
    }
}
#pragma pack(pop)
