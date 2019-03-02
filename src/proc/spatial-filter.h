// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved
//// In-place Domain Transform Edge-preserving filter based on
// http://inf.ufrgs.br/~eslgastal/DomainTransform/Gastal_Oliveira_SIGGRAPH2011_Domain_Transform.pdf
// The filter also allows to apply holes filling extention that due to implementation constrains can be applied horizontally only

#pragma once

#include <map>
#include <vector>
#include <cmath>

#include "../include/librealsense2/hpp/rs_frame.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"

namespace librealsense
{
    class spatial_filter : public depth_processing_block
    {
    public:
        spatial_filter();

    protected:
        void    update_configuration(const rs2::frame& f);

        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        template <typename T>
        void dxf_smooth(void *frame_data, float alpha, float delta, int iterations)
        {
            static_assert((std::is_arithmetic<T>::value), "Spatial filter assumes numeric types");
            bool fp = (std::is_floating_point<T>::value);

            for (int i = 0; i < iterations; i++)
            {
                if (fp)
                {
                    recursive_filter_horizontal_fp(frame_data, alpha, delta);
                    recursive_filter_vertical_fp(frame_data, alpha, delta);
                }
                else
                {
                    recursive_filter_horizontal<T>(frame_data, alpha, delta);
                    recursive_filter_vertical<T>(frame_data, alpha, delta);
                }
            }

            // Disparity domain hole filling requires a second pass over the frame data
            // For depth domain a more efficient in-place hole filling is performed
            if (_holes_filling_mode && fp)
                intertial_holes_fill<T>(static_cast<T*>(frame_data));
        }

        void recursive_filter_horizontal_fp(void * image_data, float alpha, float deltaZ);
        void recursive_filter_vertical_fp(void * image_data, float alpha, float deltaZ);

        template <typename T>
        void  recursive_filter_horizontal(void * image_data, float alpha, float deltaZ)
        {
            size_t v{}, u{};

            // Handle conversions for invalid input data
            bool fp = (std::is_floating_point<T>::value);

            // Filtering integer values requires round-up to the nearest discrete value
            const float round = fp ? 0.f : 0.5f;
            // define invalid inputs
            const T valid_threshold = fp ? static_cast<T>(std::numeric_limits<T>::epsilon()) : static_cast<T>(1);
            const T delta_z = static_cast<T>(deltaZ);

            auto image = reinterpret_cast<T*>(image_data);
            size_t cur_fill = 0;

            for (v = 0; v < _height; v++)
            {
                // left to right
                T *im = image + v * _width;
                T val0 = im[0];
                cur_fill = 0;

                for (u = 1; u < _width - 1; u++)
                {
                    T val1 = im[1];

                    if (fabs(val0) >= valid_threshold)
                    {
                        if (fabs(val1) >= valid_threshold)
                        {
                            cur_fill = 0;
                            T diff = static_cast<T>(fabs(val1 - val0));

                            if (diff >= valid_threshold && diff <= delta_z)
                            {
                                float filtered = val1 * alpha + val0 * (1.0f - alpha);
                                val1 = static_cast<T>(filtered + round);
                                im[1] = val1;
                            }
                        }
                        else // Only the old value is valid - appy holes filling
                        {
                            if (_holes_filling_radius)
                            {
                                if (++cur_fill <_holes_filling_radius)
                                    im[1] = val1 = val0;
                            }
                        }
                    }

                    val0 = val1;
                    im += 1;
                }

                // right to left
                im = image + (v + 1) * _width - 2;  // end of row - two pixels
                T val1 = im[1];
                cur_fill = 0;

                for (u = _width - 1; u > 0; u--)
                {
                    T val0 = im[0];

                    if (val1 >= valid_threshold)
                    {
                        if (val0 > valid_threshold)
                        {
                            cur_fill = 0;
                            T diff = static_cast<T>(fabs(val1 - val0));

                            if (diff <= delta_z)
                            {
                                float filtered = val0 * alpha + val1 * (1.0f - alpha);
                                val0 = static_cast<T>(filtered + round);
                                im[0] = val0;
                            }
                        }
                        else // 'inertial' hole filling
                        {
                            if (_holes_filling_radius)
                            {
                                if (++cur_fill <_holes_filling_radius)
                                    im[0] = val0 = val1;
                            }
                        }
                    }

                    val1 = val0;
                    im -= 1;
                }
            }
        }

