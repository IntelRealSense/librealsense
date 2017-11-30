// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
// Metadata attributes provided by RS4xx Depth Cameras

#pragma once

#include "types.h"
#include "archive.h"
#include "metadata.h"

using namespace librealsense;

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

        rs2_metadata_type get(const frame & frm) const override
        {
            auto s = reinterpret_cast<const S*>(((const uint8_t*)frm.additional_data.metadata_blob.data()) + _offset);

            if (!is_attribute_valid(s))
                throw invalid_value_exception("metadata not available");

            auto attrib = static_cast<rs2_metadata_type>((*s).*_md_attribute);
            if (_modifyer) attrib = _modifyer(attrib);
            return attrib;
        }

        // Verifies that the parameter is both supported and available
        bool supports(const frame & frm) const override
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

                if ((s->header.md_type_id != expected_type) || (s->header.md_size !=sizeof(*s)))
                {
                    std::string type = (md_type_desc.count(s->header.md_type_id) > 0) ?
                                md_type_desc.at(s->header.md_type_id) : (to_string() << "0x" << static_cast<uint32_t>(s->header.md_type_id));
                    LOG_DEBUG("Metadata mismatch - actual: " << type
                        << ", expected: " << md_type_desc.at(expected_type) << "(0x" << std::hex << (uint32_t)expected_type << std::dec << ")");
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

        rs2_metadata_type get(const frame & frm) const override
        {
            if (!supports(frm))
                throw invalid_value_exception("UVC header is not available");

            auto attrib =  static_cast<rs2_metadata_type>((*reinterpret_cast<const St*>((const uint8_t*)frm.additional_data.metadata_blob.data())).*_md_attribute);
            if (_modifyer) attrib = _modifyer(attrib);
            return attrib;
        }

        bool supports(const frame & frm) const override
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

    /**\brief provide attributes generated and stored internally by the library*/
    template<class St, class Attribute>
    class md_additional_parser : public md_attribute_parser_base
    {
    public:
        md_additional_parser(Attribute St::* attribute_name) :
            _md_attribute(attribute_name) {};

        rs2_metadata_type get(const frame & frm) const override
        {
            return static_cast<rs2_metadata_type>(frm.additional_data.*_md_attribute);
        }

        bool supports(const frame & frm) const override
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
        rs2_metadata_type get(const frame & frm) const override
        {
            return _frame_ts_parser->get(frm) - _sensor_ts_parser->get(frm);
        };

        bool supports(const frame & frm) const override
        {
            return (_sensor_ts_parser->supports(frm) && _frame_ts_parser->supports(frm));
        };
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

        rs2_metadata_type get(const frame & frm) const override
        {
            if (!supports(frm))
                throw invalid_value_exception("Metadata is not available");

            auto s = reinterpret_cast<const S*>((frm.additional_data.metadata_blob.data()) + _offset);

            auto param = static_cast<rs2_metadata_type>((*s).*_md_attribute);
            if (_modifyer)
                param = _modifyer(param);

            return param;
        }

        bool supports(const frame & frm) const override
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
