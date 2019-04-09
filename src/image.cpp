// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "image-avx.h"
#include "types.h"

#ifdef RS2_USE_CUDA
#include "cuda/cuda-conversion.cuh"
#endif
#ifdef __SSSE3__
#include <tmmintrin.h> // For SSSE3 intrinsics
#endif

#if defined (ANDROID) || (defined (__linux__) && !defined (__x86_64__))

bool has_avx() { return false; }

#else

#ifdef _WIN32
#include <intrin.h>
#define cpuid(info, x)    __cpuidex(info, x, 0)
#else
#include <cpuid.h>
void cpuid(int info[4], int info_type){
    __cpuid_count(info_type, 0, info[0], info[1], info[2], info[3]);
}
#endif

bool has_avx()
{
    int info[4];
    cpuid(info, 0);
    cpuid(info, 0x80000000);
    return (info[2] & ((int)1 << 28)) != 0;
}

#endif

#pragma pack(push, 1) // All structs in this file are assumed to be byte-packed
namespace librealsense
{

    ////////////////////////////
    // Image size computation //
    ////////////////////////////

    size_t get_image_size(int width, int height, rs2_format format)
    {
        if (format == RS2_FORMAT_YUYV || (format == RS2_FORMAT_UYVY)) assert(width % 2 == 0);
        if (format == RS2_FORMAT_RAW10) assert(width % 4 == 0);
        return width * height * get_image_bpp(format) / 8;
    }

    int get_image_bpp(rs2_format format)
    {
        switch (format)
        {
        case RS2_FORMAT_Z16: return  16;
        case RS2_FORMAT_DISPARITY16: return 16;
        case RS2_FORMAT_DISPARITY32: return 32;
        case RS2_FORMAT_XYZ32F: return 12 * 8;
        case RS2_FORMAT_YUYV:  return 16;
        case RS2_FORMAT_RGB8: return 24;
        case RS2_FORMAT_BGR8: return 24;
        case RS2_FORMAT_RGBA8: return 32;
        case RS2_FORMAT_BGRA8: return 32;
        case RS2_FORMAT_Y8: return 8;
        case RS2_FORMAT_Y16: return 16;
        case RS2_FORMAT_RAW10: return 10;
        case RS2_FORMAT_RAW16: return 16;
        case RS2_FORMAT_RAW8: return 8;
        case RS2_FORMAT_UYVY: return 16;
        case RS2_FORMAT_GPIO_RAW: return 1;
        case RS2_FORMAT_MOTION_RAW: return 1;
        case RS2_FORMAT_MOTION_XYZ32F: return 1;
        case RS2_FORMAT_6DOF: return 1;
        default: assert(false); return 0;
        }
    }
    //////////////////////////////
    // Naive unpacking routines //
    //////////////////////////////

#pragma pack (push, 1)

    struct hid_data
    {
        short x;
        byte reserved1[2];
        short y;
        byte reserved2[2];
        short z;
        byte reserved3[2];
    };

#pragma pack(pop)

    template<rs2_format FORMAT> void copy_hid_axes(byte * const dest[], const byte * source, double factor)
    {
        using namespace librealsense;
        auto hid = (hid_data*)(source);

        auto res= float3{ float(hid->x), float(hid->y), float(hid->z)} * float(factor);

        librealsense::copy(dest[0], &res, sizeof(float3));
    }

    // The Accelerometer input format: signed int 16bit. data units 1LSB=0.001g;
    // Librealsense output format: floating point 32bit. units m/s^2,
    template<rs2_format FORMAT> void unpack_accel_axes(byte * const dest[], const byte * source, int width, int height)
    {
        static constexpr float gravity = 9.80665f;          // Standard Gravitation Acceleration
        static constexpr double accelerator_transform_factor = 0.001*gravity;

        copy_hid_axes<FORMAT>(dest, source, accelerator_transform_factor);
    }

    // The Gyro input format: signed int 16bit. data units 1LSB=0.1deg/sec;
    // Librealsense output format: floating point 32bit. units rad/sec,
    template<rs2_format FORMAT> void unpack_gyro_axes(byte * const dest[], const byte * source, int width, int height)
    {
        static const double gyro_transform_factor = deg2rad(0.1);

        copy_hid_axes<FORMAT>(dest, source, gyro_transform_factor);
    }

    void unpack_hid_raw_data(byte * const dest[], const byte * source, int width, int height)
    {
        librealsense::copy(dest[0] + 0, source + 0, 2);
        librealsense::copy(dest[0] + 2, source + 4, 2);
        librealsense::copy(dest[0] + 4, source + 8, 2);
        librealsense::copy(dest[0] + 6, source + 16, 8);
    }

    void unpack_input_reports_data(byte * const dest[], const byte * source, int width, int height)
    {
        // Input Report Struct
        // uint8_t  sensor_state
        // uint8_t  sourceId
        // uint32_t customTimestamp
        // uint8_t  mmCounter
        // uint8_t  usbCounter
        static const int input_reports_size = 9;
        static const int input_reports_offset = 15;
        librealsense::copy(dest[0], source + input_reports_offset, input_reports_size);
    }

    template<size_t SIZE>
    void align_l500_image_optimized(byte * const dest[], const byte * source, int width, int height)
    {
        auto width_out = height;
        auto height_out = width;

        auto out = dest[0];
        byte buffer[8][8 * SIZE]; // = { 0 };
        for (int i = 0; i <= height-8; i = i + 8)
        {
            for (int j = 0; j <= width-8; j = j + 8)
            {
                for (int ii = 0; ii < 8; ++ii)
                {
                    for (int jj = 0; jj < 8; ++jj)
                    {
                        auto source_index = ((j+ jj) + (width * (i + ii))) * SIZE;
                        memcpy((void*)(&buffer[7-jj][(7-ii) * SIZE]), &source[source_index], SIZE);
                    }
                }

                for (int ii = 0; ii < 8; ++ii)
                {
                    auto out_index = (((height_out - 8 - j + 1) * width_out) - i - 8 + (ii)* width_out);
                    memcpy(&out[(out_index)* SIZE], &(buffer[ii]), 8 * SIZE);
                }
            }
        }
    }

    template<size_t SIZE>
    void align_l500_image(byte * const dest[], const byte * source, int width, int height)
    {
        auto width_out = height;
        auto height_out = width;

        auto out = dest[0];
        for (int i = 0; i < height; ++i)
        {
            auto row_offset = i * width;
            for (int j = 0; j < width; ++j)
            {
                auto out_index = (((height_out - j) * width_out) - i - 1) * SIZE;
                librealsense::copy((void*)(&out[out_index]), &(source[(row_offset + j) * SIZE]), SIZE);
            }
        }
    }

    void unpack_confidence(byte * const dest[], const byte * source, int width, int height)
    {
#pragma pack (push, 1)
        struct lsb_msb
        {
            unsigned lsb : 4;
            unsigned msb : 4;
        };
#pragma pack(pop)

        align_l500_image<1>(dest, source, width, height);
        auto out = dest[0];
        for (int i = (width - 1), out_i = ((width - 1) * 2); i >= 0; --i, out_i-=2)
        {
            auto row_offset = i * height;
            for (int j = 0; j < height; ++j)
            {
                auto val = *(reinterpret_cast<const lsb_msb*>(&out[(row_offset + j)]));
                auto out_index = out_i * height + j;
                out[out_index] = val.lsb << 4;
                out[out_index + height] = val.msb << 4;
            }
        }
    }

