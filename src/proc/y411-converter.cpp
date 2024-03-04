// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "y411-converter.h"

#ifdef RS2_USE_CUDA
#include "cuda/cuda-conversion.cuh"
#endif
#ifdef __SSSE3__
#include <tmmintrin.h> // For SSSE3 intrinsics
#endif

namespace librealsense 
{
    void convert_yuv_to_rgb(const uint8_t yuv[3], uint8_t * rgb)
    {
        int32_t c = yuv[0] - 16;
        int32_t d = yuv[1] - 128;
        int32_t e = yuv[2] - 128;

        int32_t t;
#define clamp( x ) ( ( t = ( x ) ) > 255 ? 255 : t < 0 ? 0 : t )
        rgb[0] = clamp((298 * c + 409 * e + 128) >> 8);
        rgb[1] = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
        rgb[2] = clamp((298 * c + 516 * d + 128) >> 8);
#undef clamp
    }

    // Y is luminance and U,V are chrome 
    // Each 4 pixels share a single U,V 
    // 
    // So, for a source image of pixels numbered: 
    //     P0 P1 P4 P5 P8 P9 ... 
    //     P2 P3 P6 P7 P10 P11 ... 
    // The encoding takes: 
    //     [y0 u0-3 v0-3] [y1 u0-3 v0-3] [y4 u4-7 v4-7] [y5 u4-7 v4-7] [y8 u8-11 v8-11] [y9 u8-11 v8-11] 
    //     [y2 u0-3 v0-3] [y3 u0-3 v0-3] [y6 u4-7 v4-7] [y7 u4-7 v4-7] [y10 u8-11 v8-11] [y11 u8-11 v8-11] 
    // And arranges them as sequential bytes: 
    //     [u0-3 y0 y1 v0-3 y2 y3] [u4-7 y4 y5 v4-7 y6 y7] [u8-11 y8 y9 v8-11 y10 y11] 
    // So decoding happens in reverse. 
    // 
    // The destination bytes we write are in RGB8. 
    // 
    // See https://www.fourcc.org/pixel-format/yuv-y411/ 
    //

#if defined __SSSE3__ && ! defined ANDROID
    void unpack_y411_sse( uint8_t * const dest, const uint8_t * const s, int w, int h, int actual_size)
    {
        auto n = w * h;
        // working each iteration on 8 y411 pixels, and extract 4 rgb pixels from each one
        // so we get 32 rgb pixels
        assert(n % 32 == 0); // All currently supported color resolutions are multiples of 32 pixels. Could easily extend support to other resolutions by copying final n<32 pixels into a zero-padded buffer and recursively calling self for final iteration.

        auto src = reinterpret_cast<const __m128i *>(s);
        auto dst = reinterpret_cast<__m128i *>(dest);

        const __m128i zero = _mm_set1_epi8(0);
        const __m128i n100 = _mm_set1_epi16(100 << 4);
        const __m128i n208 = _mm_set1_epi16(208 << 4);
        const __m128i n298 = _mm_set1_epi16(298 << 4);
        const __m128i n409 = _mm_set1_epi16(409 << 4);
        const __m128i n516 = _mm_set1_epi16(516 << 4);

        // shuffle to y,u,v of pixels 1-2
        const __m128i shuffle_y_1_2_0 = _mm_setr_epi8(1, 2, 4, 5, 7, 8, 10, 11, 0, 0, 0, 0, 0, 0, 0, 0);    // to get   yyyyyyyy00000000
        const __m128i shuffle_u_1_2_0 = _mm_setr_epi8(0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   uuuuuuuu00000000
        const __m128i shuffle_v_1_2_0 = _mm_setr_epi8(3, 3, 3, 3, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   vvvvvvvv00000000

        // shuffle to y,u,v of pixels 3-4 - combination of registers 0 and 1
        const __m128i shuffle_y_3_4_0 = _mm_setr_epi8(13, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   yy00000000000000
        const __m128i mask_y_3_4_0 = _mm_setr_epi8(-1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);           // to zero the other bytes
        const __m128i shuffle_u_3_4_0 = _mm_setr_epi8(12, 12, 12, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   uuuu000000000000
        const __m128i shuffle_v_3_4_0 = _mm_setr_epi8(15, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   vvvv000000000000
        const __m128i shuffle_y_3_4_1 = _mm_setr_epi8(0, 0, 0, 1, 3, 4, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0);          // to get   00yyyyyy00000000
        const __m128i mask_y_3_4_1 = _mm_setr_epi8(0, 0, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i shuffle_u_3_4_1 = _mm_setr_epi8(0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   0000uuuu00000000
        const __m128i shuffle_v_3_4_1 = _mm_setr_epi8(0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   0000vvvv00000000

        // shuffle to y,u,v of pixels 5-6- combination of registers 1 and 2
        const __m128i shuffle_y_5_6_1 = _mm_setr_epi8(9, 10, 12, 13, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   yyyyy00000000000
        const __m128i mask_y_5_6_1 = _mm_setr_epi8(-1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i shuffle_u_5_6_1 = _mm_setr_epi8(8, 8, 8, 8, 14, 14, 14, 14, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   uuuuuuuu00000000
        const __m128i shuffle_v_5_6_1 = _mm_setr_epi8(11, 11, 11, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);      // to get   vvvv000000000000
        const __m128i shuffle_y_5_6_2 = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0);          // to get   00000yyy00000000
        const __m128i mask_y_5_6_2 = _mm_setr_epi8(0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i shuffle_v_5_6_2 = _mm_setr_epi8(0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0);          // to get   0000vvvv00000000

        // shuffle to y,u,v of pixels 7-8
        const __m128i shuffle_y_7_8_2 = _mm_setr_epi8(5, 6, 8, 9, 11, 12, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   yyyyyyyy00000000
        const __m128i shuffle_u_7_8_2 = _mm_setr_epi8(4, 4, 4, 4, 10, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   uuuuuuuu00000000
        const __m128i shuffle_v_7_8_2 = _mm_setr_epi8(7, 7, 7, 7, 13, 13, 13, 13, 0, 0, 0, 0, 0, 0, 0, 0);        // to get   vvvvvvvv00000000

        const __m128i mask_uv_0 = _mm_setr_epi8(-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        const __m128i mask_uv_1 = _mm_setr_epi8(0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0);

//#pragma omp parallel for
        for (int i = 0; i < n / 32; i++)
        {
            // Load 8 y411 pixels into 3 16-byte registers
            __m128i s0 = _mm_loadu_si128(&src[i * 3]);
            __m128i s1 = _mm_loadu_si128(&src[i * 3 + 1]);
            __m128i s2 = _mm_loadu_si128(&src[i * 3 + 2]);

            // pixels 1-2
            __m128i pixel_y_1_2 = _mm_shuffle_epi8(s0, shuffle_y_1_2_0);
            __m128i pixel_u_1_2 = _mm_shuffle_epi8(s0, shuffle_u_1_2_0);
            __m128i pixel_v_1_2 = _mm_shuffle_epi8(s0, shuffle_v_1_2_0);

            // pixels 3-4
            __m128i pixel_y_3_4_register_0 = _mm_shuffle_epi8(s0, shuffle_y_3_4_0);
            __m128i pixel_y_3_4_register_1 = _mm_shuffle_epi8(s1, shuffle_y_3_4_1);
            pixel_y_3_4_register_0 = _mm_and_si128(pixel_y_3_4_register_0, mask_y_3_4_0);
            pixel_y_3_4_register_1 = _mm_and_si128(pixel_y_3_4_register_1, mask_y_3_4_1);
            __m128i pixel_y_3_4 = _mm_or_si128(pixel_y_3_4_register_0, pixel_y_3_4_register_1);

            __m128i pixel_u_3_4_register_0 = _mm_shuffle_epi8(s0, shuffle_u_3_4_0);
            __m128i pixel_u_3_4_register_1 = _mm_shuffle_epi8(s1, shuffle_u_3_4_1);
            pixel_u_3_4_register_0 = _mm_and_si128(pixel_u_3_4_register_0, mask_uv_0);
            pixel_u_3_4_register_1 = _mm_and_si128(pixel_u_3_4_register_1, mask_uv_1);
            __m128i pixel_u_3_4 = _mm_or_si128(pixel_u_3_4_register_0, pixel_u_3_4_register_1);

            __m128i pixel_v_3_4_register_0 = _mm_shuffle_epi8(s0, shuffle_v_3_4_0);
            __m128i pixel_v_3_4_register_1 = _mm_shuffle_epi8(s1, shuffle_v_3_4_1);
            pixel_v_3_4_register_0 = _mm_and_si128(pixel_v_3_4_register_0, mask_uv_0);
            pixel_v_3_4_register_1 = _mm_and_si128(pixel_v_3_4_register_1, mask_uv_1);
            __m128i pixel_v_3_4 = _mm_or_si128(pixel_v_3_4_register_0, pixel_v_3_4_register_1);

            // pixels 5-6
            __m128i pixel_y_5_6_register_1 = _mm_shuffle_epi8(s1, shuffle_y_5_6_1);
            __m128i pixel_y_5_6_register_2 = _mm_shuffle_epi8(s2, shuffle_y_5_6_2);
            pixel_y_5_6_register_1 = _mm_and_si128(pixel_y_5_6_register_1, mask_y_5_6_1);
            pixel_y_5_6_register_2 = _mm_and_si128(pixel_y_5_6_register_2, mask_y_5_6_2);
            __m128i pixel_y_5_6 = _mm_or_si128(pixel_y_5_6_register_1, pixel_y_5_6_register_2);

            __m128i pixel_u_5_6_register_1 = _mm_shuffle_epi8(s1, shuffle_u_5_6_1);
            __m128i mask_uv = _mm_or_si128(mask_uv_0, mask_uv_1);
            __m128i pixel_u_5_6 = _mm_and_si128(pixel_u_5_6_register_1, mask_uv);

            __m128i pixel_v_5_6_register_1 = _mm_shuffle_epi8(s1, shuffle_v_5_6_1);
            __m128i pixel_v_5_6_register_2 = _mm_shuffle_epi8(s2, shuffle_v_5_6_2);
            pixel_v_5_6_register_1 = _mm_and_si128(pixel_v_5_6_register_1, mask_uv_0);
            pixel_v_5_6_register_2 = _mm_and_si128(pixel_v_5_6_register_2, mask_uv_1);
            __m128i pixel_v_5_6 = _mm_or_si128(pixel_v_5_6_register_1, pixel_v_5_6_register_2);

            // pixels 7-8
            __m128i pixel_y_7_8 = _mm_shuffle_epi8(s2, shuffle_y_7_8_2);
            __m128i pixel_u_7_8 = _mm_shuffle_epi8(s2, shuffle_u_7_8_2);
            __m128i pixel_v_7_8 = _mm_shuffle_epi8(s2, shuffle_v_7_8_2);

            // Retrieve all 32 Y components as 16-bit values (8 components per register))
            // Retrieve all 8 u components as 16-bit values (2 components per register))
            // Retrieve all 8 v components as 16-bit values (2 components per register))
            __m128i y16_pix_1_2 = _mm_unpacklo_epi8(pixel_y_1_2, zero);         // convert to 16 bit
            __m128i u16_pix_1_2 = _mm_unpacklo_epi8(pixel_u_1_2, zero);         // convert to 16 bit
            __m128i v16_pix_1_2 = _mm_unpacklo_epi8(pixel_v_1_2, zero);

            __m128i y16_pix_3_4 = _mm_unpacklo_epi8(pixel_y_3_4, zero);         // convert to 16 bit
            __m128i u16_pix_3_4 = _mm_unpacklo_epi8(pixel_u_3_4, zero);                         // convert to 16 bit
            __m128i v16_pix_3_4 = _mm_unpacklo_epi8(pixel_v_3_4, zero);

            __m128i y16_pix_5_6 = _mm_unpacklo_epi8(pixel_y_5_6, zero);         // convert to 16 bit
            __m128i u16_pix_5_6 = _mm_unpacklo_epi8(pixel_u_5_6, zero);                         // convert to 16 bit
            __m128i v16_pix_5_6 = _mm_unpacklo_epi8(pixel_v_5_6, zero);

            __m128i y16_pix_7_8 = _mm_unpacklo_epi8(pixel_y_7_8, zero);         // convert to 16 bit
            __m128i u16_pix_7_8 = _mm_unpacklo_epi8(pixel_u_7_8, zero);                         // convert to 16 bit
            __m128i v16_pix_7_8 = _mm_unpacklo_epi8(pixel_v_7_8, zero);

            // r,g,b
            __m128i c16_pix_1_2 = _mm_slli_epi16(_mm_subs_epi16(y16_pix_1_2, _mm_set1_epi16(16)), 4);
            __m128i d16_pix_1_2 = _mm_slli_epi16(_mm_subs_epi16(u16_pix_1_2, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16_pix_1_2 = _mm_slli_epi16(_mm_subs_epi16(v16_pix_1_2, _mm_set1_epi16(128)), 4);
            __m128i r16_pix_1_2 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_1_2, n298), _mm_mulhi_epi16(e16_pix_1_2, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16_pix_1_2 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16_pix_1_2, n298), _mm_mulhi_epi16(d16_pix_1_2, n100)), _mm_mulhi_epi16(e16_pix_1_2, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16_pix_1_2 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_1_2, n298), _mm_mulhi_epi16(d16_pix_1_2, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

            __m128i c16_pix_3_4 = _mm_slli_epi16(_mm_subs_epi16(y16_pix_3_4, _mm_set1_epi16(16)), 4);
            __m128i d16_pix_3_4 = _mm_slli_epi16(_mm_subs_epi16(u16_pix_3_4, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16_pix_3_4 = _mm_slli_epi16(_mm_subs_epi16(v16_pix_3_4, _mm_set1_epi16(128)), 4);
            __m128i r16_pix_3_4 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_3_4, n298), _mm_mulhi_epi16(e16_pix_3_4, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16_pix_3_4 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16_pix_3_4, n298), _mm_mulhi_epi16(d16_pix_3_4, n100)), _mm_mulhi_epi16(e16_pix_3_4, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16_pix_3_4 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_3_4, n298), _mm_mulhi_epi16(d16_pix_3_4, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

            __m128i c16_pix_5_6 = _mm_slli_epi16(_mm_subs_epi16(y16_pix_5_6, _mm_set1_epi16(16)), 4);
            __m128i d16_pix_5_6 = _mm_slli_epi16(_mm_subs_epi16(u16_pix_5_6, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16_pix_5_6 = _mm_slli_epi16(_mm_subs_epi16(v16_pix_5_6, _mm_set1_epi16(128)), 4);
            __m128i r16_pix_5_6 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_5_6, n298), _mm_mulhi_epi16(e16_pix_5_6, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16_pix_5_6 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16_pix_5_6, n298), _mm_mulhi_epi16(d16_pix_5_6, n100)), _mm_mulhi_epi16(e16_pix_5_6, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16_pix_5_6 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_5_6, n298), _mm_mulhi_epi16(d16_pix_5_6, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

            __m128i c16_pix_7_8 = _mm_slli_epi16(_mm_subs_epi16(y16_pix_7_8, _mm_set1_epi16(16)), 4);
            __m128i d16_pix_7_8 = _mm_slli_epi16(_mm_subs_epi16(u16_pix_7_8, _mm_set1_epi16(128)), 4); // perhaps could have done these u,v to d,e before the duplication
            __m128i e16_pix_7_8 = _mm_slli_epi16(_mm_subs_epi16(v16_pix_7_8, _mm_set1_epi16(128)), 4);
            __m128i r16_pix_7_8 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_7_8, n298), _mm_mulhi_epi16(e16_pix_7_8, n409))))));                                                 // (298 * c + 409 * e + 128) ; //
            __m128i g16_pix_7_8 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_sub_epi16(_mm_sub_epi16(_mm_mulhi_epi16(c16_pix_7_8, n298), _mm_mulhi_epi16(d16_pix_7_8, n100)), _mm_mulhi_epi16(e16_pix_7_8, n208)))))); // (298 * c - 100 * d - 208 * e + 128)
            __m128i b16_pix_7_8 = _mm_min_epi16(_mm_set1_epi16(255), _mm_max_epi16(zero, ((_mm_add_epi16(_mm_mulhi_epi16(c16_pix_7_8, n298), _mm_mulhi_epi16(d16_pix_7_8, n516))))));                                                 // clampbyte((298 * c + 516 * d + 128) >> 8);

            // Shuffle separate R, G, B values into four registers storing four pixels each in (R, G, B, A) order
            const __m128i evens_odds = _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15);

            __m128i rg8_pix_1_2 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16_pix_1_2, evens_odds), _mm_shuffle_epi8(g16_pix_1_2, evens_odds)); // hi to take the odds which are the upper bytes we care about
            __m128i ba8_pix_1_2 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16_pix_1_2, evens_odds), _mm_set1_epi8(-1));
            __m128i rg8_pix_3_4 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16_pix_3_4, evens_odds), _mm_shuffle_epi8(g16_pix_3_4, evens_odds)); // hi to take the odds which are the upper bytes we care about
            __m128i ba8_pix_3_4 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16_pix_3_4, evens_odds), _mm_set1_epi8(-1));
            __m128i rg8_pix_5_6 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16_pix_5_6, evens_odds), _mm_shuffle_epi8(g16_pix_5_6, evens_odds)); // hi to take the odds which are the upper bytes we care about
            __m128i ba8_pix_5_6 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16_pix_5_6, evens_odds), _mm_set1_epi8(-1));
            __m128i rg8_pix_7_8 = _mm_unpacklo_epi8(_mm_shuffle_epi8(r16_pix_7_8, evens_odds), _mm_shuffle_epi8(g16_pix_7_8, evens_odds)); // hi to take the odds which are the upper bytes we care about
            __m128i ba8_pix_7_8 = _mm_unpacklo_epi8(_mm_shuffle_epi8(b16_pix_7_8, evens_odds), _mm_set1_epi8(-1));

            __m128i rgba_0_3 = _mm_unpacklo_epi16(rg8_pix_1_2, ba8_pix_1_2);
            __m128i rgba_4_7 = _mm_unpackhi_epi16(rg8_pix_1_2, ba8_pix_1_2);
            __m128i rgba_8_11 = _mm_unpacklo_epi16(rg8_pix_3_4, ba8_pix_3_4);
            __m128i rgba_12_15 = _mm_unpackhi_epi16(rg8_pix_3_4, ba8_pix_3_4);
            __m128i rgba_16_19 = _mm_unpacklo_epi16(rg8_pix_5_6, ba8_pix_5_6);
            __m128i rgba_20_23 = _mm_unpackhi_epi16(rg8_pix_5_6, ba8_pix_5_6);
            __m128i rgba_24_27 = _mm_unpacklo_epi16(rg8_pix_7_8, ba8_pix_7_8);
            __m128i rgba_28_32 = _mm_unpackhi_epi16(rg8_pix_7_8, ba8_pix_7_8);

            // Shuffle rgb triples to the start and end of each register
            __m128i rgba_0_7_l0 = _mm_unpacklo_epi64(rgba_0_3, rgba_4_7);
            __m128i rgba_0_7_l1 = _mm_unpackhi_epi64(rgba_0_3, rgba_4_7);
            __m128i rgba_8_15_l0 = _mm_unpacklo_epi64(rgba_8_11, rgba_12_15);
            __m128i rgba_8_15_l1 = _mm_unpackhi_epi64(rgba_8_11, rgba_12_15);
            __m128i rgba_16_23_l0 = _mm_unpacklo_epi64(rgba_16_19, rgba_20_23);
            __m128i rgba_16_23_l1 = _mm_unpackhi_epi64(rgba_16_19, rgba_20_23);
            __m128i rgba_24_32_l0 = _mm_unpacklo_epi64(rgba_24_27, rgba_28_32);
            __m128i rgba_24_32_l1 = _mm_unpackhi_epi64(rgba_24_27, rgba_28_32);

            // Shuffle rgb triples to the start and end of each register
            __m128i rgb0_l0 = _mm_shuffle_epi8(rgba_0_7_l0, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
            __m128i rgb1_l0 = _mm_shuffle_epi8(rgba_8_15_l0, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
            __m128i rgb2_l0 = _mm_shuffle_epi8(rgba_16_23_l0, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
            __m128i rgb3_l0 = _mm_shuffle_epi8(rgba_24_32_l0, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));


            // calculate the current line and column
            auto num_on_regs_at_once = 3;
            auto rgb_bpp = 3;
            auto reg_num_on_line = w * rgb_bpp / 16;
            auto line = (i*num_on_regs_at_once) / reg_num_on_line;
            auto j = i % (reg_num_on_line / num_on_regs_at_once);

            // Align registers and store 16 pixels (48 bytes) at once on the line above
            _mm_storeu_si128(&dst[(line*2 ) *reg_num_on_line + j * 3], _mm_alignr_epi8(rgb1_l0, rgb0_l0, 4));
            _mm_storeu_si128(&dst[(line*2 ) * reg_num_on_line + j * 3 + 1], _mm_alignr_epi8(rgb2_l0, rgb1_l0, 8));
            _mm_storeu_si128(&dst[(line*2 ) * reg_num_on_line + j * 3 + 2], _mm_alignr_epi8(rgb3_l0, rgb2_l0, 12));

            // Shuffle rgb triples to the start and end of each register
            __m128i rgb0_l1 = _mm_shuffle_epi8(rgba_0_7_l1, _mm_setr_epi8(3, 7, 11, 15, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14));
            __m128i rgb1_l1 = _mm_shuffle_epi8(rgba_8_15_l1, _mm_setr_epi8(0, 1, 2, 4, 3, 7, 11, 15, 5, 6, 8, 9, 10, 12, 13, 14));
            __m128i rgb2_l1 = _mm_shuffle_epi8(rgba_16_23_l1, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 3, 7, 11, 15, 10, 12, 13, 14));
            __m128i rgb3_l1 = _mm_shuffle_epi8(rgba_24_32_l1, _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 3, 7, 11, 15));

            // Align registers and store 16 pixels(48 bytes) at once on the line bellow
            _mm_storeu_si128(&dst[(line*2  + 1) *reg_num_on_line + j * 3], _mm_alignr_epi8(rgb1_l1, rgb0_l1, 4));
            _mm_storeu_si128(&dst[(line*2  + 1) * reg_num_on_line + j * 3 + 1], _mm_alignr_epi8(rgb2_l1, rgb1_l1, 8));
            _mm_storeu_si128(&dst[(line*2  + 1) * reg_num_on_line + j * 3 + 2], _mm_alignr_epi8(rgb3_l1, rgb2_l1, 12));
        }
    }
