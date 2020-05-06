// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "zero-order.h"
#include <iomanip>
#include "l500/l500-depth.h"

const double METER_TO_MM = 1000;

namespace librealsense
{
    enum zero_order_invalidation_options
    {
        RS2_OPTION_FILTER_ZO_IR_THRESHOLD = static_cast<rs2_option>(RS2_OPTION_COUNT + 0), /**< IR min threshold used by zero order filter */
        RS2_OPTION_FILTER_ZO_RTD_HIGH_THRESHOLD = static_cast<rs2_option>(RS2_OPTION_COUNT + 1), /**< RTD high threshold used by zero order filter */
        RS2_OPTION_FILTER_ZO_RTD_LOW_THRESHOLD = static_cast<rs2_option>(RS2_OPTION_COUNT + 2), /**< RTD low threshold used by zero order filter */
        RS2_OPTION_FILTER_ZO_BASELINE = static_cast<rs2_option>(RS2_OPTION_COUNT + 3), /**< baseline of camera used by zero order filter */
        RS2_OPTION_FILTER_ZO_PATCH_SIZE = static_cast<rs2_option>(RS2_OPTION_COUNT + 4), /**< patch size used by zero order filter */
        RS2_OPTION_FILTER_ZO_MAX_VALUE = static_cast<rs2_option>(RS2_OPTION_COUNT + 5), /**< z max value used by zero order filter */
        RS2_OPTION_FILTER_ZO_IR_MIN_VALUE = static_cast<rs2_option>(RS2_OPTION_COUNT + 6), /**< ir min value used by zero order filter */
        RS2_OPTION_FILTER_ZO_THRESHOLD_OFFSET = static_cast<rs2_option>(RS2_OPTION_COUNT + 7), /**< threshold offset used by zero order filter */
        RS2_OPTION_FILTER_ZO_THRESHOLD_SCALE = static_cast<rs2_option>(RS2_OPTION_COUNT + 8) /**< threshold scale used by zero order filter */
    };

    double get_pixel_rtd(const rs2::vertex& v, int baseline)
    {
        auto x = (double)v.x*METER_TO_MM;
        auto y = (double)v.y*METER_TO_MM;
        auto z = (double)v.z*METER_TO_MM;
       
        auto rtd = sqrt(x*x + y*y + z*z) + sqrt((x - baseline) *(x - baseline) + y*y + z*z);
        return v.z ? rtd : 0;
    }

    void z2rtd(const rs2::vertex* vertices, double* rtd, const rs2_intrinsics& intrinsics, int baseline)
    {
        for (auto i = 0;i < intrinsics.height*intrinsics.width; i++)
        {
            rtd[i] = get_pixel_rtd(vertices[i], baseline);           
        }
    }
    
    template<typename T>
    std::vector <T> get_zo_point_values(const T* frame_data_in, const rs2_intrinsics& intrinsics, int zo_point_x, int zo_point_y, int patch_r)
    {
        std::vector<T> values;
        values.reserve((patch_r + 2ULL) *(patch_r + 2ULL));

        for (auto i = zo_point_y - 1 - patch_r; i <= (zo_point_y + patch_r) && i < intrinsics.height; i++)
        {
            for (auto j = (zo_point_x - 1 - patch_r); j <= (zo_point_x + patch_r) && i < intrinsics.width; j++)
            {
                values.push_back(frame_data_in[i*intrinsics.width + j]);
            }
        }

        return values;
    }

    template<typename T>
    T get_zo_point_value(std::vector<T>& values)
    {
        std::sort(values.begin(), values.end());

        if ((values.size()) % 2 == 0)
        {
            return (values[values.size() / 2 - 1] + values[values.size() / 2]) / 2;
        }
        else if (values.size() > 0)
            return values[values.size() / 2];

        return 0;
    }