    template<size_t SIZE> void copy_pixels(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        librealsense::copy(dest[0], source, SIZE * count);
    }

    void copy_raw10(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        librealsense::copy(dest[0], source, (5 * (count / 4)));
    }

    template<class SOURCE, class UNPACK> void unpack_pixels(byte * const dest[], int count, const SOURCE * source, UNPACK unpack)
    {
        auto out = reinterpret_cast<decltype(unpack(SOURCE())) *>(dest[0]);
        for (int i = 0; i < count; ++i) *out++ = unpack(*source++);
    }

    void unpack_y16_from_y8(byte * const d[], const byte * s, int width, int height) { unpack_pixels(d, width * height, reinterpret_cast<const uint8_t *>(s), [](uint8_t  pixel) -> uint16_t { return pixel | pixel << 8; }); }
    void unpack_y16_from_y16_10(byte * const d[], const byte * s, int width, int height) { unpack_pixels(d, width * height, reinterpret_cast<const uint16_t*>(s), [](uint16_t pixel) -> uint16_t { return pixel << 6; }); }
    void unpack_y8_from_y16_10(byte * const d[], const byte * s, int width, int height) { unpack_pixels(d, width * height, reinterpret_cast<const uint16_t*>(s), [](uint16_t pixel) -> uint8_t  { return pixel >> 2; }); }
    void unpack_rw10_from_rw8(byte *  const d[], const byte * s, int width, int height)
    {
#ifdef __SSSE3__
        auto src = reinterpret_cast<const __m128i *>(s);
        auto dst = reinterpret_cast<__m128i *>(d[0]);

        __m128i * xin = (__m128i *)src;
        __m128i * xout = (__m128i *) dst;
        for (int i = 0; i < width * height; i += 16, ++xout, xin += 2)
        {
            __m128i  in1_16 = _mm_load_si128((__m128i *)(xin));
            __m128i  in2_16 = _mm_load_si128((__m128i *)(xin + 1));
            __m128i  out1_16 = _mm_srli_epi16(in1_16, 2);
            __m128i  out2_16 = _mm_srli_epi16(in2_16, 2);
            __m128i  out8 = _mm_packus_epi16(out1_16, out2_16);
            _mm_store_si128(xout, out8);
        }
#else  // Generic code for when SSSE3 is not available.
        unsigned short* from = (unsigned short*)s;
        byte * to = d[0];

        for(int i = 0; i < width * height; ++i)
        {
            byte temp = (byte)(*from >> 2);
            *to = temp;
            ++from;
            ++to;
        }
#endif
    }

    // Unpack luminocity 8 bit from 10-bit packed macro-pixels (4 pixels in 5 bytes):
    // The first four bytes store the 8 MSB of each pixel, and the last byte holds the 2 LSB for each pixel :8888[2222]
    void unpack_y8_from_rw10(byte *  const d[], const byte * s, int width, int height)
    {
#ifdef __SSSE3__
        auto n = width * height;
        assert(!(n % 48));  //We process 12 macro-pixels simultaneously to achieve performance boost
        auto src = reinterpret_cast<const uint8_t *>(s);
        auto dst = reinterpret_cast<uint8_t *>(d[0]);

        const __m128i * blk_0_in = reinterpret_cast<const __m128i *>(src);
        const __m128i * blk_1_in = reinterpret_cast<const __m128i *>(src + 15);
        const __m128i * blk_2_in = reinterpret_cast<const __m128i *>(src + 30);
        const __m128i * blk_3_in = reinterpret_cast<const __m128i *>(src + 45);

        auto blk_0_out = reinterpret_cast<__m128i *>(dst);
        auto blk_1_out = reinterpret_cast<__m128i *>(dst + 12);
        auto blk_2_out = reinterpret_cast<__m128i *>(dst + 24);
        auto blk_3_out = reinterpret_cast<__m128i *>(dst + 36);

        __m128i res[4];
        // The mask will reorder the input so the 12 bytes with pixels' MSB values will come first
        static const __m128i mask = _mm_setr_epi8(0x0, 0x1, 0x2, 0x3, 0x5, 0x6, 0x7, 0x8, 0xa, 0xb, 0xc, 0xd, -1, -1, -1, -1);


        for (int i = 0; (i + 48) < n; i += 48, src += 60, dst += 48)
        {
            blk_0_in = reinterpret_cast<const __m128i *>(src);
            blk_1_in = reinterpret_cast<const __m128i *>(src + 15);
            blk_2_in = reinterpret_cast<const __m128i *>(src + 30);
            blk_3_in = reinterpret_cast<const __m128i *>(src + 45);

            blk_0_out = reinterpret_cast<__m128i *>(dst);
            blk_1_out = reinterpret_cast<__m128i *>(dst + 12);
            blk_2_out = reinterpret_cast<__m128i *>(dst + 24);
            blk_3_out = reinterpret_cast<__m128i *>(dst + 36);

            res[0] = _mm_shuffle_epi8(_mm_loadu_si128(blk_0_in), mask);
            res[1] = _mm_shuffle_epi8(_mm_loadu_si128(blk_1_in), mask);
            res[2] = _mm_shuffle_epi8(_mm_loadu_si128(blk_2_in), mask);
            res[3] = _mm_shuffle_epi8(_mm_loadu_si128(blk_3_in), mask);

            _mm_storeu_si128(blk_0_out, res[0]);
            _mm_storeu_si128(blk_1_out, res[1]);
            _mm_storeu_si128(blk_2_out, res[2]);
            _mm_storeu_si128(blk_3_out, res[3]);
        }
#else  // Generic code for when SSSE3 is not available.
        auto from = reinterpret_cast<const uint8_t *>(s);
        uint8_t * tgt = d[0];

        for (int i = 0; i < width * height; i += 4, from += 5)
        {
            *tgt++ = from[0];
            *tgt++ = from[1];
            *tgt++ = from[2];
            *tgt++ = from[3];
        }
#endif
    }

