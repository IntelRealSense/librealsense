//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-private.h"
#include "l500-device.h"
#include "l500-color.h"
#include "l500-depth.h"
#include "fw-update/fw-update-unsigned.h"
#include "context.h"
#include "core/video.h"
#include "depth-to-rgb-calibration.h"
#include "log.h"
#include <chrono>
#include "algo/depth-to-rgb-calibration/debug.h"

using namespace std;

namespace librealsense
{
    namespace ivcam2
    {
        const int ac_depth_results::table_id;
        const uint16_t ac_depth_results::this_version;
        
        const uint16_t rgb_calibration_table::table_id;
        const uint16_t rgb_calibration_table::eeprom_table_id;


        rs2_extrinsics get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
        {
            if (raw_data.size() < sizeof(pose))
                throw invalid_value_exception("size of extrinsic invalid");
            
            assert( sizeof( pose ) == sizeof( rs2_extrinsics ) );
            auto res = *(rs2_extrinsics*)raw_data.data();
            AC_LOG( DEBUG, "raw extrinsics data from camera:\n" << std::setprecision(15) << res );
            
            return from_raw_extrinsics(res);
        }

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
            const platform::uvc_device_info& info, platform::usb_device_info& result)
        {
            for (auto it = devices.begin(); it != devices.end(); ++it)
            {
                if (it->unique_id == info.unique_id)
                {

                    result = *it;
                    switch(info.pid)
                    {
                    case L515_PID_PRE_PRQ:
                    case L515_PID:
                        if(result.mi == 7)
                        {
                            devices.erase(it);
                            return true;
                        }
                        break;
                    case L500_PID:
                        if(result.mi == 4 || result.mi == 6)
                        {
                            devices.erase(it);
                            return true;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            return false;
        }
        float l500_temperature_options::query() const
        {
            if (!is_enabled())
                throw wrong_api_call_sequence_exception("query option is allow only in streaming!");
            
            auto temperature_data = _l500_depth_dev->get_temperatures();

            switch (_option)
            {
            case RS2_OPTION_LLD_TEMPERATURE:
                return float(temperature_data.LDD_temperature);
            case RS2_OPTION_MC_TEMPERATURE:
                return float(temperature_data.MC_temperature);
            case RS2_OPTION_MA_TEMPERATURE:
                return float(temperature_data.MA_temperature);
            case RS2_OPTION_APD_TEMPERATURE:
                return float(temperature_data.APD_temperature);
            case RS2_OPTION_HUMIDITY_TEMPERATURE:
                return float(temperature_data.HUM_temperature);
            default:
                throw invalid_value_exception(to_string() << _option << " is not temperature option!");
            }
        }

        l500_temperature_options::l500_temperature_options(l500_device *l500_depth_dev,
                                                            rs2_option opt,
                                                            const std::string& description )
            :_l500_depth_dev(l500_depth_dev)
            , _option( opt )
            , _description( description )
        {
        }

        flash_structure get_rw_flash_structure(const uint32_t flash_version)
        {
            switch (flash_version)
            {
                // { number of payloads in section { ro table types }} see Flash.xml
            case 103: return { 1, { 40, 320, 321, 326, 327, 54} };
            default:
                throw std::runtime_error("Unsupported flash version: " + std::to_string(flash_version));
            }
        }

        flash_structure get_ro_flash_structure(const uint32_t flash_version)
        {
            switch (flash_version)
            {
                // { number of payloads in section { ro table types }} see Flash.xml
            case 103: return { 4, { 256, 257, 258, 263, 264, 512, 25, 2 } };
            default:
                throw std::runtime_error("Unsupported flash version: " + std::to_string(flash_version));
            }
        }

        flash_info get_flash_info(const std::vector<uint8_t>& flash_buffer)
        {
            flash_info rv = {};

            uint32_t header_offset = FLASH_INFO_HEADER_OFFSET;
            memcpy(&rv.header, flash_buffer.data() + header_offset, sizeof(rv.header));

            uint32_t ro_toc_offset = FLASH_RO_TABLE_OF_CONTENT_OFFSET;
            uint32_t rw_toc_offset = FLASH_RW_TABLE_OF_CONTENT_OFFSET;

            auto ro_toc = parse_table_of_contents(flash_buffer, ro_toc_offset);
            auto rw_toc = parse_table_of_contents(flash_buffer, rw_toc_offset);

            auto ro_structure = get_ro_flash_structure(ro_toc.header.version);
            auto rw_structure = get_rw_flash_structure(rw_toc.header.version);

            rv.read_only_section = parse_flash_section(flash_buffer, ro_toc, ro_structure);
            rv.read_only_section.offset = rv.header.read_only_start_address;
            rv.read_write_section = parse_flash_section(flash_buffer, rw_toc, rw_structure);
            rv.read_write_section.offset = rv.header.read_write_start_address;

            return rv;
        }

        freefall_option::freefall_option( hw_monitor & hwm, bool enabled )
            : _hwm( hwm )
            , _enabled( enabled )
        {
            // bool_option initializes with def=true, which is what we want
            assert( is_true() );
        }

        void freefall_option::set( float value )
        {
            bool_option::set( value );

            command cmd{ FALL_DETECT_ENABLE, is_true() };
            auto res = _hwm.send( cmd );
            _record_action( *this );
        }

        void freefall_option::enable( bool e )
        {
            if( e != is_enabled() )
            {
                if( !e  &&  is_true() )
                    set( 0 );
                _enabled = e;
            }
        }

        hw_sync_option::hw_sync_option( hw_monitor& hwm, std::shared_ptr< freefall_option > freefall_opt )
            : bool_option( false )
            , _hwm( hwm )
            , _freefall_opt( freefall_opt )
        {
        }

        void hw_sync_option::set( float value )
        {
            bool_option::set( value );

            // Disable the free-fall option if we're enabled!
            if( _freefall_opt )
                _freefall_opt->enable( ! is_true() );

            command cmd{ HW_SYNC_EX_TRIGGER, is_true() };
            auto res = _hwm.send( cmd );
            _record_action( *this );
        }

        // Returns the number of weeks that passed since manufactoring of this device
        // The Serial Number format is PYWWXXXX:
        // P – Site Name(ex.“F” for Fabrinet)
        // Y – Year(ex.“9” for 2019, "0" for 2020, , "1" for 2021  ..etc)
        // WW – Work Week
        // XXXX – Sequential number
        uint8_t weeks_since_factory_calibration(const std::string& serial) {
            int Y = serial[1] - '0'; // Converts char to int, '0'-> 0, '1'-> 1, ...
            int man_year = 0;
            if (Y == 9) //using Y from serial number to get manufactoring year
                man_year = 2019;
            else
                man_year = 2020 + Y;
            int man_ww = std::stoi(serial.substr(2, 2)); //using WW from serial number to get manufactoring work week
            auto t = (std::time(nullptr));
            auto curr_time = std::localtime(&t);
            int curr_year = curr_time->tm_year + 1900; // The tm_year field contains the number of years since 1900, we add 1900 to get current year
            int curr_ww = curr_time->tm_yday / 7; // The tm_yday field contains the number of days sine 1st of January, we devide by 7 to get current work week
            curr_ww++; // We add 1 because work weeks start at 1 and not 0 (ex. 1st of January will return ww 0, but it ww 1)
            uint8_t age = ((curr_year - man_year) * 52) + (curr_ww - man_ww); // There are 52 weeks in a year
            return age;
        }
    } // librealsense::ivcam2
} // namespace librealsense
