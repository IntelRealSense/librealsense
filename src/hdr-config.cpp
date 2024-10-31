// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "hdr-config.h"
#include "ds/d400/d400-private.h"

#include <rsutils/string/from.h>


namespace librealsense
{
    hdr_config::hdr_config(hw_monitor& hwm, std::shared_ptr<sensor_base> depth_ep,
        const option_range& exposure_range, const option_range& gain_range, hwmon_response_type no_data_to_return_opcode) :
        _hwm(hwm),
        _sensor(depth_ep),
        _is_enabled(false),
        _is_config_in_process(false),
        _current_hdr_sequence_index(DEFAULT_CURRENT_HDR_SEQUENCE_INDEX),
        _auto_exposure_to_be_restored(false),
        _emitter_on_off_to_be_restored(false),
        _id(DEFAULT_HDR_ID),
        _sequence_size(DEFAULT_HDR_SEQUENCE_SIZE),
        _exposure_range(exposure_range),
        _gain_range(gain_range),
        _use_workaround(true),
        _pre_hdr_exposure(0.f),
        _no_data_to_return_opcode(no_data_to_return_opcode)
    {
        _hdr_sequence_params.clear();
        _hdr_sequence_params.resize(DEFAULT_HDR_SEQUENCE_SIZE);

        // restoring current HDR configuration if such subpreset is active
        bool existing_subpreset_restored = false;
        std::vector< uint8_t > res;
        if (is_hdr_enabled_in_device(res))
            existing_subpreset_restored = configure_hdr_as_in_fw(res);

        if (!existing_subpreset_restored)
        {
            // setting default config
            float exposure_default_value = _exposure_range.def-1000.f; // D455 W/A
            float gain_default_value = _gain_range.def;
            hdr_params params_0(0, exposure_default_value, gain_default_value);
            _hdr_sequence_params[0] = params_0;

            float exposure_low_value = _exposure_range.min;
            float gain_min_value = _gain_range.min;
            hdr_params params_1(1, exposure_low_value, gain_min_value);
            _hdr_sequence_params[1] = params_1;
        }
    }

    bool hdr_config::is_hdr_enabled_in_device( std::vector< uint8_t > & result ) const
    {
        command cmd(ds::GETSUBPRESET);
        bool hdr_enabled_in_device = false;
        try {
            result = _hwm.send(cmd);
            hdr_enabled_in_device = (result.size() && is_current_subpreset_hdr(result));
        }
        catch (std::exception ex) {
            LOG_WARNING("In hdr_config::hdr_config() - hw command failed: " << ex.what());
        }
        return hdr_enabled_in_device;
    }

    bool hdr_config::is_current_subpreset_hdr( const std::vector< uint8_t > & current_subpreset ) const
    {
        bool result = false;
        if (current_subpreset.size() > 0)
        {
            int current_subpreset_id = current_subpreset[1];
            result = is_hdr_id(current_subpreset_id);
        }
        return result;
    }

    bool hdr_config::is_hdr_id(int id) const
    {
        return id >= 0 && id <= 3;
    }

    bool hdr_config::configure_hdr_as_in_fw( const std::vector< uint8_t > & current_subpreset )
    {
        // parsing subpreset pattern, considering:
        // SubPresetHeader::iterations always equals 0 (continuous subpreset)
        // SubPresetHeader::numOfItems always equals 2 - for gain and exposure
        // SubPresetItemHeader::iterations always equals 1 - only one frame on each sequence ID in the sequence
        const int size_of_subpreset_header = 5;
        const int size_of_subpreset_item_header = 4;
        const int size_of_control_id = 1;
        const int size_of_control_value = 4;

        int subpreset_size = size_of_subpreset_header + 2 * (size_of_subpreset_item_header +
            2 * (size_of_control_id + size_of_control_value));

        if (current_subpreset.size() != subpreset_size)
            return false;

        int offset = 0;
        offset += size_of_subpreset_header;
        offset += size_of_subpreset_item_header;

        if (current_subpreset[offset] != CONTROL_ID_EXPOSURE)
            return false;
        offset += size_of_control_id;
        float exposure_0
            = (float)*reinterpret_cast< const uint32_t * >( &( current_subpreset[offset] ) );
        offset += size_of_control_value;

        if (current_subpreset[offset] != CONTROL_ID_GAIN)
            return false;
        offset += size_of_control_id;
        float gain_0
            = (float)*reinterpret_cast< const uint32_t * >( &( current_subpreset[offset] ) );
        offset += size_of_control_value;

        offset += size_of_subpreset_item_header;

        if (current_subpreset[offset] != CONTROL_ID_EXPOSURE)
            return false;
        offset += size_of_control_id;
        float exposure_1
            = (float)*reinterpret_cast< const uint32_t * >( &( current_subpreset[offset] ) );
        offset += size_of_control_value;

        if (current_subpreset[offset] != CONTROL_ID_GAIN)
            return false;
        offset += size_of_control_id;
        float gain_1
            = (float)*reinterpret_cast< const uint32_t * >( &( current_subpreset[offset] ) );
        offset += size_of_control_value;

        _hdr_sequence_params[0]._exposure = exposure_0;
        _hdr_sequence_params[0]._gain = gain_0;
        _hdr_sequence_params[1]._exposure = exposure_1;
        _hdr_sequence_params[1]._gain = gain_1;

        return true;
    }