    /////////////////////////////
    // YUY2 unpacking routines //
    /////////////////////////////
    // This templated function unpacks YUY2 into Y8/Y16/RGB8/RGBA8/BGR8/BGRA8, depending on the compile-time parameter FORMAT.
    // It is expected that all branching outside of the loop control variable will be removed due to constant-folding.
    template<rs2_format FORMAT> void unpack_yuy2(byte * const d[], const byte * s, int width, int height)
    {
        auto n = width * height;
        assert(n % 16 == 0); // All currently supported color resolutions are multiples of 16 pixels. Could easily extend support to other resolutions by copying final n<16 pixels into a zero-padded buffer and recursively calling self for final iteration.
#ifdef RS2_USE_CUDA
        rscuda::unpack_yuy2_cuda<FORMAT>(d, s, n);
        return;
#endif
#if defined __SSSE3__ && ! defined ANDROID
        static bool do_avx = has_avx();
        #ifdef __AVX2__

                if (do_avx)
                {
                    if (FORMAT == RS2_FORMAT_Y8) unpack_yuy2_avx_y8(d, s, n);
                    if (FORMAT == RS2_FORMAT_Y16) unpack_yuy2_avx_y16(d, s, n);
                    if (FORMAT == RS2_FORMAT_RGB8) unpack_yuy2_avx_rgb8(d, s, n);
                    if (FORMAT == RS2_FORMAT_RGBA8) unpack_yuy2_avx_rgba8(d, s, n);
                    if (FORMAT == RS2_FORMAT_BGR8) unpack_yuy2_avx_bgr8(d, s, n);
                    if (FORMAT == RS2_FORMAT_BGRA8) unpack_yuy2_avx_bgra8(d, s, n);
                }
                else
        #endif
        {
            auto src = reinterpret_cast<const __m128i *>(s);
            auto dst = reinterpret_cast<__m128i *>(d[0]);

#pragma omp parallel for
            for (int i = 0; i < n / 16; i++)
            {
                const __m128i zero = _mm_set1_epi8(0);
                const __m128i n100 = _mm_set1_epi16(100 << 4);
                const __m128i n208 = _mm_set1_epi16(208 << 4);
                const __m128i n298 = _mm_set1_epi16(298 << 4);
                const __m128i n409 = _mm_set1_epi16(409 << 4);
                const __m128i n516 = _mm_set1_epi16(516 << 4);
                const __m128i evens_odds = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);

                // Load 8 YUY2 pixels each into two 16-byte registers
                __m128i s0 = _mm_loadu_si128(&src[i * 2]);
                __m128i s1 = _mm_loadu_si128(&src[i * 2 + 1]);

                if (FORMAT == RS2_FORMAT_Y8)
                {
                    // Align all Y components and output 16 pixels (16 bytes) at once
                    __m128i y0 = _mm_shuffle_epi8(s0, _mm_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0, 2, 4, 6, 8, 10, 12, 14));
                    __m128i y1 = _mm_shuffle_epi8(s1, _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15));
                    _mm_storeu_si128(&dst[i], _mm_alignr_epi8(y0, y1, 8));
                    continue;
                }

                // Shuffle all Y components to the low order bytes of the register, and all U/V components to the high order bytes
                const __m128i evens_odd1s_odd3s = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 5, 9, 13, 3, 7, 11, 15); // to get yyyyyyyyuuuuvvvv
                __m128i yyyyyyyyuuuuvvvv0 = _mm_shuffle_epi8(s0, evens_odd1s_odd3s);
                __m128i yyyyyyyyuuuuvvvv8 = _mm_shuffle_epi8(s1, evens_odd1s_odd3s);

                // Retrieve all 16 Y components as 16-bit values (8 components per register))
                __m128i y16__0_7 = _mm_unpacklo_epi8(yyyyyyyyuuuuvvvv0, zero);         // convert to 16 bit
                __m128i y16__8_F = _mm_unpacklo_epi8(yyyyyyyyuuuuvvvv8, zero);         // convert to 16 bit

                if (FORMAT == RS2_FORMAT_Y16)
                {
                    // Output 16 pixels (32 bytes) at once
                    _mm_storeu_si128(&dst[i * 2], _mm_slli_epi16(y16__0_7, 8));
                    _mm_storeu_si128(&dst[i * 2 + 1], _mm_slli_epi16(y16__8_F, 8));
                    continue;
                }

                // Retrieve all 16 U and V components as 16-bit values (8 components per register)
                __m128i uv = _mm_unpackhi_epi32(yyyyyyyyuuuuvvvv0, yyyyyyyyuuuuvvvv8); // uuuuuuuuvvvvvvvv
                __m128i u = _mm_unpacklo_epi8(uv, uv);                                 //  uu uu uu uu uu uu uu uu  u's duplicated
                __m128i v = _mm_unpackhi_epi8(uv, uv);                                 //  vv vv vv vv vv vv vv vv
                __m128i u16__0_7 = _mm_unpacklo_epi8(u, zero);                         // convert to 16 bit
                __m128i u16__8_F = _mm_unpackhi_epi8(u, zero);                         // convert to 16 bit
                __m128i v16__0_7 = _mm_unpacklo_epi8(v, zero);                         // convert to 16 bit
                __m128i v16__8_F = _mm_unpackhi_epi8(v, zero);                         // convert to 16 bit

                                                                                       // Compute R, G, B values for first 8 pixels
                __m128i c16__0_7 = _mm_slli_epi16(_mm_subs_epi16(y16__0_7, _mm_set1_epi16(16)), 4);
                __m128i d16__0_7 = _mm_slli_epi16(_mm_subs_epi16(u16__0_7, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
                __m128i e16__0_7 = _mm_slli_epi16(_mm_subs_epi16(v16__0_7, _mm_set1_epi16(128)), 4);
                __m128i r16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(e16__0_7, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
                __m128i g16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(d16__0_7, n100)), _mm_mulhi_epi16(e16__0_7, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
                __m128i b16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(d16__0_7, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

                                                                                                                                                                                                                                 // Compute R, G, B values for second 8 pixels
                __m128i c16__8_F = _mm_slli_epi16(_mm_subs_epi16(y16__8_F, _mm_set1_epi16(16)), 4);
                __m128i d16__8_F = _mm_slli_epi16(_mm_subs_epi16(u16__8_F, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
                __m128i e16__8_F = _mm_slli_epi16(_mm_subs_epi16(v16__8_F, _mm_set1_epi16(128)), 4);
                __m128i r16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(e16__8_F, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
                __m128i g16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(d16__8_F, n100)), _mm_mulhi_epi16(e16__8_F, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
                __m128i b16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(d16__8_F, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

                if (FORMAT == RS2_FORMAT_RGB8 || FORMAT == RS2_FORMAT_RGBA8)
                {
                    // Shuffle separate R, G, B values into four registers storing four pixels each in (R, G, B, A) order
                    __m128i rg8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__0_7, evens_odds), _mm_shuffle_epi8(g16__0_7, evens_odds)); // hi to take the odds which are the upper bytes we care about
                    __m128i ba8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__0_7, evens_odds), _mm_set1_epi8(-1));
                    __m128i rgba_0_3 = _mm_unpacklo_epi16(rg8__0_7, ba8__0_7);
                    __m128i rgba_4_7 = _mm_unpackhi_epi16(rg8__0_7, ba8__0_7);

                    __m128i rg8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__8_F, evens_odds), _mm_shuffle_epi8(g16__8_F, evens_odds)); // hi to take the odds which are the upper bytes we care about
                    __m128i ba8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__8_F, evens_odds), _mm_set1_epi8(-1));
                    __m128i rgba_8_B = _mm_unpacklo_epi16(rg8__8_F, ba8__8_F);
                    __m128i rgba_C_F = _mm_unpackhi_epi16(rg8__8_F, ba8__8_F);

                    if (FORMAT == RS2_FORMAT_RGBA8)
                    {
                        // Store 16 pixels (64 bytes) at once
                        _mm_storeu_si128(&dst[i * 4], rgba_0_3);
                        _mm_storeu_si128(&dst[i * 4 + 1], rgba_4_7);
                        _mm_storeu_si128(&dst[i * 4 + 2], rgba_8_B);
                        _mm_storeu_si128(&dst[i * 4 + 3], rgba_C_F);
                    }

                    if (FORMAT == RS2_FORMAT_RGB8)
                    {
                        // Shuffle rgb triples to the start and end of each register
                        __m128i rgb0 = _mm_shuffle_epi8(rgba_0_3, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
                        __m128i rgb1 = _mm_shuffle_epi8(rgba_4_7, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
                        __m128i rgb2 = _mm_shuffle_epi8(rgba_8_B, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
                        __m128i rgb3 = _mm_shuffle_epi8(rgba_C_F, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));

                        // Align registers and store 16 pixels (48 bytes) at once
                        _mm_storeu_si128(&dst[i * 3], _mm_alignr_epi8(rgb1, rgb0, 4));
                        _mm_storeu_si128(&dst[i * 3 + 1], _mm_alignr_epi8(rgb2, rgb1, 8));
                        _mm_storeu_si128(&dst[i * 3 + 2], _mm_alignr_epi8(rgb3, rgb2, 12));
                    }
                }

                if (FORMAT == RS2_FORMAT_BGR8 || FORMAT == RS2_FORMAT_BGRA8)
                {
                    // Shuffle separate R, G, B values into four registers storing four pixels each in (B, G, R, A) order
                    __m128i bg8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__0_7, evens_odds), _mm_shuffle_epi8(g16__0_7, evens_odds)); // hi to take the odds which are the upper bytes we care about
                    __m128i ra8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__0_7, evens_odds), _mm_set1_epi8(-1));
                    __m128i bgra_0_3 = _mm_unpacklo_epi16(bg8__0_7, ra8__0_7);
                    __m128i bgra_4_7 = _mm_unpackhi_epi16(bg8__0_7, ra8__0_7);

                    __m128i bg8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__8_F, evens_odds), _mm_shuffle_epi8(g16__8_F, evens_odds)); // hi to take the odds which are the upper bytes we care about
                    __m128i ra8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__8_F, evens_odds), _mm_set1_epi8(-1));
                    __m128i bgra_8_B = _mm_unpacklo_epi16(bg8__8_F, ra8__8_F);
                    __m128i bgra_C_F = _mm_unpackhi_epi16(bg8__8_F, ra8__8_F);

                    if (FORMAT == RS2_FORMAT_BGRA8)
                    {
                        // Store 16 pixels (64 bytes) at once
                        _mm_storeu_si128(&dst[i * 4], bgra_0_3);
                        _mm_storeu_si128(&dst[i * 4 + 1], bgra_4_7);
                        _mm_storeu_si128(&dst[i * 4 + 2], bgra_8_B);
                        _mm_storeu_si128(&dst[i * 4 + 3], bgra_C_F);
                    }

                    if (FORMAT == RS2_FORMAT_BGR8)
                    {
                        // Shuffle rgb triples to the start and end of each register
                        __m128i bgr0 = _mm_shuffle_epi8(bgra_0_3, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
                        __m128i bgr1 = _mm_shuffle_epi8(bgra_4_7, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
                        __m128i bgr2 = _mm_shuffle_epi8(bgra_8_B, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
                        __m128i bgr3 = _mm_shuffle_epi8(bgra_C_F, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));

                        // Align registers and store 16 pixels (48 bytes) at once
                        _mm_storeu_si128(&dst[i * 3], _mm_alignr_epi8(bgr1, bgr0, 4));
                        _mm_storeu_si128(&dst[i * 3 + 1], _mm_alignr_epi8(bgr2, bgr1, 8));
                        _mm_storeu_si128(&dst[i * 3 + 2], _mm_alignr_epi8(bgr3, bgr2, 12));
                    }
                }
            }
        }
