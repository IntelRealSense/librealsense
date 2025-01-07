// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "downsample.h"

#include <assert.h>

#ifdef __SSSE3__
#include <emmintrin.h>
#include <smmintrin.h>
#endif

void downsample_min_4x4(const cv::Mat& source, cv::Mat* pDest)
{
    static constexpr auto DOWNSAMPLE_FACTOR = 4;

    assert(source.cols % 8 == 0);
    assert(source.rows % 4 == 0);
    assert(source.type() == CV_16U);

    assert(pDest->cols == source.cols / DOWNSAMPLE_FACTOR);
    assert(pDest->rows == source.rows / DOWNSAMPLE_FACTOR);
    assert(pDest->type() == CV_16U);

    const size_t sizeYresized = source.rows / DOWNSAMPLE_FACTOR;

#ifdef __SSSE3__
    __m128i ones = _mm_set1_epi16(1);

    // Note on multi-threading here, 2018-08-17
    // This function is called for every depth image coming from RealSense
    // without MT this function takes on Joule in average 0.47 ms
    // with    MT this function takes on Joule in average 0.15 ms
    #pragma omp parallel for
    for (int y = 0; y < sizeYresized; y++)
    {
        for (uint16_t x = 0; x < source.cols; x += 8) {
            const int newY = y * 4;
            // load data rows
            __m128i A = _mm_loadu_si128((const __m128i*)&source.at<uint16_t>(newY, x));
            __m128i B = _mm_loadu_si128((const __m128i*)&source.at<uint16_t>(newY + 1, x));
            __m128i C = _mm_loadu_si128((const __m128i*)&source.at<uint16_t>(newY + 2, x));
            __m128i D = _mm_loadu_si128((const __m128i*)&source.at<uint16_t>(newY + 3, x));

            // subtract 1 to shift invalid pixels to max value (16bit integer underflow)
            A = _mm_sub_epi16(A, ones);
            B = _mm_sub_epi16(B, ones);
            C = _mm_sub_epi16(C, ones);
            D = _mm_sub_epi16(D, ones);

            // calculate minimum
            __m128i rowMin = _mm_min_epu16(D, C);
            rowMin = _mm_min_epu16(rowMin, B);
            rowMin = _mm_min_epu16(rowMin, A);

            __m128i shuf32 = _mm_shuffle_epi32(rowMin, _MM_SHUFFLE(2, 3, 0, 1));

            __m128i min32 = _mm_min_epu16(rowMin, shuf32);

            __m128i shuf16 = _mm_shufflelo_epi16(min32, _MM_SHUFFLE(3, 2, 0, 1));
            shuf16 = _mm_shufflehi_epi16(shuf16, _MM_SHUFFLE(3, 2, 0, 1));

            __m128i min2 = _mm_min_epu16(min32, shuf16);

            // undo invalid pixel shifting by adding one
            min2 = _mm_add_epi16(min2, ones);

            uint16_t minA = _mm_extract_epi16(min2, 0);
            uint16_t minB = _mm_extract_epi16(min2, 4);

            pDest->at<uint16_t>(y, x / DOWNSAMPLE_FACTOR) = minA;
            pDest->at<uint16_t>(y, x / DOWNSAMPLE_FACTOR + 1) = minB;
        }
    }
#else
    const uint16_t MAX_DEPTH = 0xffff;

    // Naive implementation for reference and non-x86 platforms:
    #pragma omp parallel for
    for (int y = 0; y < sizeYresized; y++)
        for (int x = 0; x < source.cols; x += DOWNSAMPLE_FACTOR)
        {
            uint16_t min_value = MAX_DEPTH;

            // Loop over 4x4 quad
            for (int i = 0; i < DOWNSAMPLE_FACTOR; i++)
                for (int j = 0; j < DOWNSAMPLE_FACTOR; j++)
                {
                    auto pixel = source.at<uint16_t>(y * DOWNSAMPLE_FACTOR + i, x + j);
                    // Only include non-zero pixels in min calculation
                    if (pixel) min_value = std::min(min_value, pixel);
                }

            // If no non-zero pixels were found, mark the output as zero
            if (min_value == MAX_DEPTH) min_value = 0;

            pDest->at<uint16_t>(y, x / DOWNSAMPLE_FACTOR) = min_value;
        }
#endif
}

