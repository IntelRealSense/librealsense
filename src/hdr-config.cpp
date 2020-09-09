// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "hdr-config.h"
#include "ds5/ds5-private.h"

namespace librealsense
{
    hdr_config::hdr_config(hw_monitor& hwm, std::shared_ptr<sensor_base> depth_ep) :
        _hwm(hwm),
        _sensor(depth_ep),
        _options_ranges_initialized(false)
    {  
        reset_to_default();
    }

    void hdr_config::reset_to_default()
    {
        _is_enabled = false;
        _is_config_in_process = false;
        _has_config_changed = false;
        _current_hdr_sequence_index = DEFAULT_CURRENT_HDR_SEQUENCE_INDEX;

        _auto_exposure_to_be_restored = false;
        _emitter_on_off_to_be_restored = false;

        _id = DEFAULT_HDR_ID;
        _sequence_size = DEFAULT_HDR_SEQUENCE_SIZE;
        _hdr_sequence_params.clear();
        _hdr_sequence_params.resize(DEFAULT_HDR_SEQUENCE_SIZE);
        
        if (_options_ranges_initialized)
        {
            float exposure_default_value = _exposure_range.def;
            float gain_default_value = _gain_range.def;
            hdr_params params_0(0, exposure_default_value, gain_default_value);
            _hdr_sequence_params[0] = params_0;

            float exposure_min_value = _exposure_range.min;
            float gain_min_value = _gain_range.min;
            hdr_params params_1(1, exposure_min_value, gain_min_value);
            _hdr_sequence_params[1] = params_1;
        }
    }