#else  // Generic code for when SSSE3 is not available.
        auto src = reinterpret_cast<const uint8_t *>(s);
        auto dst = reinterpret_cast<uint8_t *>(d[0]);
        for (; n; n -= 16, src += 32)
        {
            if (FORMAT == RS2_FORMAT_Y8)
            {
                uint8_t out[16] = {
                    src[0], src[2], src[4], src[6],
                    src[8], src[10], src[12], src[14],
                    src[16], src[18], src[20], src[22],
                    src[24], src[26], src[28], src[30],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_Y16)
            {
                // Y16 is little-endian.  We output Y << 8.
                uint8_t out[32] = {
                    0, src[0], 0, src[2], 0, src[4], 0, src[6],
                    0, src[8], 0, src[10], 0, src[12], 0, src[14],
                    0, src[16], 0, src[18], 0, src[20], 0, src[22],
                    0, src[24], 0, src[26], 0, src[28], 0, src[30],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            int16_t y[16] = {
                src[0], src[2], src[4], src[6],
                src[8], src[10], src[12], src[14],
                src[16], src[18], src[20], src[22],
                src[24], src[26], src[28], src[30],
            }, u[16] = {
                src[1], src[1], src[5], src[5],
                src[9], src[9], src[13], src[13],
                src[17], src[17], src[21], src[21],
                src[25], src[25], src[29], src[29],
            }, v[16] = {
                src[3], src[3], src[7], src[7],
                src[11], src[11], src[15], src[15],
                src[19], src[19], src[23], src[23],
                src[27], src[27], src[31], src[31],
            };

            uint8_t r[16], g[16], b[16];
            for (int i = 0; i < 16; i++)
            {
                int32_t c = y[i] - 16;
                int32_t d = u[i] - 128;
                int32_t e = v[i] - 128;

                int32_t t;
                #define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)
                r[i] = clamp((298 * c + 409 * e + 128) >> 8);
                g[i] = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
                b[i] = clamp((298 * c + 516 * d + 128) >> 8);
                #undef clamp
            }

            if (FORMAT == RS2_FORMAT_RGB8)
            {
                uint8_t out[16 * 3] = {
                    r[0], g[0], b[0], r[1], g[1], b[1],
                    r[2], g[2], b[2], r[3], g[3], b[3],
                    r[4], g[4], b[4], r[5], g[5], b[5],
                    r[6], g[6], b[6], r[7], g[7], b[7],
                    r[8], g[8], b[8], r[9], g[9], b[9],
                    r[10], g[10], b[10], r[11], g[11], b[11],
                    r[12], g[12], b[12], r[13], g[13], b[13],
                    r[14], g[14], b[14], r[15], g[15], b[15],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_BGR8)
            {
                uint8_t out[16 * 3] = {
                    b[0], g[0], r[0], b[1], g[1], r[1],
                    b[2], g[2], r[2], b[3], g[3], r[3],
                    b[4], g[4], r[4], b[5], g[5], r[5],
                    b[6], g[6], r[6], b[7], g[7], r[7],
                    b[8], g[8], r[8], b[9], g[9], r[9],
                    b[10], g[10], r[10], b[11], g[11], r[11],
                    b[12], g[12], r[12], b[13], g[13], r[13],
                    b[14], g[14], r[14], b[15], g[15], r[15],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_RGBA8)
            {
                uint8_t out[16 * 4] = {
                    r[0], g[0], b[0], 255, r[1], g[1], b[1], 255,
                    r[2], g[2], b[2], 255, r[3], g[3], b[3], 255,
                    r[4], g[4], b[4], 255, r[5], g[5], b[5], 255,
                    r[6], g[6], b[6], 255, r[7], g[7], b[7], 255,
                    r[8], g[8], b[8], 255, r[9], g[9], b[9], 255,
                    r[10], g[10], b[10], 255, r[11], g[11], b[11], 255,
                    r[12], g[12], b[12], 255, r[13], g[13], b[13], 255,
                    r[14], g[14], b[14], 255, r[15], g[15], b[15], 255,
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_BGRA8)
            {
                uint8_t out[16 * 4] = {
                    b[0], g[0], r[0], 255, b[1], g[1], r[1], 255,
                    b[2], g[2], r[2], 255, b[3], g[3], r[3], 255,
                    b[4], g[4], r[4], 255, b[5], g[5], r[5], 255,
                    b[6], g[6], r[6], 255, b[7], g[7], r[7], 255,
                    b[8], g[8], r[8], 255, b[9], g[9], r[9], 255,
                    b[10], g[10], r[10], 255, b[11], g[11], r[11], 255,
                    b[12], g[12], r[12], 255, b[13], g[13], r[13], 255,
                    b[14], g[14], r[14], 255, b[15], g[15], r[15], 255,
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }
        }
    #endif
    }

    void unpack_yuy2_y8(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_Y8>(d, s, w, h);
    }
    void unpack_yuy2_y16(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_Y16>(d, s, w, h);
    }
    void unpack_yuy2_rgb8(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_RGB8>(d, s, w, h);
    }
    void unpack_yuy2_rgba8(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_RGBA8>(d, s, w, h);
    }
    void unpack_yuy2_bgr8(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_BGR8>(d, s, w, h);
    }
    void unpack_yuy2_bgra8(byte * const d[], const byte * s, int w, int h)
    {
        unpack_yuy2<RS2_FORMAT_BGRA8>(d, s, w, h);
    }

    // This templated function unpacks UYVY into RGB8/RGBA8/BGR8/BGRA8, depending on the compile-time parameter FORMAT.
    // It is expected that all branching outside of the loop control variable will be removed due to constant-folding.
    template<rs2_format FORMAT> void unpack_uyvy(byte * const d[], const byte * s, int width, int height)
    {
        auto n = width * height;
        assert(n % 16 == 0); // All currently supported color resolutions are multiples of 16 pixels. Could easily extend support to other resolutions by copying final n<16 pixels into a zero-padded buffer and recursively calling self for final iteration.
#ifdef __SSSE3__
        auto src = reinterpret_cast<const __m128i *>(s);
        auto dst = reinterpret_cast<__m128i *>(d[0]);
        for (; n; n -= 16)
        {
            const __m128i zero = _mm_set1_epi8(0);
            const __m128i n100 = _mm_set1_epi16(100 << 4);
            const __m128i n208 = _mm_set1_epi16(208 << 4);
            const __m128i n298 = _mm_set1_epi16(298 << 4);
            const __m128i n409 = _mm_set1_epi16(409 << 4);
            const __m128i n516 = _mm_set1_epi16(516 << 4);
            const __m128i evens_odds = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);

            // Load 8 UYVY pixels each into two 16-byte registers
            __m128i s0 = _mm_loadu_si128(src++);
            __m128i s1 = _mm_loadu_si128(src++);


            // Shuffle all Y components to the low order bytes of the register, and all U/V components to the high order bytes
            const __m128i evens_odd1s_odd3s = _mm_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0, 4, 8, 12, 2, 6, 10, 14); // to get yyyyyyyyuuuuvvvv
            __m128i yyyyyyyyuuuuvvvv0 = _mm_shuffle_epi8(s0, evens_odd1s_odd3s);
            __m128i yyyyyyyyuuuuvvvv8 = _mm_shuffle_epi8(s1, evens_odd1s_odd3s);

            // Retrieve all 16 Y components as 16-bit values (8 components per register))
            __m128i y16__0_7 = _mm_unpacklo_epi8(yyyyyyyyuuuuvvvv0, zero);         // convert to 16 bit
            __m128i y16__8_F = _mm_unpacklo_epi8(yyyyyyyyuuuuvvvv8, zero);         // convert to 16 bit


            // Retrieve all 16 U and V components as 16-bit values (8 components per register)
            __m128i uv = _mm_unpackhi_epi32(yyyyyyyyuuuuvvvv0, yyyyyyyyuuuuvvvv8); // uuuuuuuuvvvvvvvv
            __m128i u = _mm_unpacklo_epi8(uv, uv);                                 //  uu uu uu uu uu uu uu uu  u's duplicated
            __m128i v = _mm_unpackhi_epi8(uv, uv);                                 //  vv vv vv vv vv vv vv vv
            __m128i u16__0_7 = _mm_unpacklo_epi8(u, zero);                         // convert to 16 bit
            __m128i u16__8_F = _mm_unpackhi_epi8(u, zero);                         // convert to 16 bit
            __m128i v16__0_7 = _mm_unpacklo_epi8(v, zero);                         // convert to 16 bit
            __m128i v16__8_F = _mm_unpackhi_epi8(v, zero);                         // convert to 16 bit

                                                                                   // Compute R, G, B values for first 8 pixels
            __m128i c16__0_7 = _mm_slli_epi16(_mm_subs_epi16(y16__0_7, _mm_set1_epi16(16)), 4);
            __m128i d16__0_7 = _mm_slli_epi16(_mm_subs_epi16(u16__0_7, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16__0_7 = _mm_slli_epi16(_mm_subs_epi16(v16__0_7, _mm_set1_epi16(128)), 4);
            __m128i r16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(e16__0_7, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(d16__0_7, n100)), _mm_mulhi_epi16(e16__0_7, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16__0_7 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__0_7, n298), _mm_mulhi_epi16(d16__0_7, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

                                                                                                                                                                                                                             // Compute R, G, B values for second 8 pixels
            __m128i c16__8_F = _mm_slli_epi16(_mm_subs_epi16(y16__8_F, _mm_set1_epi16(16)), 4);
            __m128i d16__8_F = _mm_slli_epi16(_mm_subs_epi16(u16__8_F, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16__8_F = _mm_slli_epi16(_mm_subs_epi16(v16__8_F, _mm_set1_epi16(128)), 4);
            __m128i r16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(e16__8_F, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(d16__8_F, n100)), _mm_mulhi_epi16(e16__8_F, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16__8_F = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16__8_F, n298), _mm_mulhi_epi16(d16__8_F, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

            if (FORMAT == RS2_FORMAT_RGB8 || FORMAT == RS2_FORMAT_RGBA8)
            {
                // Shuffle separate R, G, B values into four registers storing four pixels each in (R, G, B, A) order
                __m128i rg8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__0_7, evens_odds), _mm_shuffle_epi8(g16__0_7, evens_odds)); // hi to take the odds which are the upper bytes we care about
                __m128i ba8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__0_7, evens_odds), _mm_set1_epi8(-1));
                __m128i rgba_0_3 = _mm_unpacklo_epi16(rg8__0_7, ba8__0_7);
                __m128i rgba_4_7 = _mm_unpackhi_epi16(rg8__0_7, ba8__0_7);

                __m128i rg8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__8_F, evens_odds), _mm_shuffle_epi8(g16__8_F, evens_odds)); // hi to take the odds which are the upper bytes we care about
                __m128i ba8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__8_F, evens_odds), _mm_set1_epi8(-1));
                __m128i rgba_8_B = _mm_unpacklo_epi16(rg8__8_F, ba8__8_F);
                __m128i rgba_C_F = _mm_unpackhi_epi16(rg8__8_F, ba8__8_F);

                if (FORMAT == RS2_FORMAT_RGBA8)
                {
                    // Store 16 pixels (64 bytes) at once
                    _mm_storeu_si128(dst++, rgba_0_3);
                    _mm_storeu_si128(dst++, rgba_4_7);
                    _mm_storeu_si128(dst++, rgba_8_B);
                    _mm_storeu_si128(dst++, rgba_C_F);
                }

                if (FORMAT == RS2_FORMAT_RGB8)
                {
                    // Shuffle rgb triples to the start and end of each register
                    __m128i rgb0 = _mm_shuffle_epi8(rgba_0_3, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
                    __m128i rgb1 = _mm_shuffle_epi8(rgba_4_7, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
                    __m128i rgb2 = _mm_shuffle_epi8(rgba_8_B, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
                    __m128i rgb3 = _mm_shuffle_epi8(rgba_C_F, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));

                    // Align registers and store 16 pixels (48 bytes) at once
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(rgb1, rgb0, 4));
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(rgb2, rgb1, 8));
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(rgb3, rgb2, 12));
                }
            }

            if (FORMAT == RS2_FORMAT_BGR8 || FORMAT == RS2_FORMAT_BGRA8)
            {
                // Shuffle separate R, G, B values into four registers storing four pixels each in (B, G, R, A) order
                __m128i bg8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__0_7, evens_odds), _mm_shuffle_epi8(g16__0_7, evens_odds)); // hi to take the odds which are the upper bytes we care about
                __m128i ra8__0_7 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__0_7, evens_odds), _mm_set1_epi8(-1));
                __m128i bgra_0_3 = _mm_unpacklo_epi16(bg8__0_7, ra8__0_7);
                __m128i bgra_4_7 = _mm_unpackhi_epi16(bg8__0_7, ra8__0_7);

                __m128i bg8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16__8_F, evens_odds), _mm_shuffle_epi8(g16__8_F, evens_odds)); // hi to take the odds which are the upper bytes we care about
                __m128i ra8__8_F = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16__8_F, evens_odds), _mm_set1_epi8(-1));
                __m128i bgra_8_B = _mm_unpacklo_epi16(bg8__8_F, ra8__8_F);
                __m128i bgra_C_F = _mm_unpackhi_epi16(bg8__8_F, ra8__8_F);

                if (FORMAT == RS2_FORMAT_BGRA8)
                {
                    // Store 16 pixels (64 bytes) at once
                    _mm_storeu_si128(dst++, bgra_0_3);
                    _mm_storeu_si128(dst++, bgra_4_7);
                    _mm_storeu_si128(dst++, bgra_8_B);
                    _mm_storeu_si128(dst++, bgra_C_F);
                }

                if (FORMAT == RS2_FORMAT_BGR8)
                {
                    // Shuffle rgb triples to the start and end of each register
                    __m128i bgr0 = _mm_shuffle_epi8(bgra_0_3, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
                    __m128i bgr1 = _mm_shuffle_epi8(bgra_4_7, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
                    __m128i bgr2 = _mm_shuffle_epi8(bgra_8_B, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
                    __m128i bgr3 = _mm_shuffle_epi8(bgra_C_F, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));

                    // Align registers and store 16 pixels (48 bytes) at once
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(bgr1, bgr0, 4));
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(bgr2, bgr1, 8));
                    _mm_storeu_si128(dst++, _mm_alignr_epi8(bgr3, bgr2, 12));
                }
            }
        }