    float hdr_config::get(rs2_option option) const
    {
        float rv = 0.f;
        switch (option)
        {
        case RS2_OPTION_SEQUENCE_NAME:
            rv = static_cast<float>(_id);
            break;
        case RS2_OPTION_SEQUENCE_SIZE:
            rv = static_cast<float>(_sequence_size);
            break;
        case RS2_OPTION_SEQUENCE_ID:
            rv = static_cast<float>(_current_hdr_sequence_index + 1);
            break;
        case RS2_OPTION_HDR_ENABLED:
            rv = static_cast<float>(is_enabled());
            break;
        case RS2_OPTION_EXPOSURE:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._exposure;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception( rsutils::string::from()
                                               << "hdr_config::get(...) failed! Index is above the sequence size." );
            }
            break;
        case RS2_OPTION_GAIN:
            try {
                rv = _hdr_sequence_params[_current_hdr_sequence_index]._gain;
            }
            // should never happen
            catch (std::out_of_range)
            {
                throw invalid_value_exception( rsutils::string::from()
                                               << "hdr_config::get(...) failed! Index is above the sequence size." );
            }
            break;
        default:
            throw invalid_value_exception( rsutils::string::from()
                                           << "option: " << rs2_option_to_string( option ) << " is not an HDR option" );
        }
        return rv;
    }

    void hdr_config::set(rs2_option option, float value, option_range range)
    {
        if (value < range.min || value > range.max)
            throw invalid_value_exception( rsutils::string::from() << "hdr_config::set(...) failed! value: " << value
                                                                   << " is out of the option range: [" << range.min
                                                                   << ", " << range.max << "]." );

        switch (option)
        {
        case RS2_OPTION_SEQUENCE_NAME:
            set_id(value);
            break;
        case RS2_OPTION_SEQUENCE_SIZE:
            set_sequence_size(value);
            break;
        case RS2_OPTION_SEQUENCE_ID:
            set_sequence_index(value);
            break;
        case RS2_OPTION_HDR_ENABLED:
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
    }

    bool hdr_config::is_config_in_process() const
    {
        return _is_config_in_process;
    }

    bool hdr_config::is_enabled() const
    {
        // status in the firmware must be checked in case this is a new instance but the HDR in enabled in firmware
        if (!_is_enabled)
        {
            float rv = 0.f;
            command cmd(ds::GETSUBPRESETID);
            try
            {
                hwmon_response_type response;
                auto res = _hwm.send( cmd, &response );  // avoid the throw
                if (response != _no_data_to_return_opcode) // If no subpreset is streaming, the firmware returns "NO_DATA_TO_RETURN" error
                {
                    // If a subpreset is streaming, checking this is the current HDR sub preset
                    if( res.size() )
                        rv = ( is_hdr_id( res[0] ) ) ? 1.0f : 0.f;
                    else
                        LOG_DEBUG( "hdr_config query: " << _hwm.hwmon_error_string( cmd, response ) );
                }
            }
            catch (...)
            {
            }

            _is_enabled = (rv == 1.f);
        }

        return _is_enabled;
    }

    void hdr_config::set_enable_status(float value)
    {
        if (value)
        {
            if (validate_config())
            {
                std::vector< uint8_t > res;
                _is_enabled = is_hdr_enabled_in_device(res);
                if (!_is_enabled)
                {
                    // saving status of options that are not compatible with hdr,
                    // so that they could be reenabled after hdr disable
                    set_options_to_be_restored_after_disable();

                    if (_use_workaround)
                    {
                        try {
                            // the following statement is needed in order to get/set the UVC exposure 
                            // instead of one of the hdr's configuration exposure
                            set_sequence_index(0.f);
                            _pre_hdr_exposure = _sensor.lock()->get_option(RS2_OPTION_EXPOSURE).query();
                            _sensor.lock()->get_option(RS2_OPTION_EXPOSURE).set(PRE_ENABLE_HDR_EXPOSURE);
                        } catch (...) {
                            LOG_WARNING("HDR: enforced exposure failed");
                        }
                    }

                    _is_enabled = send_sub_preset_to_fw();
                    if (!_is_enabled)
                    {
                        LOG_WARNING("Couldn't enable HDR." );
                    }
                }
                else
                {
                    LOG_WARNING("HDR is already enabled. Skipping the request." );
                }
            }
            else
                // msg to user to be improved later on
                throw invalid_value_exception("config is not valid");
        }
        else
        {
            disable();
            _is_enabled = false;

            if (_use_workaround)
            {
                // this sleep is needed to let the fw restore the manual exposure
                std::this_thread::sleep_for(std::chrono::milliseconds(70));

                if (_pre_hdr_exposure >= _exposure_range.min && _pre_hdr_exposure <= _exposure_range.max)
                {
                    try {
                        // the following statement is needed in order to get the UVC exposure 
                        // instead of one of the hdr's configuration exposure
                        set_sequence_index(0.f);
                        _sensor.lock()->get_option(RS2_OPTION_EXPOSURE).set(_pre_hdr_exposure);
                    } catch (...) {
                        LOG_WARNING("HDR failed to restore manual exposure");
                    }
                }
            }

            // re-enabling options that were disabled in order to permit the hdr
            restore_options_after_disable();
        }
    }


    void hdr_config::set_options_to_be_restored_after_disable()
    {
        // AUTO EXPOSURE
        if (_sensor.lock()->supports_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        {
            if (_sensor.lock()->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).query())
            {
                _sensor.lock()->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(0.f);
                _auto_exposure_to_be_restored = true;
            }
        }


        // EMITTER ON OFF
        if (_sensor.lock()->supports_option(RS2_OPTION_EMITTER_ON_OFF))
        {
            if (_sensor.lock()->get_option(RS2_OPTION_EMITTER_ON_OFF).query())
            {
                _sensor.lock()->get_option(RS2_OPTION_EMITTER_ON_OFF).set(0.f);
                _emitter_on_off_to_be_restored = true;
            }
        }
    }

    void hdr_config::restore_options_after_disable()
    {
        // AUTO EXPOSURE
        if (_auto_exposure_to_be_restored)
        {
            _sensor.lock()->get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(1.f);
            _auto_exposure_to_be_restored = false;
        }

        // EMITTER ON OFF
        if (_emitter_on_off_to_be_restored)
        {
            _sensor.lock()->get_option(RS2_OPTION_EMITTER_ON_OFF).set(1.f);
            _emitter_on_off_to_be_restored = false;
        }
    }

    bool hdr_config::send_sub_preset_to_fw()
    {
        bool result = false;
        // prepare sub-preset command
        command cmd = prepare_hdr_sub_preset_command();
        try {
            auto res = _hwm.send(cmd);
            result = true;
        }
        catch (std::exception ex) {
            LOG_WARNING("In hdr_config::send_sub_preset_to_fw() - hw command failed: " << ex.what());
        }
        return result;
    }

    void hdr_config::disable()
    {
        // sending empty sub preset
        std::vector<uint8_t> pattern;

        // TODO - make it usable not only for ds - use _sensor
        command cmd(ds::SETSUBPRESET, static_cast<int>(pattern.size()));
        cmd.data = pattern;
        try {
            auto res = _hwm.send(cmd);
        }
        catch (std::exception ex) {
            LOG_WARNING("In hdr_config::disable() - hw command failed: " << ex.what());
        }
    }

    command hdr_config::prepare_hdr_sub_preset_command() const
    {
        std::vector<uint8_t> subpreset_header = prepare_sub_preset_header();
        std::vector<uint8_t> subpreset_frames_config = prepare_sub_preset_frames_config();

        std::vector<uint8_t> pattern;
        if (subpreset_frames_config.size() > 0)
        {
            pattern.insert(pattern.end(), &subpreset_header[0], &subpreset_header[0] + subpreset_header.size());
            pattern.insert(pattern.end(), &subpreset_frames_config[0], &subpreset_frames_config[0] + subpreset_frames_config.size());
        }

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
            throw invalid_value_exception(
                rsutils::string::from()
                << "hdr_config::set_sequence_size(...) failed! Only size 2 or 3 are supported." );

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
            _current_hdr_sequence_index = (int)new_index - 1;
        }
        else
            throw invalid_value_exception(
                rsutils::string::from() << "hdr_config::set_sequence_index(...) failed! Index above sequence size." );
    }

    void hdr_config::set_exposure(float value)
    {
        if (!_is_enabled)
            _hdr_sequence_params[_current_hdr_sequence_index]._exposure = value;
        else
            throw wrong_api_call_sequence_exception(rsutils::string::from()
                << "Cannot update HDR config (exposure) while HDR mode is active." );
    }

    void hdr_config::set_gain(float value)
    {
        if (!_is_enabled)
        {
            _hdr_sequence_params[_current_hdr_sequence_index]._gain = value;
        }
        else
            throw wrong_api_call_sequence_exception(rsutils::string::from()
                << "Cannot update HDR config (gain) while HDR mode is active." );
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