    bool try_get_zo_rtd_ir_point_values(const double* rtd, const uint16_t* depth_data_in, const uint8_t* ir_data, 
        const rs2_intrinsics& intrinsics, const zero_order_options& options, int zo_point_x, int zo_point_y,
        double *rtd_zo_value, uint8_t* ir_zo_data)
    {
        if (zo_point_x - options.patch_size < 0 || zo_point_x + options.patch_size >= intrinsics.width ||
            zo_point_y - options.patch_size < 0 || zo_point_y + options.patch_size >= intrinsics.height)
            return false;

        auto values_rtd = get_zo_point_values(rtd, intrinsics, zo_point_x, zo_point_y, options.patch_size);
        auto values_ir = get_zo_point_values(ir_data, intrinsics, zo_point_x, zo_point_y, options.patch_size);
        auto values_z = get_zo_point_values(depth_data_in, intrinsics, zo_point_x, zo_point_y, options.patch_size);

        for (auto i = 0; i < values_rtd.size(); i++)
        {
            if ((values_z[i] / 8.0) > options.z_max || (values_ir[i] < options.ir_min))
            {
                values_rtd[i] = 0;  
                values_ir[i] = 0;
            }       
        }

        values_rtd.erase(std::remove_if(values_rtd.begin(), values_rtd.end(), [](double val)
        {
            return val == 0;
        }), values_rtd.end());

        values_ir.erase(std::remove_if(values_ir.begin(), values_ir.end(), [](uint8_t val)
        {
            return val == 0;
        }), values_ir.end());

        if (values_rtd.empty() || values_ir.empty())
            return false;

        *rtd_zo_value = get_zo_point_value(values_rtd);
        *ir_zo_data = get_zo_point_value(values_ir);

        return true;
    }

    template<class T>
    void detect_zero_order(const double * rtd, const uint16_t* depth_data_in, const uint8_t* ir_data, T zero_pixel,
       const rs2_intrinsics& intrinsics, const zero_order_options& options,
       double zo_value, uint8_t iro_value)
    {
        const double ir_dynamic_range = 256.0;

        double r = std::exp((ir_dynamic_range / 2.0 + options.threshold_offset - iro_value) / (double)options.threshold_scale);

        double res = (1.0 + r);
        double i_threshold_relative = options.ir_threshold / res;
        for (auto i = 0; i < intrinsics.height*intrinsics.width; i++)
        {
            double rtd_val = rtd[i];
            uint8_t ir_val = ir_data[i];

            bool zero = (depth_data_in[i] > 0) && 
                        (ir_val < i_threshold_relative) &&
                        (rtd_val > (zo_value - options.rtd_low_threshold)) && 
                        (rtd_val < (zo_value + options.rtd_high_threshold));

            zero_pixel(i, zero);
        }
    }

    template<class T>
    bool zero_order_invalidation(const uint16_t * depth_data_in, const uint8_t * ir_data, T zero_pixel,
        const rs2::vertex* vertices,
        rs2_intrinsics intrinsics,
        const zero_order_options& options, int zo_point_x, int zo_point_y)
    {
        std::vector<double> rtd(size_t(intrinsics.height)*intrinsics.width);
        z2rtd(vertices, rtd.data(), intrinsics, int(options.baseline));
        double rtd_zo_value; 
        uint8_t ir_zo_value;

        if (try_get_zo_rtd_ir_point_values(rtd.data(), depth_data_in, ir_data, intrinsics, 
            options,zo_point_x, zo_point_y, &rtd_zo_value, &ir_zo_value))
        {
            detect_zero_order(rtd.data(), depth_data_in, ir_data, zero_pixel, intrinsics,
                options, rtd_zo_value, ir_zo_value);
            return true;
        }
        return false;
    }

    zero_order::zero_order(std::shared_ptr<bool_option> is_enabled_opt)
       : generic_processing_block("Zero Order Fix"), _first_frame(true), _is_enabled_opt(is_enabled_opt),
        _resolutions_depth { 0 }
    {
        auto ir_threshold = std::make_shared<ptr_option<uint8_t>>(
            0,
            255,
            1,
            115,
            &_options.ir_threshold,
            "IR threshold");
        ir_threshold->on_set([ir_threshold](float val)
        {
            if (!ir_threshold->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported ir threshold " << val << " is out of range.");
        });

        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_IR_THRESHOLD), ir_threshold);

