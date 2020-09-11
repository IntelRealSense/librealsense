// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "hdr-config.h"
#include "ds5/ds5-private.h"

namespace librealsense
{
    hdr_config::hdr_config(hw_monitor& hwm, std::shared_ptr<sensor_base> depth_ep, 
        const option_range& exposure_range, const option_range& gain_range) :
        _hwm(hwm),
        _sensor(depth_ep),
        _is_enabled(false),
        _is_config_in_process(false),
        _has_config_changed(false),
        _current_hdr_sequence_index(DEFAULT_CURRENT_HDR_SEQUENCE_INDEX),
        _auto_exposure_to_be_restored(false),
        _emitter_on_off_to_be_restored(false),
        _id(DEFAULT_HDR_ID),
        _sequence_size(DEFAULT_HDR_SEQUENCE_SIZE),
        _exposure_range(exposure_range),
        _gain_range(gain_range)
    {  
        _hdr_sequence_params.clear();
        _hdr_sequence_params.resize(DEFAULT_HDR_SEQUENCE_SIZE);

        // setting default config
        float exposure_default_value = _exposure_range.def;
        float gain_default_value = _gain_range.def;
        hdr_params params_0(0, exposure_default_value, gain_default_value);
        _hdr_sequence_params[0] = params_0;

        float exposure_low_value = DEFAULT_CONFIG_LOW_EXPOSURE;
        float gain_min_value = _gain_range.min;
        hdr_params params_1(1, exposure_low_value, gain_min_value);
        _hdr_sequence_params[1] = params_1;
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
            rv = static_cast<float>(is_enabled());
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
            set_exposure(value);
            break;
        case RS2_OPTION_GAIN:
            set_gain(value);
            break;
        default:
            throw invalid_value_exception("option is not an HDR option");
        }

        // subpreset configuration change is immediately sent to firmware if HDR is already running
        if (_is_enabled && _has_config_changed)
        {
            send_sub_preset_to_fw();
        }
    }

    bool hdr_config::is_config_in_process() const
    {
        return _is_config_in_process;
    }

    bool hdr_config::is_enabled() const
    {
        float rv = 0.f;
        command cmd(ds::GETSUBPRESETID);
        // if no subpreset is streaming, the firmware returns "ON_DATA_TO_RETURN" error
        try {
            auto res = _hwm.send(cmd);
            // if a subpreset is streaming, checking this is the current HDR sub preset
            rv = (res[0] == _id) ? 1.0f : 0.f;
        }
        catch (...)
        {
            rv = 0.f;
        }

        _is_enabled = (rv == 1.f);

        return rv;
    }

    void hdr_config::set_enable_status(float value)
    {
        if (value)
        {
            if (validate_config())
            {
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


    void hdr_config::set_options_to_be_restored_after_disable()
    {
        // AUTO EXPOSURE
        if (_sensor->supports_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        {
            if (_sensor->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).query())
            {
                _sensor->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(0.f);
                _auto_exposure_to_be_restored = true;
            }
        }
        

        // EMITTER ON OFF
        if (_sensor->supports_option(RS2_OPTION_EMITTER_ON_OFF))
        {
            if (_sensor->get_option(RS2_OPTION_EMITTER_ON_OFF).query())
            {
                //_sensor->get_option(RS2_OPTION_EMITTER_ON_OFF).set(0.f);
                _emitter_on_off_to_be_restored = true;
            }
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
        uint8_t header_size = 5;
        //id - from member (uint8_t)
        //iterations - always 0 so that it will be continuous until stopped
        uint16_t iterations = 0;
        //sequence size
        uint8_t num_of_items = static_cast<uint8_t>(_sequence_size);
        
        std::vector<uint8_t> header;
        header.insert(header.end(), &header_size, &header_size + 1);
        header.insert(header.end(), &_id, &_id + 1);
        header.insert(header.end(), (uint8_t*)&iterations, (uint8_t*)&iterations + 2);
        header.insert(header.end(), &num_of_items, &num_of_items + 1);

        return header;
    }

    std::vector<uint8_t> hdr_config::prepare_sub_preset_frames_config() const
    {
        //size for each frame header
        uint8_t frame_header_size = 4;
        //number of iterations for each frame
        uint16_t iterations = 1;
        // number of Controls for current frame
        uint8_t num_of_controls = 2;

        std::vector<uint8_t> frame_header;
        frame_header.insert(frame_header.end(), &frame_header_size, &frame_header_size + 1);
        frame_header.insert(frame_header.end(), (uint8_t*)&iterations, (uint8_t*)&iterations + 2);
        frame_header.insert(frame_header.end(), &num_of_controls, &num_of_controls + 1);

        std::vector<uint8_t> frames_config;
        for (int i = 0; i < _sequence_size; ++i)
        {
            frames_config.insert(frames_config.end(), &frame_header[0], &frame_header[0] + frame_header.size());

            uint32_t exposure_value = static_cast<uint32_t>(_hdr_sequence_params[i]._exposure);
            frames_config.insert(frames_config.end(), &CONTROL_ID_EXPOSURE, &CONTROL_ID_EXPOSURE + 1);
            frames_config.insert(frames_config.end(), (uint8_t*)&exposure_value, (uint8_t*)&exposure_value + 4);
            
            uint32_t gain_value = static_cast<uint32_t>(_hdr_sequence_params[i]._gain);
            frames_config.insert(frames_config.end(), &CONTROL_ID_GAIN, &CONTROL_ID_GAIN + 1);
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

    void hdr_config::set_exposure(float value)
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
    
    #define SUB_PRESET_BUFFER_SIZE 0x400
    #pragma pack(push, 1)
    typedef struct SubPresetHeader
    {
        uint8_t  headerSize;
        uint8_t  id;
        uint16_t iterations;
        uint8_t  numOfItems;
    }SubPresetHeader;

    typedef struct SubPresetItemHeader
    {
        uint8_t  headerSize;
        uint16_t iterations;
        uint8_t  numOfControls;
    }SubPresetItemHeader;

    typedef struct SubPresetControl
    {
        uint8_t  controlId;
        uint32_t controlValue;
    }SubPresetControl;
    #pragma pack(pop) 
     */


}