#endif

    void unpack_y411_native( uint8_t * const dest, const uint8_t * const s, int w, int h, int actual_size)
    {
        auto index_source = 0;
        for (auto i = 0; i < h; i += 2)
        {
            for (auto j = 0; j < w; j += 2)
            {
                auto y411_pix = &s[index_source];
                // [u y0 y1 v y2 y3]
                auto u = y411_pix[0];
                auto l0_y0 = y411_pix[1];
                auto l0_y1 = y411_pix[2];
                auto v = y411_pix[3];
                auto l1_y0 = y411_pix[4];
                auto l1_y1 = y411_pix[5];

                uint8_t yuv0_0[3] = { l0_y0, u, v };
                convert_yuv_to_rgb(yuv0_0, &dest[i * w * 3 + j * 3]);

                uint8_t yuv0_1[3] = { l0_y1, u, v };
                convert_yuv_to_rgb(yuv0_1, &dest[i * w * 3 + j * 3 + 3]);

                uint8_t yuv1_0[3] = { l1_y0, u, v };
                convert_yuv_to_rgb(yuv1_0, &dest[(i + 1) * w * 3 + j * 3]);

                uint8_t yuv1_1[3] = { l1_y1, u, v };
                convert_yuv_to_rgb(yuv1_1, &dest[(i + 1) * w * 3 + j * 3 + 3]);

                index_source += 6;
            }
        }
    }

    // This function unpacks Y411 format into RGB8 using SSE if defined
    // The size of the frame must be bigger than 4 pixels and product of 32
    void unpack_y411( uint8_t * const dest[], const uint8_t * const s, int w, int h, int actual_size )
    {
#if defined __SSSE3__ && ! defined ANDROID
        unpack_y411_sse(dest[0], s, w, h, actual_size);
#else
        unpack_y411_native(dest[0], s, w, h, actual_size);
#endif
    }

    void y411_converter::process_function( uint8_t * const dest[],
        const uint8_t * source,
        int width,
        int height,
        int actual_size,
        int input_size)
    {
        unpack_y411(dest, source, width, height, actual_size);
    }  // namespace librealsense
}
