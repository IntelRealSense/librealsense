// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
// Metadata attributes provided by RS4xx Depth Cameras

#pragma once

#include "types.h"
#include "archive.h"
#include "metadata.h"
#include <cmath>

namespace librealsense
{
/** \brief Metadata fields that are utilized internally by librealsense
    Provides extention to the r2_frame_metadata list of attributes*/
    enum frame_metadata_internal
    {
        RS2_FRAME_METADATA_HW_TYPE  =   RS2_FRAME_METADATA_COUNT +1 , /**< 8-bit Module type: RS4xx, IVCAM*/
        RS2_FRAME_METADATA_SKU_ID                                   , /**< 8-bit SKU Id*/
        RS2_FRAME_METADATA_FORMAT                                   , /**< 16-bit Frame format*/
        RS2_FRAME_METADATA_WIDTH                                    , /**< 16-bit Frame width. pixels*/
        RS2_FRAME_METADATA_HEIGHT                                   , /**< 16-bit Frame height. pixels*/
        RS2_FRAME_METADATA_COUNT
    };

    /**\brief Base class that establishes the interface for retrieving metadata attributes*/
    class md_attribute_parser_base
    {
    public:
        virtual rs2_metadata_type get(const frame& frm) const = 0;
        virtual bool supports(const frame& frm) const = 0;

        virtual ~md_attribute_parser_base() = default;
    };

    /**\brief metadata parser class - support metadata in format: rs2_frame_metadata_value, rs2_metadata_type */
    class md_constant_parser : public md_attribute_parser_base
    {
    public:
        md_constant_parser(rs2_frame_metadata_value type) : _type(type) {}
        rs2_metadata_type get(const frame& frm) const override
        {
            rs2_metadata_type v;
            if (try_get(frm, v) == false)
            {
                throw invalid_value_exception("Frame does not support this type of metadata");
            }
            return v;
        }
        bool supports(const frame& frm) const override
        {
            rs2_metadata_type v;
            return try_get(frm, v);
        }

        static std::shared_ptr<metadata_parser_map> create_metadata_parser_map()
        {
            auto md_parser_map = std::make_shared<metadata_parser_map>();
            for (int i = 0; i < static_cast<int>(rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT); ++i)
            {
                auto frame_md_type = static_cast<rs2_frame_metadata_value>(i);
                md_parser_map->insert(std::make_pair(frame_md_type, std::make_shared<md_constant_parser>(frame_md_type)));
            }
            return md_parser_map;
        }
    private:
        bool try_get(const frame& frm, rs2_metadata_type& result) const
        {
            auto pair_size = (sizeof(rs2_frame_metadata_value) + sizeof(rs2_metadata_type));
            const uint8_t* pos = frm.additional_data.metadata_blob.data();
            while (pos <= frm.additional_data.metadata_blob.data() + frm.additional_data.metadata_blob.size())
            {
                const rs2_frame_metadata_value* type = reinterpret_cast<const rs2_frame_metadata_value*>(pos);
                pos += sizeof(rs2_frame_metadata_value);
                if (_type == *type)
                {
                    const rs2_metadata_type* value = reinterpret_cast<const rs2_metadata_type*>(pos);
                    memcpy((void*)&result, (const void*)value, sizeof(*value));
                    return true;
                }
                pos += sizeof(rs2_metadata_type);
            }
            return false;
        }
        rs2_frame_metadata_value _type;
    };


    /**\brief Post-processing adjustment of the metadata attribute
     *  e.g change auto_exposure enum to boolean, change units from nano->ms,etc'*/
    typedef std::function<rs2_metadata_type(const rs2_metadata_type& param)> attrib_modifyer;

    class md_time_of_arrival_parser : public md_attribute_parser_base
    {
    public:
        rs2_metadata_type get(const frame& frm) const override
        {
            return (rs2_metadata_type)frm.get_frame_system_time();
        }

        bool supports(const frame& frm) const override
        {
            return true;
        }
    };

    /**\brief The metadata parser class directly access the metadata attribute in the blob received from HW.
    *   Given the metadata-nested construct, and the c++ lack of pointers
    *   to the inner struct, we pre-calculate and store the attribute offset internally
    *   http://stackoverflow.com/questions/1929887/is-pointer-to-inner-struct-member-forbidden*/
    template<class S, class Attribute, typename Flag>
    class md_attribute_parser : public md_attribute_parser_base
    {
    public:
        md_attribute_parser(Attribute S::* attribute_name, Flag flag, unsigned long long offset, attrib_modifyer mod) :
            _md_attribute(attribute_name), _md_flag(flag), _offset(offset), _modifyer(mod) {};

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            auto s = reinterpret_cast<const S*>(((const uint8_t*)frm.additional_data.metadata_blob.data()) + _offset);

