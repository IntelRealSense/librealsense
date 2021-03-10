// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved
// Enhancing the input video frame by filling missing data.
#pragma once

namespace librealsense
{
    enum holes_filling_types : uint8_t
    {
        hf_fill_from_left,
        hf_farest_from_around,
        hf_nearest_from_around,
        hf_max_value
    };

    class hole_filling_filter : public depth_processing_block
    {
    public:
        hole_filling_filter();

    protected:
        void update_configuration(const rs2::frame& f);
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

        rs2::frame prepare_target_frame(const rs2::frame& f, const rs2::frame_source& source);

        template<typename T>
        void apply_hole_filling(void * image_data)
        {
            bool fp = (std::is_floating_point<T>::value);
            T* data = reinterpret_cast<T*>(image_data);

            // Select and apply the appropriate hole filling method
            switch (_hole_filling_mode)
            {
            case hf_fill_from_left:
                holes_fill_left(data, _width, _height, _stride);
                break;
            case hf_farest_from_around:
                holes_fill_farest(data, _width, _height, _stride);
                break;
            case hf_nearest_from_around:
                holes_fill_nearest(data, _width, _height, _stride);
                break;
            default:
                throw invalid_value_exception(to_string()
                    << "Unsupported hole filling mode: " << _hole_filling_mode << " is out of range.");
            }
        }

        // Implementations of the hole-filling methods
        template<typename T>
        inline void holes_fill_left(T* image_data, size_t width, size_t height, size_t stride)
        {
            std::function<bool(T*)> fp_oper = [](T* ptr) { return !*((int *)ptr); };
            std::function<bool(T*)> uint_oper = [](T* ptr) { return !(*ptr); };
            auto empty = (std::is_floating_point<T>::value) ? fp_oper : uint_oper;

            T* p = image_data;

            for (int j = 0; j < height; ++j)
            {
                ++p;
                for (int i = 1; i < width; ++i)
                {
                    if (empty(p))
                        *p = *(p - 1);
                    ++p;
                }
            }
        }

        template<typename T>
        inline void holes_fill_farest(T* image_data, size_t width, size_t height, size_t stride)
        {
            std::function<bool(T*)> fp_oper = [](T* ptr) { return !*((int *)ptr); };
            std::function<bool(T*)> uint_oper = [](T* ptr) { return !(*ptr); };
            auto empty = (std::is_floating_point<T>::value) ? fp_oper : uint_oper;

            T tmp = 0;
            T * p = image_data + width;
            T * q = nullptr;
            for (int j = 1; j < height - 1; ++j)
            {
                ++p;
                for (int i = 1; i < width; ++i)
                {
                    if (empty(p))
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
            std::function<bool(T*)> fp_oper = [](T* ptr) { return !*((int *)ptr); };
            std::function<bool(T*)> uint_oper = [](T* ptr) { return !(*ptr); };
            auto empty = (std::is_floating_point<T>::value) ? fp_oper : uint_oper;

            T tmp = 0;
            T * p = image_data + width;
            T * q = nullptr;
            for (int j = 1; j < height - 1; ++j)
            {
                ++p;
                for (int i = 1; i < width; ++i)
                {
                    if (empty(p))
                    {
                        tmp = *(p - width);

                        q = p - width - 1;
                        if (!empty(q) && (*q < tmp))
                            tmp = *q;

                        q = p - 1;
                        if (!empty(q) && (*q < tmp))
                            tmp = *q;

                        q = p + width - 1;
                        if (!empty(q) && (*q < tmp))
                            tmp = *q;

                        q = p + width;
                        if (!empty(q) && (*q < tmp))
                            tmp = *q;

                        *p = tmp;
                    }

                    p++;
                }
            }
        }

    private:

        size_t                  _width, _height, _stride;
        size_t                  _bpp;
        rs2_extension           _extension_type;            // Strictly Depth/Disparity
        size_t                  _current_frm_size_pixels;
        rs2::stream_profile     _source_stream_profile;
        rs2::stream_profile     _target_stream_profile;
        uint8_t                 _hole_filling_mode;
    };
    MAP_EXTENSION(RS2_EXTENSION_HOLE_FILLING_FILTER, librealsense::hole_filling_filter);
}