        auto rtd_high_threshold = std::make_shared<ptr_option<uint16_t>>(
            0,
            400,
            1,
            200,
            &_options.rtd_high_threshold,
            "RTD high threshold");
        rtd_high_threshold->on_set([rtd_high_threshold](float val)
        {
            if (!rtd_high_threshold->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported rtd high threshold " << val << " is out of range.");
        });

        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_RTD_HIGH_THRESHOLD), rtd_high_threshold);

        auto rtd_low_threshold = std::make_shared<ptr_option<uint16_t>>(
            0,
            400,
            1,
            200,
            &_options.rtd_low_threshold,
            "RTD high threshold");
        rtd_low_threshold->on_set([rtd_low_threshold](float val)
        {
            if (!rtd_low_threshold->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported rtd low threshold " << val << " is out of range.");
        });

        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_RTD_LOW_THRESHOLD), rtd_low_threshold);

        auto baseline = std::make_shared<ptr_option<float>>(
            -50,
            50,
            1,
            -10,
            &_options.baseline,
            "Baseline");
        baseline->on_set([baseline](float val)
        {
            if (!baseline->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_BASELINE), baseline);
    
        auto patch_size = std::make_shared<ptr_option<int>>(
            0,
            50,
            1,
            5,
            &_options.patch_size,
            "Patch size");
        patch_size->on_set([patch_size](float val)
        {
            if (!patch_size->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_PATCH_SIZE), patch_size);

        auto zo_max = std::make_shared<ptr_option<int>>(
            0,
            65535,
            1,
            1200,
            &_options.z_max,
            "ZO max value");
        zo_max->on_set([zo_max](float val)
        {
            if (!zo_max->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_MAX_VALUE), zo_max);

        auto ir_min = std::make_shared<ptr_option<int>>(
            0,
            256,
            1,
            75,
            &_options.ir_min,
            "Minimum IR value (saturation)");
        ir_min->on_set([ir_min](float val)
        {
            if (!ir_min->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_IR_MIN_VALUE), ir_min);
       
        auto offset = std::make_shared<ptr_option<int>>(
            0,
            1000,
            1,
            10,
            &_options.threshold_offset,
            "Threshold offset");
        offset->on_set([offset](float val)
        {
            if (!offset->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_THRESHOLD_OFFSET), offset);
        
        auto scale = std::make_shared<ptr_option<int>>(
            0,
            2000,
            1,
            20,
            &_options.threshold_scale,
            "Threshold scale");
        scale->on_set([scale](float val)
        {
            if (!scale->is_valid(val))
                throw invalid_value_exception(to_string()
                    << "Unsupported patch size value " << val << " is out of range.");
        });
        register_option(static_cast<rs2_option>(RS2_OPTION_FILTER_ZO_THRESHOLD_SCALE), scale);
    }

    const char* zero_order::get_option_name(rs2_option option) const
    {
        switch (static_cast<zero_order_invalidation_options>(option))
        {
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_IR_THRESHOLD:
            return "IR Threshold";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_RTD_HIGH_THRESHOLD:
            return "RTD high Threshold";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_RTD_LOW_THRESHOLD:
            return "RTD Low Threshold";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_BASELINE:
            return "Baseline";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_PATCH_SIZE:
            return "Patch size";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_MAX_VALUE:
            return "ZO max value";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_IR_MIN_VALUE:
            return "IR min value";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_THRESHOLD_OFFSET:
            return "Threshold offset";
        case zero_order_invalidation_options::RS2_OPTION_FILTER_ZO_THRESHOLD_SCALE:
            return "Threshold scale";
        }

        return options_container::get_option_name(option);
    }

    bool zero_order::try_read_baseline(const rs2::frame& frame)
    {
        if (auto sensor = ((frame_interface*)frame.get())->get_sensor())
        {
            if(auto l5 = dynamic_cast<l500_depth_sensor_interface*>(sensor.get()))
            {
                _options.baseline = l5->read_baseline();
                return true;
            }
            // For playback
            else
            {
                auto extendable = As<librealsense::extendable_interface>(sensor);
                if (extendable && extendable->extend_to(TypeToExtension<librealsense::l500_depth_sensor_interface>::value, (void**)(&l5)))
                {
                    return l5->read_baseline();
                }
            }
        }
        return false;
    }

    ivcam2::intrinsic_params zero_order::try_read_intrinsics(const rs2::frame & frame)
    {
        if (auto sensor = ((frame_interface*)frame.get())->get_sensor())
        {
            auto profile = frame.get_profile().as<rs2::video_stream_profile>();

            if (auto l5 = dynamic_cast<l500_depth_sensor_interface*>(sensor.get()))
            {
                return l500_depth_sensor::get_intrinsic_params(profile.width(), profile.height(), l5->get_intrinsic());
            }
            // For playback
            else
            {
                auto extendable = As<librealsense::extendable_interface>(sensor);
                if (extendable && extendable->extend_to(TypeToExtension<librealsense::l500_depth_sensor_interface>::value, (void**)(&l5)))
                {
                    return l500_depth_sensor::get_intrinsic_params(profile.width(), profile.height(), l5->get_intrinsic());
                }
            }
        }
        throw std::runtime_error("didn't succeed to get intrinsics");
    }

    std::pair<int, int> zero_order::get_zo_point(const rs2::frame& frame)
    {
        auto intrinsics = try_read_intrinsics(frame);
        return { intrinsics.zo.x, intrinsics.zo.y };
    }

    rs2::frame zero_order::process_frame(const rs2::frame_source& source, const rs2::frame& f)
    {
        // If is_enabled_opt is false, meaning this processing block is not active,
        // return the frame as is.
        if (auto is_enabled = _is_enabled_opt.lock())
            if (!is_enabled->is_true())
                // zero order is disabled, passthrough the frame
                return f;

        std::vector<rs2::frame> result;

        if (_first_frame)
        {
            auto zo = get_zo_point(f);
            LOG_DEBUG("ZO values: "<<zo.first<<", "<<zo.second);

            if (!try_read_baseline(f))
                LOG_WARNING("Couldn't read the baseline value");

            _first_frame = false;
        }
     
        auto data = f.as<rs2::frameset>();
        if (!data)
        {
            LOG_ERROR("Frame received is not a frameset.");
            return f;
        }

        if (_source_profile_depth.get() != data.get_depth_frame().get_profile().get())
        {
            _source_profile_depth = data.get_depth_frame().get_profile();
            _target_profile_depth = _source_profile_depth.clone(_source_profile_depth.stream_type(), _source_profile_depth.stream_index(), _source_profile_depth.format());
        }

        auto depth_frame = data.get_depth_frame();
        auto ir_frame = data.get_infrared_frame();
        auto confidence_frame = data.first_or_default(RS2_STREAM_CONFIDENCE);

        auto points = _pc.calculate(depth_frame);

        auto depth_out = source.allocate_video_frame(_target_profile_depth, depth_frame, 0, 0, 0, 0, RS2_EXTENSION_DEPTH_FRAME);

        rs2::frame confidence_out;
        if (confidence_frame)
        {
            if (_source_profile_confidence.get() != confidence_frame.get_profile().get())
            {
                _source_profile_confidence = confidence_frame.get_profile();
                _target_profile_confidence = _source_profile_confidence.clone(_source_profile_confidence.stream_type(), _source_profile_confidence.stream_index(), _source_profile_confidence.format());

            }
            confidence_out = source.allocate_video_frame(_source_profile_confidence, confidence_frame, 0, 0, 0, 0, RS2_EXTENSION_VIDEO_FRAME);
        }
        auto depth_intrinsics = depth_frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics();

        auto depth_output = (uint16_t*)depth_out.get_data();
        uint8_t* confidence_output;

        if (confidence_frame)
        {
            confidence_output = (uint8_t*)confidence_out.get_data();
        }

        auto zo = get_zo_point(depth_frame);

        if (zero_order_invalidation((const uint16_t*)depth_frame.get_data(),
            (const uint8_t*)ir_frame.get_data(),
            [&](int index, bool zero) 
        {
            depth_output[index] = zero ? 0 : ((uint16_t*)depth_frame.get_data())[index];

            if (confidence_frame)
            {
                confidence_output[index] = zero ? 0 : ((uint8_t*)confidence_frame.get_data())[index];
            }
        },
            points.get_vertices(),
            depth_intrinsics,
            _options, zo.first, zo.second))
        {
            result.push_back(depth_out);
            if (confidence_frame)
                result.push_back(confidence_out);
        }
        else
        {
            result.push_back(depth_frame);
            if (confidence_frame)
                result.push_back(confidence_frame);
        }
        return source.allocate_composite_frame(result);
    }

    bool zero_order::should_process(const rs2::frame& frame)
    {
        // Zero order might get frames to process even if it is disabled by option.
        // In such case, it should passthrough all of the frames it receives, except for IR frames,
        // which are handled by processing blocks and must me droped.

        if (auto set = frame.as<rs2::frameset>())
        {
            if (!set.get_depth_frame() || !set.get_infrared_frame())
            {
                return false;
            }
            auto depth_frame = set.get_depth_frame();

            auto zo = get_zo_point(depth_frame);

            if (zo.first - _options.patch_size < 0 || zo.first + _options.patch_size >= depth_frame.get_width() ||
               (zo.second - _options.patch_size < 0 || zo.second + _options.patch_size >= depth_frame.get_height()))
            {
                return false;
            }
            return true;

        }
        else if (frame.get_profile().stream_type() == RS2_STREAM_INFRARED)
            // a single IR frame received, drop it.
            return false;

        return true;
    }

    rs2::frame zero_order::prepare_output(const rs2::frame_source & source, rs2::frame input, std::vector<rs2::frame> results)
    {
        if (auto composite = input.as<rs2::frameset>())
        {
            composite.foreach_rs([&](rs2::frame f)
            {
                if (f.get_profile().stream_type() != RS2_STREAM_DEPTH && f.get_profile().stream_type() != RS2_STREAM_INFRARED && f.get_profile().stream_type() != RS2_STREAM_CONFIDENCE && 
                    results.size() > 0)
                    results.push_back(f);
            });
        }

        return source.allocate_composite_frame(results);
    }
}
