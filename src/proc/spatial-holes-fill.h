// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved
// The routies used to perform the actual holes filling when the depth is presented as disparity (floating point) data
#pragma once

namespace librealsense
{
    enum holes_filling_types : uint8_t
    {
        hf_disabled,
        hf_2_pixel_radius,
        hf_4_pixel_radius,
        hf_8_pixel_radius,
        hf_16_pixel_radius,
        hf_unlimited_radius,
        hf_fill_from_left,
        hf_farest_from_around,
        hf_nearest_from_around,
        hf_max_value
    };

    template<typename T>
    inline void intertial_holes_fill(T* image_data, size_t width, size_t height, size_t stride, uint8_t radius)
    {
        size_t cur_fill = 0;

        T* p = image_data;
        for (int j = 0; j < height; ++j)
        {
            ++p;
            cur_fill = 0;

            //Left to Right
            for (size_t i = 1; i < width; ++i)
            {
                //if (!*((int *)p)) // Evgeni - requires verification
                if (!*p && *(p - 1))
                {
                    if (++cur_fill < radius)
                        *p = *(p - 1);
                }
                else
                    cur_fill = 0;

                ++p;
            }

            --p;
            cur_fill = 0;
            //Right to left
            for (size_t i = 1; i < width; ++i)
            {
                //if (!*((int *)p)) // Evgeni - requires verification
                if (!*p)
                {
                    if (++cur_fill < radius)
                        *p = *(p + 1);
                }
                else
                    cur_fill = 0;
                --p;
            }
            p += width;
        }
    }

    template<typename T>
    inline void holes_fill_left(T* image_data, size_t width, size_t height, size_t stride)
    {
        T* p = image_data;
        for (int j = 0; j < height; ++j)
        {
            ++p;
            for (int i = 1; i < width; ++i)
            {
                //if (!*((int *)p)) // Evgeni - requires verification
                if (!*p)
                    *p = *(p - 1);
                ++p;
            }
        }
    }

    template<typename T>
    inline void holes_fill_farest(T* image_data, size_t width, size_t height, size_t stride)
    {
        T tmp = 0;
        T * p = image_data + width;
        T * q = nullptr;
        for (int j = 1; j < height - 1; ++j)
        {
            ++p;
            for (int i = 1; i < width; ++i)
            {
                if (!*((int *)p))
                {
                    tmp = *(p - width);

                    q = p - width - 1;
                    if (*q > tmp)
                        tmp = *q;

                    q = p - 1;
                    if (*q > tmp)
                        tmp = *q;

                    q = p + width - 1;
                    if (*q > tmp)
                        tmp = *q;

                    q = p + width;
                    if (*q > tmp)
                        tmp = *q;

                    *p = tmp;
                }

                p++;
            }
        }
    }

    template<typename T>
    inline void holes_fill_nearest(T* image_data, size_t width, size_t height, size_t stride)
    {
        T tmp = 0;
        T * p = image_data + width;
        T * q = nullptr;
        for (int j = 1; j < height - 1; ++j)
        {
            ++p;
            for (int i = 1; i < width; ++i)
            {
                if (!*p)
                {
                    tmp = *(p - width);

                    q = p - width - 1;
                    if (*((int *)q) && *q < tmp)
                        tmp = *q;

                    q = p - 1;
                    if (*((int *)q) && *q < tmp)
                        tmp = *q;

                    q = p + width - 1;
                    if (*((int *)q) && *q < tmp)
                        tmp = *q;

                    q = p + width;
                    if (*((int *)q) && *q < tmp)
                        tmp = *q;

                    *p = tmp;
                }

                p++;
            }
        }
    }

    template<typename T>
    void apply_holes_filling(void * image_data, size_t width, size_t height, size_t stride,
        holes_filling_types mode, uint8_t radius)
    {
        T* data= reinterpret_cast<T*>(image_data);

        // The holes fill routines except for the radius-based derived from the reference design
        switch (mode)
        {
        case hf_2_pixel_radius:
        case hf_4_pixel_radius:
        case hf_8_pixel_radius:
        case hf_16_pixel_radius:
        case hf_unlimited_radius:
            intertial_holes_fill(data, width, height, stride, radius);
            break;
        case hf_fill_from_left:
            holes_fill_left(data, width, height, stride);
            break;
        case hf_farest_from_around:
            holes_fill_farest(data, width, height, stride);
            break;
        case hf_nearest_from_around:
            holes_fill_nearest(data, width, height, stride);
            break;
        }
    }
}