            if (!is_attribute_valid(s))
                throw invalid_value_exception("metadata not available");

            auto attrib = static_cast<rs2_metadata_type>((*s).*_md_attribute);
            if (_modifyer) attrib = _modifyer(attrib);
            return attrib;
        }

        // Verifies that the parameter is both supported and available
        bool supports(const librealsense::frame & frm) const override
        {
            auto s = reinterpret_cast<const S*>(((const uint8_t*)frm.additional_data.metadata_blob.data()) + _offset);

            return is_attribute_valid(s);
        }

    protected:

            bool is_attribute_valid(const S* s) const
            {
                // verify that the struct is of the correct type
                // Check that the header id and the struct size corresponds.
                // Note that this heurisic is not deterministic and may validate false frames! TODO - requires review
                md_type expected_type = md_type_trait<S>::type;

                if ((s->header.md_type_id != expected_type) || (s->header.md_size < sizeof(*s)))
                {
                    std::string type = (md_type_desc.count(s->header.md_type_id) > 0) ?
                                md_type_desc.at(s->header.md_type_id) : (to_string()
                                << "0x" << std::hex << static_cast<uint32_t>(s->header.md_type_id) << std::dec);
                    LOG_DEBUG("Metadata mismatch - actual: " << type
                        << ", expected: 0x"  << std::hex << (uint32_t)expected_type << std::dec << " (" << md_type_desc.at(expected_type) << ")");
                    return false;
                }

                 // Check if the attribute's flag is set
                 auto attribute_enabled =  (0 !=(s->flags & static_cast<uint32_t>(_md_flag)));
                 if (!attribute_enabled)
                    LOG_DEBUG("Metadata attribute No: "<< (*s.*_md_attribute) << "is not active");

                 return attribute_enabled;
            }

    private:
        md_attribute_parser() = delete;
        md_attribute_parser(const md_attribute_parser&) = delete;