    float hdr_config::get(rs2_option option) const
    {
        float rv = 0.f;
        switch (option)
        {
        case RS2_OPTION_SUBPRESET_ID:
            rv = static_cast<float>(_id);
            break;
        case RS2_OPTION_SUBPRESET_SEQUENCE_SIZE:
            rv = static_cast<float>(_sequence_size);
            break;
        case RS2_OPTION_SUBPRESET_SEQUENCE_ID:
            rv = static_cast<float>(_current_hdr_sequence_index + 1);
            break;
        case RS2_OPTION_HDR_MODE:
            rv = static_cast<float>(_is_enabled);
            break;
        case RS2_OPTION_EXPOSURE:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._exposure;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception(to_string() << "hdr_config::get(...) failed! Index is above the sequence size.");
            }
            break;
        case RS2_OPTION_GAIN:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._gain;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception(to_string() << "hdr_config::get(...) failed! Index is above the sequence size.");
            }
            break;
        default:
            throw invalid_value_exception("option is not an HDR option");
        }
        return rv;
    }


    void hdr_config::set(rs2_option option, float value, option_range range)
    {
        if (value < range.min || value > range.max)
            throw invalid_value_exception(to_string() << "hdr_config::set(...) failed! value is out of the option range.");

        switch (option)
        {
        case RS2_OPTION_SUBPRESET_ID:
            set_id(value);
            break;
        case RS2_OPTION_SUBPRESET_SEQUENCE_SIZE:
            set_sequence_size(value);
            break;
        case RS2_OPTION_SUBPRESET_SEQUENCE_ID:
            set_sequence_index(value);
            break;
        case RS2_OPTION_HDR_MODE:
            set_enable_status(value);
            break;
        case RS2_OPTION_EXPOSURE:
            set_exposure(value, range);
            break;
        case RS2_OPTION_GAIN:
            set_gain(value);
            break;
        default:
            throw invalid_value_exception("option is not an HDR option");
        }

        if (_is_enabled && _has_config_changed)
        {
            send_sub_preset_to_fw();
        }
    }

    bool hdr_config::is_config_in_process() const
    {
        return _is_config_in_process;
    }

    void hdr_config::set_enable_status(float value)
    {
        if (value)
        {
            if (validate_config())
            {
                if (!_options_ranges_initialized)
                {
                    initialize_options_ranges();
                    _options_ranges_initialized = true;
                    reset_to_default();
                }

                // saving status of options that are not compatible with hdr,
                // so that they could be reenabled after hdr disable
                set_options_to_be_restored_after_disable();

                send_sub_preset_to_fw();
                _is_enabled = true;
                _has_config_changed = false;
            }
            else
                // msg to user to be improved later on
                throw invalid_value_exception("config is not valid");
        }
        else
        {
            disable(); 
            _is_enabled = false;

            // re-enabling options that were disabled in order to permit the hdr
            restore_options_after_disable();
        }
    }

    void hdr_config::initialize_options_ranges()
    {
        _exposure_range = _sensor->get_option(RS2_OPTION_EXPOSURE).get_range();
        _gain_range = _sensor->get_option(RS2_OPTION_GAIN).get_range();
    }

    void hdr_config::set_options_to_be_restored_after_disable()
    {
        // AUTO EXPOSURE
        if (_sensor->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).query())
        {
            _sensor->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(0.f);
            _auto_exposure_to_be_restored = true;
        }

        // EMITTER ON OFF
        if (_sensor->get_option(RS2_OPTION_EMITTER_ON_OFF).query())
        {
            //_sensor->get_option(RS2_OPTION_EMITTER_ON_OFF).set(0.f);
            _emitter_on_off_to_be_restored = true;
        }
    }

    void hdr_config::restore_options_after_disable()
    {
        // AUTO EXPOSURE
        if (_auto_exposure_to_be_restored)
        {
            _sensor->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(1.f);
            _auto_exposure_to_be_restored = false;
        }

        // EMITTER ON OFF
        if (_emitter_on_off_to_be_restored)
        {
            _sensor->get_option(RS2_OPTION_EMITTER_ON_OFF).set(1.f);
            _emitter_on_off_to_be_restored = false;
        }
    }

    void hdr_config::send_sub_preset_to_fw()
    {
        // prepare sub-preset command
        command cmd = prepare_hdr_sub_preset_command();
        auto res = _hwm.send(cmd);
    }

    void hdr_config::disable()
    {
        // sending empty sub preset
        std::vector<uint8_t> pattern{};

        // TODO - make it usable not only for ds - use _sensor
        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        auto res = _hwm.send(cmd);
    }

    //helper method - for debug only - to be deleted
    std::string hdrchar2hex(unsigned char n)
    {
        std::string res;

        do
        {
            res += "0123456789ABCDEF"[n % 16];
            n >>= 4;
        } while (n);

        reverse(res.begin(), res.end());

        if (res.size() == 1)
        {
            res.insert(0, "0");
        }

        return res;
    }


    command hdr_config::prepare_hdr_sub_preset_command() const
    {
        std::vector<uint8_t> subpreset_header = prepare_sub_preset_header();
        std::vector<uint8_t> subpreset_frames_config = prepare_sub_preset_frames_config();

        std::vector<uint8_t> pattern{};
        if (subpreset_frames_config.size() > 0)
        {
            pattern.insert(pattern.end(), &subpreset_header[0], &subpreset_header[0] + subpreset_header.size());
            pattern.insert(pattern.end(), &subpreset_frames_config[0], &subpreset_frames_config[0] + subpreset_frames_config.size());
        }
        
        /*std::cout << "pattern for hdr sub-preset: ";
        for (int i = 0; i < pattern.size(); ++i)
            std::cout << hdrchar2hex(pattern[i]) << " ";
        std::cout << std::endl;

        std::ofstream outFile("subpreset_bytes.txt", std::ofstream::out);
        outFile << "pattern for hdr sub-preset: ";
        for (int i = 0; i < pattern.size(); ++i)
            outFile << hdrchar2hex(pattern[i]) << " ";
        outFile << std::endl;*/

        //uint8_t sub_preset_opcode = _sensor->get_set_sub_preset_opcode();
        // TODO - make it usable not only for ds - use _sensor
        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        return cmd;
    }

    std::vector<uint8_t> hdr_config::prepare_sub_preset_header() const
    {
        //size
        uint16_t header_size = 25;
        //name
        static const int SUB_PRESET_NAME_LENGTH = 20;
        uint8_t sub_preset_name[SUB_PRESET_NAME_LENGTH];
        memset(sub_preset_name, 0, SUB_PRESET_NAME_LENGTH);
        const char* name = "HDRSubPreset";
        const int lim = std::min(static_cast<const int>(strlen(name)), SUB_PRESET_NAME_LENGTH);
        for (int i = 0; i < lim ; ++i)
        {
            sub_preset_name[i] = name[i];
        }
        //iterations - always 0 so that it will be continuous until stopped
        uint8_t iterations = 0;
        //sequence size
        uint16_t num_of_items = static_cast<uint16_t>(_sequence_size);
        
        std::vector<uint8_t> header;
        header.insert(header.end(), (uint8_t*)&header_size, (uint8_t*)&header_size + 2);
        header.insert(header.end(), &sub_preset_name[0], &sub_preset_name[0] + SUB_PRESET_NAME_LENGTH);
        header.insert(header.end(), &iterations, &iterations + 1);
        header.insert(header.end(), (uint8_t*)&num_of_items, (uint8_t*)&num_of_items + 2);

        return header;
    }

    std::vector<uint8_t> hdr_config::prepare_sub_preset_frames_config() const
    {
        //size for each frame header
        uint16_t frame_header_size = 5;
        //number of iterations for each frame
        uint8_t iterations = 1;
        // number of Controls for current frame
        uint16_t num_of_controls = 2;

        std::vector<uint8_t> frame_header;
        frame_header.insert(frame_header.end(), (uint8_t*)&frame_header_size, (uint8_t*)&frame_header_size + 2);
        frame_header.insert(frame_header.end(), &iterations, &iterations + 1);
        frame_header.insert(frame_header.end(), (uint8_t*)&num_of_controls, (uint8_t*)&num_of_controls + 2);

        std::vector<uint8_t> frames_config;
        for (int i = 0; i < _sequence_size; ++i)
        {
            frames_config.insert(frames_config.end(), &frame_header[0], &frame_header[0] + frame_header.size());

            uint16_t exposure_id = static_cast<uint16_t>(depth_manual_exposure);
            uint32_t exposure_value = static_cast<uint32_t>(_hdr_sequence_params[i]._exposure);
            frames_config.insert(frames_config.end(), (uint8_t*)&exposure_id, (uint8_t*)&exposure_id + 2);
            frames_config.insert(frames_config.end(), (uint8_t*)&exposure_value, (uint8_t*)&exposure_value + 4);
            
            uint16_t gain_id = static_cast<uint16_t>(depth_gain);
            uint32_t gain_value = static_cast<uint32_t>(_hdr_sequence_params[i]._gain);
            frames_config.insert(frames_config.end(), (uint8_t*)&gain_id, (uint8_t*)&gain_id + 2);
            frames_config.insert(frames_config.end(), (uint8_t*)&gain_value, (uint8_t*)&gain_value + 4);
        }

        return frames_config;
    }

    bool hdr_config::validate_config() const
    {
        // to be elaborated or deleted
        return true;
    }

    void hdr_config::set_id(float value)
    {
        int new_id = static_cast<int>(value);
        
        if (new_id != _id)
        {
            _id = new_id;
        }
    }

    void hdr_config::set_sequence_size(float value)
    {
        size_t new_size = static_cast<size_t>(value);
        if (new_size > 3 || new_size < 2)
            throw invalid_value_exception(to_string() << "hdr_config::set_sequence_size(...) failed! Only size 2 or 3 are supported.");

        if (new_size != _sequence_size)
        {
            _hdr_sequence_params.resize(new_size);
            _sequence_size = new_size;
        }       
    }

    void hdr_config::set_sequence_index(float value)
    {
        size_t new_index = static_cast<size_t>(value);
        
        _is_config_in_process = (new_index != 0);

        if (new_index <= _hdr_sequence_params.size())
        {
            _current_hdr_sequence_index = new_index - 1;
            _has_config_changed = true;
        }
        else
            throw invalid_value_exception(to_string() << "hdr_config::set_sequence_index(...) failed! Index above sequence size.");
    }

    void hdr_config::set_exposure(float value, option_range range)
    {
        /* TODO - add limitation on max exposure to be below frame interval - is range really needed for this?*/
        _hdr_sequence_params[_current_hdr_sequence_index]._exposure = value;
        _has_config_changed = true;
    }

    void hdr_config::set_gain(float value)
    {
        _hdr_sequence_params[_current_hdr_sequence_index]._gain = value;
        _has_config_changed = true;
    }


    hdr_params::hdr_params() :
        _sequence_id(0),
        _exposure(0.f),
        _gain(0.f)
    {}
    
    hdr_params::hdr_params(int sequence_id, float exposure, float gain) :
        _sequence_id(sequence_id),
        _exposure(exposure),
        _gain(gain)
    {}

    hdr_params& hdr_params::operator=(const hdr_params& other)
    {
        _sequence_id = other._sequence_id;
        _exposure = other._exposure;
        _gain = other._gain;

        return *this;
    }

    

    // explanation for the sub-preset:
    /* the structure is:
    
    #pragma pack(push, 1)
    typedef uint8_t SubPresetName[SUB_PRESET_NAME_LEN];

    typedef struct SubPresetHeader
    {
        uint16_t      headerSize;
        SubPresetName name;
        uint8_t       iterations;
        uint16_t      numOfItems;
    }SubPresetHeader;

    typedef struct SubPresetItemHeader
    {
        uint16_t headerSize;
        uint8_t  iterations;
        uint16_t numOfControls;
    }SubPresetItemHeader;

    typedef struct SubPresetControl
    {
        uint16_t controlId;
        uint32_t controlValue;
    }SubPresetControl;
    #pragma pack(pop) 
    #define SUB_PRESET_BUFFER_SIZE 1000
    #define SUB_PRESET_NAME_LEN    20 
     */


}