        template <typename T>
        void recursive_filter_vertical(void * image_data, float alpha, float deltaZ)
        {
            size_t v{}, u{};

            // Handle conversions for invalid input data
            bool fp = (std::is_floating_point<T>::value);

            // Filtering integer values requires round-up to the nearest discrete value
            const float round = fp ? 0.f : 0.5f;
            // define invalid range
            const T valid_threshold = fp ? static_cast<T>(std::numeric_limits<T>::epsilon()) : static_cast<T>(1);
            const T delta_z = static_cast<T>(deltaZ);

            auto image = reinterpret_cast<T*>(image_data);

            // we'll do one row at a time, top to bottom, then bottom to top

            // top to bottom

            T *im = image;
            T im0{};
            T imw{};
            for (v = 1; v < _height; v++)
            {
                for (u = 0; u < _width; u++)
                {
                    im0 = im[0];
                    imw = im[_width];

                    //if ((fabs(im0) >= valid_threshold) && (fabs(imw) >= valid_threshold))
                    {
                        T diff = static_cast<T>(fabs(im0 - imw));
                        if (diff < delta_z)
                        {
                            float filtered = imw * alpha + im0 * (1.f - alpha);
                            im[_width] = static_cast<T>(filtered + round);
                        }
                    }
                    im += 1;
                }
            }

            // bottom to top
            im = image + (_height - 2) * _width;
            for (v = 1; v < _height; v++, im -= (_width * 2))
            {
                for (u = 0; u < _width; u++)
                {
                    im0 = im[0];
                    imw = im[_width];

                    if ((fabs(im0) >= valid_threshold) && (fabs(imw) >= valid_threshold))
                    {
                        T diff = static_cast<T>(fabs(im0 - imw));
                        if (diff < delta_z)
                        {
                            float filtered = im0 * alpha + imw * (1.f - alpha);
                            im[0] = static_cast<T>(filtered + round);
                        }
                    }
                    im += 1;
                }
            }
        }

        template<typename T>
        inline void intertial_holes_fill(T* image_data)
        {
            std::function<bool(T*)> fp_oper = [](T* ptr) { return !*((int *)ptr); };
            std::function<bool(T*)> uint_oper = [](T* ptr) { return !(*ptr); };
            auto empty = (std::is_floating_point<T>::value) ? fp_oper : uint_oper;

            size_t cur_fill = 0;

            T* p = image_data;
            for (int j = 0; j < _height; ++j)
            {
                ++p;
                cur_fill = 0;

                //Left to Right
                for (size_t i = 1; i < _width; ++i)
                {
                    if (empty(p))
                    {
                        if (++cur_fill < _holes_filling_radius)
                            *p = *(p - 1);
                    }
                    else
                        cur_fill = 0;

                    ++p;
                }

                --p;
                cur_fill = 0;
                //Right to left
                for (size_t i = 1; i < _width; ++i)
                {
                    if (empty(p))
                    {
                        if (++cur_fill < _holes_filling_radius)
                            *p = *(p + 1);
                    }
                    else
                        cur_fill = 0;
                    --p;
                }
                p += _width;
            }
        }

    private:

        float                   _spatial_alpha_param;
        uint8_t                 _spatial_delta_param;
        uint8_t                 _spatial_iterations;
        float                   _spatial_edge_threshold;
        size_t                  _width, _height, _stride;
        size_t                  _bpp;
        rs2_extension           _extension_type;            // Strictly Depth/Disparity
        size_t                  _current_frm_size_pixels;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        bool                    _stereoscopic_depth;
        float                   _focal_lenght_mm;
        float                   _stereo_baseline_mm;
        uint8_t                 _holes_filling_mode;
        uint8_t                 _holes_filling_radius;
    };
    MAP_EXTENSION(RS2_EXTENSION_SPATIAL_FILTER, librealsense::spatial_filter);
}