#else  // Generic code for when SSSE3 is not available.
        auto src = reinterpret_cast<const uint8_t *>(s);
        auto dst = reinterpret_cast<uint8_t *>(d[0]);
        for (; n; n -= 16, src += 32)
        {
            int16_t y[16] = {
                src[1], src[3], src[5], src[7],
                src[9], src[11], src[13], src[15],
                src[17], src[19], src[21], src[23],
                src[25], src[27], src[29], src[31],
            }, u[16] = {
                src[0], src[0], src[4], src[4],
                src[8], src[8], src[12], src[12],
                src[16], src[16], src[20], src[20],
                src[24], src[24], src[28], src[28],
            }, v[16] = {
                src[2], src[2], src[6], src[6],
                src[10], src[10], src[14], src[14],
                src[18], src[18], src[22], src[22],
                src[26], src[26], src[30], src[30],
            };

            uint8_t r[16], g[16], b[16];
            for (int i = 0; i < 16; i++)
            {
                int32_t c = y[i] - 16;
                int32_t d = u[i] - 128;
                int32_t e = v[i] - 128;

                int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)
                r[i] = clamp((298 * c + 409 * e + 128) >> 8);
                g[i] = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
                b[i] = clamp((298 * c + 516 * d + 128) >> 8);
#undef clamp
            }

            if (FORMAT == RS2_FORMAT_RGB8)
            {
                uint8_t out[16 * 3] = {
                    r[0], g[0], b[0], r[1], g[1], b[1],
                    r[2], g[2], b[2], r[3], g[3], b[3],
                    r[4], g[4], b[4], r[5], g[5], b[5],
                    r[6], g[6], b[6], r[7], g[7], b[7],
                    r[8], g[8], b[8], r[9], g[9], b[9],
                    r[10], g[10], b[10], r[11], g[11], b[11],
                    r[12], g[12], b[12], r[13], g[13], b[13],
                    r[14], g[14], b[14], r[15], g[15], b[15],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_BGR8)
            {
                uint8_t out[16 * 3] = {
                    b[0], g[0], r[0], b[1], g[1], r[1],
                    b[2], g[2], r[2], b[3], g[3], r[3],
                    b[4], g[4], r[4], b[5], g[5], r[5],
                    b[6], g[6], r[6], b[7], g[7], r[7],
                    b[8], g[8], r[8], b[9], g[9], r[9],
                    b[10], g[10], r[10], b[11], g[11], r[11],
                    b[12], g[12], r[12], b[13], g[13], r[13],
                    b[14], g[14], r[14], b[15], g[15], r[15],
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_RGBA8)
            {
                uint8_t out[16 * 4] = {
                    r[0], g[0], b[0], 255, r[1], g[1], b[1], 255,
                    r[2], g[2], b[2], 255, r[3], g[3], b[3], 255,
                    r[4], g[4], b[4], 255, r[5], g[5], b[5], 255,
                    r[6], g[6], b[6], 255, r[7], g[7], b[7], 255,
                    r[8], g[8], b[8], 255, r[9], g[9], b[9], 255,
                    r[10], g[10], b[10], 255, r[11], g[11], b[11], 255,
                    r[12], g[12], b[12], 255, r[13], g[13], b[13], 255,
                    r[14], g[14], b[14], 255, r[15], g[15], b[15], 255,
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }

            if (FORMAT == RS2_FORMAT_BGRA8)
            {
                uint8_t out[16 * 4] = {
                    b[0], g[0], r[0], 255, b[1], g[1], r[1], 255,
                    b[2], g[2], r[2], 255, b[3], g[3], r[3], 255,
                    b[4], g[4], r[4], 255, b[5], g[5], r[5], 255,
                    b[6], g[6], r[6], 255, b[7], g[7], r[7], 255,
                    b[8], g[8], r[8], 255, b[9], g[9], r[9], 255,
                    b[10], g[10], r[10], 255, b[11], g[11], r[11], 255,
                    b[12], g[12], r[12], 255, b[13], g[13], r[13], 255,
                    b[14], g[14], r[14], 255, b[15], g[15], r[15], 255,
                };
                librealsense::copy(dst, out, sizeof out);
                dst += sizeof out;
                continue;
            }
        }
#endif
    }

    //////////////////////////////////////
    // 2-in-1 format splitting routines //
    //////////////////////////////////////

    template<class SOURCE, class SPLIT_A, class SPLIT_B> void split_frame(byte * const dest[], int count, const SOURCE * source, SPLIT_A split_a, SPLIT_B split_b)
    {
        auto a = reinterpret_cast<decltype(split_a(SOURCE())) *>(dest[0]);
        auto b = reinterpret_cast<decltype(split_b(SOURCE())) *>(dest[1]);
        for (int i = 0; i < count; ++i)
        {
            *a++ = split_a(*source);
            *b++ = split_b(*source++);
        }
    }

    struct y8i_pixel { uint8_t l, r; };
    void unpack_y8_y8_from_y8i(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
#ifdef RS2_USE_CUDA
        rscuda::split_frame_y8_y8_from_y8i_cuda(dest, count, reinterpret_cast<const y8i_pixel *>(source));
#else
        split_frame(dest, count, reinterpret_cast<const y8i_pixel*>(source),
            [](const y8i_pixel & p) -> uint8_t { return p.l; },
            [](const y8i_pixel & p) -> uint8_t { return p.r; });
#endif
    }

    struct y12i_pixel { uint8_t rl : 8, rh : 4, ll : 4, lh : 8; int l() const { return lh << 4 | ll; } int r() const { return rh << 8 | rl; } };
    void unpack_y16_y16_from_y12i_10(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
#ifdef RS2_USE_CUDA
    rscuda::split_frame_y16_y16_from_y12i_cuda(dest, count, reinterpret_cast<const y12i_pixel *>(source));
#else
        split_frame(dest, count, reinterpret_cast<const y12i_pixel*>(source),
        [](const y12i_pixel & p) -> uint16_t { return p.l() << 6 | p.l() >> 4; },  // We want to convert 10-bit data to 16-bit data
        [](const y12i_pixel & p) -> uint16_t { return p.r() << 6 | p.r() >> 4; }); // Multiply by 64 1/16 to efficiently approximate 65535/1023
#endif
    }

    struct f200_inzi_pixel { uint16_t z16; uint8_t y8; };
    void unpack_z16_y8_from_f200_inzi(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        split_frame(dest, count, reinterpret_cast<const f200_inzi_pixel*>(source),
            [](const f200_inzi_pixel & p) -> uint16_t { return p.z16; },
            [](const f200_inzi_pixel & p) -> uint8_t { return p.y8; });
    }

    void unpack_z16_y16_from_f200_inzi(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        split_frame(dest, count, reinterpret_cast<const f200_inzi_pixel*>(source),
            [](const f200_inzi_pixel & p) -> uint16_t { return p.z16; },
            [](const f200_inzi_pixel & p) -> uint16_t { return p.y8 | p.y8 << 8; });
    }

    void unpack_z16_y8_from_sr300_inzi(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        auto in = reinterpret_cast<const uint16_t*>(source);
        auto out_ir = reinterpret_cast<uint8_t *>(dest[1]);
#ifdef RS2_USE_CUDA
        rscuda::unpack_z16_y8_from_sr300_inzi_cuda(out_ir, in, count);
#else
        for (int i = 0; i < count; ++i) *out_ir++ = *in++ >> 2;
#endif
        librealsense::copy(dest[0], in, count * 2);
    }

    void unpack_z16_y16_from_sr300_inzi (byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        auto in = reinterpret_cast<const uint16_t*>(source);
        auto out_ir = reinterpret_cast<uint16_t*>(dest[1]);
#ifdef RS2_USE_CUDA
        rscuda::unpack_z16_y16_from_sr300_inzi_cuda(out_ir, in, count);
#else
        for (int i = 0; i < count; ++i) *out_ir++ = *in++ << 6;
#endif
        librealsense::copy(dest[0], in, count * 2);
    }

    void unpack_rgb_from_bgr(byte * const dest[], const byte * source, int width, int height)
    {
        auto count = width * height;
        auto in = reinterpret_cast<const uint8_t *>(source);
        auto out = reinterpret_cast<uint8_t *>(dest[0]);

        librealsense::copy(out, in, count * 3);
        for (auto i = 0; i < count; i++)
        {
            std::swap(out[i * 3], out[i * 3 + 2]);
        }
    }

#ifdef ZERO_COPY
    constexpr bool requires_processing = false;
#else
    constexpr bool requires_processing = true;
#endif

    resolution rotate_resolution(resolution res)
    {
        return resolution{ res.height , res.width};
    }

    resolution l500_confidence_resolution(resolution res)
    {
        return resolution{ res.height , res.width * 2 };
    }

    //////////////////////////
    // Native pixel formats //
    //////////////////////////
    const native_pixel_format pf_fe_raw8_unpatched_kernel = { 'RAW8', 1, 1, {  { false,               &copy_pixels<1>,                               { { RS2_STREAM_FISHEYE,        RS2_FORMAT_RAW8 } } } } };
    const native_pixel_format pf_raw8                     = { 'GREY', 1, 1, {  { true,                &copy_pixels<1>,                               { { RS2_STREAM_FISHEYE,        RS2_FORMAT_RAW8 } } } } };
    const native_pixel_format pf_rw16                     = { 'RW16', 1, 2, {  { false,               &copy_pixels<2>,                               { { RS2_STREAM_COLOR,          RS2_FORMAT_RAW16 } } } } };
    const native_pixel_format pf_bayer16                  = { 'BYR2', 1, 2, {  { false,               &copy_pixels<2>,                               { { RS2_STREAM_COLOR,          RS2_FORMAT_RAW16 } } } } };
    const native_pixel_format pf_rw10                     = { 'pRAA', 1, 1, {  { false,               &copy_raw10,                                   { { RS2_STREAM_COLOR,          RS2_FORMAT_RAW10 } } } } };
    // W10 development format will be exposed to the user via Y8
    const native_pixel_format pf_w10                      = { 'W10 ', 1, 1, {  { true,                &unpack_y8_from_rw10,                        { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8 } } } } };

    const native_pixel_format pf_yuy2                     = { 'YUY2', 1, 2, {  { true,                &unpack_yuy2<RS2_FORMAT_RGB8 >,                { { RS2_STREAM_COLOR,          RS2_FORMAT_RGB8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_Y16>,                  { { RS2_STREAM_COLOR,          RS2_FORMAT_Y16 } } },
                                                                               { true,                &copy_pixels<2>,                               { { RS2_STREAM_COLOR,          RS2_FORMAT_YUYV } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_RGBA8>,                { { RS2_STREAM_COLOR,          RS2_FORMAT_RGBA8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_BGR8 >,                { { RS2_STREAM_COLOR,          RS2_FORMAT_BGR8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_BGRA8>,                { { RS2_STREAM_COLOR,          RS2_FORMAT_BGRA8 } } } } };

    const native_pixel_format pf_confidence_l500          = { 'C   ', 1, 1, {  { true,                &unpack_confidence,                            { { RS2_STREAM_CONFIDENCE,     RS2_FORMAT_RAW8, l500_confidence_resolution } } },
                                                                               { requires_processing, &copy_pixels<1>,                               { { RS2_STREAM_CONFIDENCE,     RS2_FORMAT_RAW8 } } } } };
    const native_pixel_format pf_z16_l500                 = { 'Z16 ', 1, 2, {  { true,                &align_l500_image_optimized<2>,              { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16,  rotate_resolution } } },
                                                                               { requires_processing, &copy_pixels<2>,                               { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16                    } } } } };
    const native_pixel_format pf_y8_l500                  = { 'GREY', 1, 1, {  { true,                &align_l500_image_optimized<1>,              { { RS2_STREAM_INFRARED,       RS2_FORMAT_Y8,   rotate_resolution } } },
                                                                               { requires_processing, &copy_pixels<1>,                               { { RS2_STREAM_INFRARED,       RS2_FORMAT_Y8 } } } } };
    const native_pixel_format pf_y8                       = { 'GREY', 1, 1, {  { requires_processing, &copy_pixels<1>,                             { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8  } } } } };
    const native_pixel_format pf_y16                      = { 'Y16 ', 1, 2, {  { true,                &unpack_y16_from_y16_10,                     { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 } } } } };
    const native_pixel_format pf_y8i                      = { 'Y8I ', 1, 2, {  { true,                &unpack_y8_y8_from_y8i,                      { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8  },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 2 },  RS2_FORMAT_Y8 } } } } };
    const native_pixel_format pf_y12i                     = { 'Y12I', 1, 3, {  { true,                &unpack_y16_y16_from_y12i_10,                { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 2 },  RS2_FORMAT_Y16 } } } } };
    const native_pixel_format pf_z16                      = { 'Z16 ', 1, 2, {  { requires_processing, &copy_pixels<2>,                               { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 } } },
        // The Disparity_Z is not applicable for D4XX. TODO - merge with INVZ when confirmed
        /*{ false, &copy_pixels<2>,                                { { RS2_STREAM_DEPTH,    RS2_FORMAT_DISPARITY16 } } }*/ } };
    const native_pixel_format pf_invz                     = { 'Z16 ', 1, 2, {  { true,               &copy_pixels<2>,                               { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 } } } } };
    const native_pixel_format pf_f200_invi                = { 'INVI', 1, 1, {  { true,               &copy_pixels<1>,                             { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8  } } },
                                                                               { true,                &unpack_y16_from_y8,                         { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 } } } } };
    const native_pixel_format pf_f200_inzi                = { 'INZI', 1, 3, {  { true,                &unpack_z16_y8_from_f200_inzi,                 { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8 } } },
                                                                               { true,                &unpack_z16_y16_from_f200_inzi,                { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 } } } } };
    const native_pixel_format pf_sr300_invi               = { 'INVI', 1, 2, {  { true,                &unpack_y8_from_y16_10,                      { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8  } } },
                                                                               { true,                &unpack_y16_from_y16_10,                     { { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 } } } } };
    const native_pixel_format pf_sr300_inzi               = { 'INZI', 2, 2, {  { true,                &unpack_z16_y8_from_sr300_inzi,                { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y8 } } },
                                                                               { true,                &unpack_z16_y16_from_sr300_inzi,               { { RS2_STREAM_DEPTH,          RS2_FORMAT_Z16 },
                                                                                                                                                     { { RS2_STREAM_INFRARED, 1 },  RS2_FORMAT_Y16 } } } } };

    const native_pixel_format pf_uyvyl                    = { 'UYVY', 1, 2, {  { true,                &unpack_uyvy<RS2_FORMAT_RGB8 >,                { { RS2_STREAM_INFRARED,       RS2_FORMAT_RGB8 } } },
                                                                               { true,                &copy_pixels<2>,                               { { RS2_STREAM_INFRARED,       RS2_FORMAT_UYVY } } },
                                                                               { true,                &unpack_uyvy<RS2_FORMAT_RGBA8>,                { { RS2_STREAM_INFRARED,       RS2_FORMAT_RGBA8} } },
                                                                               { true,                &unpack_uyvy<RS2_FORMAT_BGR8 >,                { { RS2_STREAM_INFRARED,       RS2_FORMAT_BGR8 } } },
                                                                               { true,                &unpack_uyvy<RS2_FORMAT_BGRA8>,                { { RS2_STREAM_INFRARED,       RS2_FORMAT_BGRA8} } } } };

    const native_pixel_format pf_rgb888                   = { 'RGB2', 1, 2, {  { true,                &unpack_rgb_from_bgr,                          { { RS2_STREAM_INFRARED,       RS2_FORMAT_RGB8 } } } } };


    const native_pixel_format pf_yuyv                     = { 'YUYV', 1, 2, {  { true,                &unpack_yuy2<RS2_FORMAT_RGB8 >,                { { RS2_STREAM_COLOR,          RS2_FORMAT_RGB8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_Y16>,                  { { RS2_STREAM_COLOR,          RS2_FORMAT_Y16 } } },
                                                                               { true,                &copy_pixels<2>,                               { { RS2_STREAM_COLOR,          RS2_FORMAT_YUYV } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_RGBA8>,                { { RS2_STREAM_COLOR,          RS2_FORMAT_RGBA8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_BGR8 >,                { { RS2_STREAM_COLOR,          RS2_FORMAT_BGR8 } } },
                                                                               { true,                &unpack_yuy2<RS2_FORMAT_BGRA8>,                { { RS2_STREAM_COLOR,          RS2_FORMAT_BGRA8 } } } } };

    const native_pixel_format pf_accel_axes               = { 'ACCL', 1, 1, {  { true,                &unpack_accel_axes<RS2_FORMAT_MOTION_XYZ32F>,  { { RS2_STREAM_ACCEL,          RS2_FORMAT_MOTION_XYZ32F } } } } };
                                                                               //{ false,               &unpack_hid_raw_data,                          { { RS2_STREAM_ACCEL,          RS2_FORMAT_MOTION_RAW  } } } } };
    const native_pixel_format pf_gyro_axes                = { 'GYRO', 1, 1, {  { true,                &unpack_gyro_axes<RS2_FORMAT_MOTION_XYZ32F>,   { { RS2_STREAM_GYRO,           RS2_FORMAT_MOTION_XYZ32F } } } } };
                                                                               //{ false,               &unpack_hid_raw_data,                          { { RS2_STREAM_GYRO,           RS2_FORMAT_MOTION_RAW  } } } } };
    const native_pixel_format pf_gpio_timestamp           = { 'GPIO', 1, 1, {  { false,               &unpack_input_reports_data,                  { { { RS2_STREAM_GPIO, 1 },      RS2_FORMAT_GPIO_RAW },
                                                                                                                                                     { { RS2_STREAM_GPIO, 2 },      RS2_FORMAT_GPIO_RAW },
                                                                                                                                                     { { RS2_STREAM_GPIO, 3 },      RS2_FORMAT_GPIO_RAW },
                                                                                                                                                     { { RS2_STREAM_GPIO, 4 },      RS2_FORMAT_GPIO_RAW }} } } };
    }

#pragma pack(pop)