        Attribute S::*      _md_attribute;  // Pointer to the attribute within struct that holds the relevant data
        Flag                _md_flag;       // Bit that indicates whether the particular attribute is active
        unsigned long long  _offset;        // Inner struct offset with regard to the most outer one
        attrib_modifyer     _modifyer;      // Post-processing on received attribute
    };

    /**\brief A helper function to create a specialized attribute parser.
     *  Return it as a pointer to a base-class*/
    template<class S, class Attribute, typename Flag>
    std::shared_ptr<md_attribute_parser_base> make_attribute_parser(Attribute S::* attribute, Flag flag, unsigned long long offset, attrib_modifyer mod = nullptr)
    {
        std::shared_ptr<md_attribute_parser<S, Attribute, Flag>> parser(new md_attribute_parser<S, Attribute, Flag>(attribute, flag, offset, mod));
        return parser;
    }

    /**\brief A UVC-Header parser class*/
    template<class St, class Attribute>
    class md_uvc_header_parser : public md_attribute_parser_base
    {
    public:
        md_uvc_header_parser(Attribute St::* attribute_name, attrib_modifyer mod) :
            _md_attribute(attribute_name), _modifyer(mod){};

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            if (!supports(frm))
                throw invalid_value_exception("UVC header is not available");

            auto attrib =  static_cast<rs2_metadata_type>((*reinterpret_cast<const St*>((const uint8_t*)frm.additional_data.metadata_blob.data())).*_md_attribute);
            if (_modifyer) attrib = _modifyer(attrib);
            return attrib;
        }

        bool supports(const librealsense::frame & frm) const override
        { return (frm.additional_data.metadata_size >= platform::uvc_header_size); }

    private:
        md_uvc_header_parser() = delete;
        md_uvc_header_parser(const md_uvc_header_parser&) = delete;

        Attribute St::*     _md_attribute;  // Pointer to the attribute within uvc header that provides the relevant data
        attrib_modifyer     _modifyer;      // Post-processing on received attribute
    };

    /**\brief A utility function to create UVC metadata header parser*/
    template<class St, class Attribute>
    std::shared_ptr<md_attribute_parser_base> make_uvc_header_parser(Attribute St::* attribute, attrib_modifyer mod = nullptr)
    {
        std::shared_ptr<md_uvc_header_parser<St, Attribute>> parser(new md_uvc_header_parser<St, Attribute>(attribute, mod));
        return parser;
    }

    /**\brief A HID-Header metadata parser class*/
    template<class St, class Attribute>
    class md_hid_header_parser : public md_attribute_parser_base
    {
    public:
        md_hid_header_parser(Attribute St::* attribute_name, attrib_modifyer mod) :
            _md_attribute(attribute_name), _modifyer(mod) {};

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            if (!supports(frm))
                throw invalid_value_exception("HID header is not available");

            auto attrib = static_cast<rs2_metadata_type>((*reinterpret_cast<const St*>((const uint8_t*)frm.additional_data.metadata_blob.data())).*_md_attribute);
            attrib &= 0x00000000ffffffff;
            if (_modifyer) attrib = _modifyer(attrib);
            return attrib;
        }

        bool supports(const librealsense::frame & frm) const override
        {
            return (frm.additional_data.metadata_size >= platform::hid_header_size);
        }

    private:
        md_hid_header_parser() = delete;
        md_hid_header_parser(const md_hid_header_parser&) = delete;

        Attribute St::*     _md_attribute;  // Pointer to the attribute within uvc header that provides the relevant data
        attrib_modifyer     _modifyer;      // Post-processing on received attribute
    };

    /**\brief A utility function to create HID metadata header parser*/
    template<class St, class Attribute>
    std::shared_ptr<md_attribute_parser_base> make_hid_header_parser(Attribute St::* attribute, attrib_modifyer mod = nullptr)
    {
        std::shared_ptr<md_hid_header_parser<St, Attribute>> parser(new md_hid_header_parser<St, Attribute>(attribute, mod));
        return parser;
    }

    /**\brief provide attributes generated and stored internally by the library*/
    template<class St, class Attribute>
    class md_additional_parser : public md_attribute_parser_base
    {
    public:
        md_additional_parser(Attribute St::* attribute_name) :
            _md_attribute(attribute_name) {};

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            return static_cast<rs2_metadata_type>(frm.additional_data.*_md_attribute);
        }

        bool supports(const librealsense::frame & frm) const override
        { return true; }

    private:
        md_additional_parser() = delete;
        md_additional_parser(const md_additional_parser&) = delete;

        Attribute St::*          _md_attribute;      // Pointer to the attribute within uvc header that provides the relevant data
    };

    /**\brief A utility function to create additional_data parser*/
    template<class St, class Attribute>
    std::shared_ptr<md_attribute_parser_base> make_additional_data_parser(Attribute St::* attribute)
    {
        std::shared_ptr<md_additional_parser<St, Attribute>> parser(new md_additional_parser<St, Attribute>(attribute));
        return parser;
    }

    /**\brief Optical timestamp for RS4xx devices is calculated internally*/
    class md_rs400_sensor_timestamp : public md_attribute_parser_base
    {
        std::shared_ptr<md_attribute_parser_base> _sensor_ts_parser = nullptr;
        std::shared_ptr<md_attribute_parser_base> _frame_ts_parser = nullptr;

    public:
        explicit md_rs400_sensor_timestamp(std::shared_ptr<md_attribute_parser_base> sensor_ts_parser,
            std::shared_ptr<md_attribute_parser_base> frame_ts_parser) :
            _sensor_ts_parser(sensor_ts_parser), _frame_ts_parser(frame_ts_parser) {};

        virtual ~md_rs400_sensor_timestamp() { _sensor_ts_parser = nullptr; _frame_ts_parser = nullptr; };

        // The sensor's timestamp is defined as the middle of exposure time. Sensor_ts= Frame_ts - (Actual_Exposure/2)
        // For RS4xx the metadata payload holds only the (Actual_Exposure/2) offset, and the actual value needs to be calculated
        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            return _frame_ts_parser->get(frm) - _sensor_ts_parser->get(frm);
        };

        bool supports(const librealsense::frame & frm) const override
        {
            return (_sensor_ts_parser->supports(frm) && _frame_ts_parser->supports(frm));
        };
    };


    class actual_fps_calculator
    {
    public:
        double get_fps(const librealsense::frame & frm)
        {
            // A computation involving unsigned operands can never overflow (ISO/IEC 9899:1999 (E) \A76.2.5/9)
            // In case of frame counter reset fallback use fps from the stream configuration
            auto num_of_frames = (frm.additional_data.frame_number) ? frm.additional_data.frame_number - frm.additional_data.last_frame_number : 0;

            if (num_of_frames == 0)
            {
                LOG_INFO("Frame counter reset");
            }

            auto diff = num_of_frames ? (double)(frm.additional_data.timestamp - frm.additional_data.last_timestamp) / (double)num_of_frames : 0;
            return diff > 0 ? std::max(1000.f / std::ceil(diff), (double)1) : frm.get_stream()->get_framerate();
        }
    };


    class ds5_md_attribute_actual_fps : public md_attribute_parser_base
    {
    public:
        ds5_md_attribute_actual_fps(bool discrete = true, attrib_modifyer  exposure_mod = [](const rs2_metadata_type& param) {return param; })
            :_exposure_modifyer(exposure_mod), _discrete(discrete), _fps_values{ 6, 15, 30, 60, 90 }
        {}

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            if (frm.supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE))
            {
                if (frm.get_stream()->get_format() == RS2_FORMAT_Y16 &&
                    frm.get_stream()->get_stream_type() == RS2_STREAM_INFRARED) //calibration mode
                {
                    if (std::find(_fps_values.begin(), _fps_values.end(), 25) == _fps_values.end())
                    {
                        _fps_values.push_back(25);
                        std::sort(_fps_values.begin(), _fps_values.end());
                    }

                }

                auto exp = frm.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

                auto exp_in_micro = _exposure_modifyer(exp);
                if (exp_in_micro > 0)
                {
                    auto fps = 1000000.f / exp_in_micro;

                    if (_discrete)
                    {
                        if (fps >= _fps_values.back())
                        {
                            fps = static_cast<float>(_fps_values.back());
                        }
                        else
                        {
                            for (auto i = 0; i < _fps_values.size() - 1; i++)
                            {
                                if (fps < _fps_values[i + 1])
                                {
                                    fps = static_cast<float>(_fps_values[i]);
                                    break;
                                }
                            }
                        }
                    }
                    return std::min((int)fps, (int)frm.get_stream()->get_framerate());
                }
            }

            return (rs2_metadata_type)_fps_calculator.get_fps(frm);

        }

        bool supports(const librealsense::frame & frm) const override
        {
            return true;
        }

    private:
        mutable actual_fps_calculator _fps_calculator;
        mutable std::vector<uint32_t> _fps_values;
        attrib_modifyer _exposure_modifyer;
        bool _discrete;
    };

    /**\brief A helper function to create a specialized parser for RS4xx sensor timestamp*/
    inline std::shared_ptr<md_attribute_parser_base> make_rs400_sensor_ts_parser(std::shared_ptr<md_attribute_parser_base> frame_ts_parser,
        std::shared_ptr<md_attribute_parser_base> sensor_ts_parser)
    {
        std::shared_ptr<md_rs400_sensor_timestamp> parser(new md_rs400_sensor_timestamp(sensor_ts_parser, frame_ts_parser));
        return parser;
    }

    /**\brief The SR300 metadata parser class*/
    template<class S, class Attribute>
    class md_sr300_attribute_parser : public md_attribute_parser_base
    {
    public:
        md_sr300_attribute_parser(Attribute S::* attribute_name, unsigned long long offset, attrib_modifyer mod) :
            _md_attribute(attribute_name), _offset(offset), _modifyer(mod){};

        rs2_metadata_type get(const librealsense::frame & frm) const override
        {
            if (!supports(frm))
                throw invalid_value_exception("Metadata is not available");

            auto s = reinterpret_cast<const S*>((frm.additional_data.metadata_blob.data()) + _offset);

            auto param = static_cast<rs2_metadata_type>((*s).*_md_attribute);
            if (_modifyer)
                param = _modifyer(param);

            return param;
        }

        bool supports(const librealsense::frame & frm) const override
        {
            return (frm.additional_data.metadata_size >= (sizeof(S) + platform::uvc_header_size));
        }

    private:
        md_sr300_attribute_parser() = delete;
        md_sr300_attribute_parser(const md_sr300_attribute_parser&) = delete;

        Attribute S::*      _md_attribute;  // Pointer to the actual data field
        unsigned long long  _offset;        // Inner struct offset with regard to the most outer one
        attrib_modifyer     _modifyer;      // Post-processing on received attribute
    };

    /**\brief A helper function to create a specialized attribute parser.
    *  Return it as a pointer to a base-class*/
    template<class S, class Attribute>
    std::shared_ptr<md_attribute_parser_base> make_sr300_attribute_parser(Attribute S::* attribute, unsigned long long offset, attrib_modifyer  mod = nullptr)
    {
        std::shared_ptr<md_sr300_attribute_parser<S, Attribute>> parser(new md_sr300_attribute_parser<S, Attribute>(attribute, offset, mod));
        return parser;
    }
}
