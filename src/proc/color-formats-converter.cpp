// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "color-formats-converter.h"

#include "option.h"
#include "image-avx.h"
#include "image.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../third-party/stb_image.h"

#ifdef RS2_USE_CUDA
#include "cuda/cuda-conversion.cuh"
#endif
#ifdef __SSSE3__
#include <tmmintrin.h> // For SSSE3 intrinsics
#endif

#if defined (ANDROID) || (defined (__linux__) && !defined (__x86_64__)) || (defined (__APPLE__) && !defined (__x86_64__))

bool has_avx() { return false; }

#else

#ifdef _WIN32
#include <intrin.h>
#define cpuid(info, x)    __cpuidex(info, x, 0)
#else
#include <cpuid.h>
void cpuid(int info[4], int info_type) {
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

namespace librealsense 
{
    /////////////////////////////
    // YUY2 unpacking routines //
    /////////////////////////////
    // This templated function unpacks YUY2 into Y8/Y16/RGB8/RGBA8/BGR8/BGRA8, depending on the compile-time parameter FORMAT.
    // It is expected that all branching outside of the loop control variable will be removed due to constant-folding.
    template<rs2_format FORMAT> void unpack_yuy2(byte * const d[], const byte * s, int width, int height, int actual_size)
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

    void unpack_yuy2(rs2_format dst_format, rs2_stream dst_stream, byte * const d[], const byte * s, int w, int h, int actual_size)
    {
        switch (dst_format)
        {
        case RS2_FORMAT_Y8:
            unpack_yuy2<RS2_FORMAT_Y8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_Y16:
            unpack_yuy2<RS2_FORMAT_Y16>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_RGB8:
            unpack_yuy2<RS2_FORMAT_RGB8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_RGBA8:
            unpack_yuy2<RS2_FORMAT_RGBA8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_BGR8:
            unpack_yuy2<RS2_FORMAT_BGR8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_BGRA8:
            unpack_yuy2<RS2_FORMAT_BGRA8>(d, s, w, h, actual_size);
            break;
        default:
            LOG_ERROR("Unsupported format for YUY2 conversion.");
            break;
        }
    }


    /////////////////////////////
    // UYVY unpacking routines //
    /////////////////////////////
    // This templated function unpacks UYVY into RGB8/RGBA8/BGR8/BGRA8, depending on the compile-time parameter FORMAT.
    // It is expected that all branching outside of the loop control variable will be removed due to constant-folding.
    template<rs2_format FORMAT> void unpack_uyvy(byte * const d[], const byte * s, int width, int height, int actual_size)
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

    void unpack_uyvyc(rs2_format dst_format, rs2_stream dst_stream, byte * const d[], const byte * s, int w, int h, int actual_size)
    {
        switch (dst_format)
        {
        case RS2_FORMAT_RGB8:
            unpack_uyvy<RS2_FORMAT_RGB8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_RGBA8:
            unpack_uyvy<RS2_FORMAT_RGBA8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_BGR8:
            unpack_uyvy<RS2_FORMAT_BGR8>(d, s, w, h, actual_size);
            break;
        case RS2_FORMAT_BGRA8:
            unpack_uyvy<RS2_FORMAT_BGRA8>(d, s, w, h, actual_size);
            break;
        default:
            LOG_ERROR("Unsupported format for UYVY conversion.");
            break;
        }
    }

    /////////////////////////////
    // MJPEG unpacking routines //
    /////////////////////////////
    void unpack_mjpeg(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        int w, h, bpp;
        auto uncompressed_rgb = stbi_load_from_memory(source, actual_size, &w, &h, &bpp, false);
        if (uncompressed_rgb)
        {
            auto uncompressed_size = w * h * bpp;
            librealsense::copy(dest[0], uncompressed_rgb, uncompressed_size);
            stbi_image_free(uncompressed_rgb);
        }
        else
            LOG_ERROR("jpeg decode failed");
    }

    /////////////////////////////
    // BGR unpacking routines //
    /////////////////////////////
    void unpack_rgb_from_bgr(byte * const dest[], const byte * source, int width, int height, int actual_size)
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

    void yuy2_converter::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        unpack_yuy2(_target_format, _target_stream, dest, source, width, height, actual_size);
    }

    void uyvy_converter::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        unpack_uyvyc(_target_format, _target_stream, dest, source, width, height, actual_size);
    }

    void mjpeg_converter::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        unpack_mjpeg(dest, source, width, height, actual_size, input_size);
    }

    void bgr_to_rgb::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        unpack_rgb_from_bgr(dest, source, width, height, actual_size);
    }
}
