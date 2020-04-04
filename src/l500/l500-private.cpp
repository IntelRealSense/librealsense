//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "l500-private.h"
#include "fw-update/fw-update-unsigned.h"
#include "context.h"
#include "core/video.h"

using namespace std;

namespace librealsense
{
    namespace ivcam2
    {
        pose get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data)
        {
            if (raw_data.size() < sizeof(pose))
                throw invalid_value_exception("size of extrinsic invalid");
            auto res = *((pose*)raw_data.data());
            float trans_scale = 0.001f; // Convert units from mm to meter

            if (res.position.y > 0.f) // Extrinsic of color is referenced to the Depth Sensor CS
                trans_scale *= -1;

            res.position.x *= trans_scale;
            res.position.y *= trans_scale;
            res.position.z *= trans_scale;
            return res;
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

#pragma pack(push, 1)
            struct temperatures
            {
                double LLD_temperature;
                double MC_temperature;
                double MA_temperature;
                double APD_temperature;
            };
#pragma pack(pop)

            auto res = _hw_monitor->send(command{ TEMPERATURES_GET });

            if (res.size() < sizeof(temperatures))
            {
                throw std::runtime_error("Invalid result size!");
            }

            auto temperature_data = *(reinterpret_cast<temperatures*>((void*)res.data()));

            switch (_option)
            {
            case RS2_OPTION_LLD_TEMPERATURE:
                return float(temperature_data.LLD_temperature);
            case RS2_OPTION_MC_TEMPERATURE:
                return float(temperature_data.MC_temperature);
            case RS2_OPTION_MA_TEMPERATURE:
                return float(temperature_data.MA_temperature);
            case RS2_OPTION_APD_TEMPERATURE:
                return float(temperature_data.APD_temperature);
            default:
                throw invalid_value_exception(to_string() << _option << " is not temperature option!");
            }
        }

        l500_temperature_options::l500_temperature_options(hw_monitor* hw_monitor, rs2_option opt)
            :_hw_monitor(hw_monitor), _option(opt)
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

        autocal_option::autocal_option( hw_monitor& hwm )
            : bool_option( false )
            , _hwm( hwm )
        {
        }

        void autocal_option::trigger_special_frame()
        {
            std::cout << "-D- Sending HW command: GET_SPECIAL_FRAME" << std::endl;
            command cmd{ GET_SPECIAL_FRAME, 0x5F, 1 };  // 5F = SF = Special Frame, for easy recognition
            auto res = _hwm.send( cmd );
        }

        void autocal_option::set( float value )
        {
            bool_option::set( value );
            if( is_true() )
            {
                // We've turned it on -- try to immediately get a special frame
                trigger_special_frame();
            }
            _record_action( *this );
        }


        auto_calibration::auto_calibration( std::shared_ptr< autocal_option > enabler_opt )
            : _is_processing{ false }
            , _enabler_opt( enabler_opt )
        {
        }

        auto_calibration::~auto_calibration()
        {
            if( _worker.joinable() )
            {
                _is_processing = false;  // Signal the thread that we want to stop!
                _worker.join();
            }
        }

        void auto_calibration::set_special_frame( rs2::frameset const& fs )
        {
            if( _is_processing )
                return;
            auto irf = fs.get_infrared_frame();
            if( ! irf )
            {
                LOG_ERROR( "Ignoring special frame: no IR frame found!" );
                std::cout << "-D- no IR frame; ignoring" << std::endl;
                call_back( RS2_CALIBRATION_FAILED );
                return;
            }
            auto df = fs.get_depth_frame();
            if( !df )
            {
                LOG_ERROR( "Ignoring special frame: no depth frame found!" );
                std::cout << "-D- no depth frame; ignoring" << std::endl;
                call_back( RS2_CALIBRATION_FAILED );
                return;
            }

            _sf = fs;

            if( check_color_depth_sync() )
                start();
        }

        void auto_calibration::set_color_frame( rs2::frame const& f )
        {
            if( _is_processing )
                return;

            _pcf = _cf;
            _cf = f;
            if( check_color_depth_sync() )
                start();
        }


        bool auto_calibration::check_color_depth_sync()
        {
            if( !_sf )
            {
                //std::cout << "-D- no special frame yet" << std::endl;
                return false;
            }
            if( !_cf )
            {
                //std::cout << "-D- no color frame yet" << std::endl;
                return false;
            }
            return true;
        }

        void auto_calibration::start()
        {
            std::cout << "-D- in start(); processing... " << std::endl;
            _is_processing = true;
            if( _worker.joinable() )
                _worker.join();
            _worker = std::thread([&]()
            {
                try
                {
                    LOG_INFO("Auto calibration has started ...");
                    call_back( RS2_CALIBRATION_STARTED );

                    auto df = _sf.get_depth_frame();
                    auto irf = _sf.get_infrared_frame();
                    auto_cal_algo algo( df, irf, _cf, _pcf );
                    _from_profile = algo.get_from_profile();
                    _to_profile = algo.get_to_profile();

                    //calibration new_calib = { rs2_extrinsics{0}, rs2_intrinsics{0}, df.get_profile().get()->profile, _cf.get_profile().get()->profile };
                    if( algo.optimize() )
                    {
                        std::cout << "-D- optimized" << std::endl;
                        /*  auto prof = _cf.get_profile().get()->profile;
                        auto&& video = dynamic_cast<video_stream_profile_interface*>(prof);
                        if (video)
                            video->set_intrinsics([new_calib]() {return new_calib.intrinsics;});
                        _df.get_profile().register_extrinsics_to(_cf.get_profile(), new_calib.extrinsics);*/
                        _extr = algo.get_extrinsics();
                        _intr = algo.get_intrinsics();
                        call_back( RS2_CALIBRATION_SUCCESSFUL );
                    }
                    else
                    {
                        call_back( RS2_CALIBRATION_FAILED );
                    }

                    reset();
                    LOG_INFO("Auto calibration has finished ...");
                }
                catch (...)
                {
                    std::cout << "-D- exception!!!!!!!!!!!!!" << std::endl;
                    call_back( RS2_CALIBRATION_FAILED );
                    reset();
                    LOG_ERROR("Auto calibration has finished ...");
                }});
        }

        void auto_calibration::reset()
        {
            _sf = rs2::frame{};
            _cf = rs2::frame{};;
            _pcf = rs2::frame{};

            _is_processing = false;
            std::cout << "-D- reset() " << std::endl;
        }

        autocal_depth_processing_block::autocal_depth_processing_block(
            std::shared_ptr< auto_calibration > autocal
        )
            : generic_processing_block( "Auto Calibration (depth)" )
            , _autocal{ autocal }
            , _is_enabled_opt( autocal->get_enabler_opt() )
        {
        }

        static bool is_special_frame( rs2::frame const& f )
        {
            if( f && !f.supports_frame_metadata( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE ) )
                std::cout << "-D- no LASER_POWER_MODE" << std::endl;
            return(f
                && f.supports_frame_metadata( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE )
                && 0x5F == f.get_frame_metadata( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE ));
        }

        rs2::frame autocal_depth_processing_block::process_frame( const rs2::frame_source& source, const rs2::frame& f )
        {
            // If is_enabled_opt is false, meaning this processing block is not active,
            // return the frame as is.
            if( auto is_enabled = _is_enabled_opt.lock() )
                if( !is_enabled->is_true() )
                    ; // return f;

            auto fs = f.as< rs2::frameset >();
            if( fs )
            {
                auto df = fs.get_depth_frame();
                if( is_special_frame( df ))
                {
                    LOG_INFO( "Auto calibration SF received" );
                    std::cout << "-D- Got special frame" << std::endl;
                    _autocal->set_special_frame( f );
                }
                // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
                return rs2::frame{};
            }

            if( is_special_frame( f.as< rs2::depth_frame >() ) )
                // We don't want the user getting this frame!
                return rs2::frame{};
            
            return f;
        }

        bool autocal_depth_processing_block::should_process( const rs2::frame & frame )
        {
            return true;
        }

        rs2::frame autocal_depth_processing_block::prepare_output( const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results )
        {
            // The default prepare_output() will send the input back as the frame if the results are empty
            if( results.empty() )
                return rs2::frame{};

            return source.allocate_composite_frame( results );

            //return generic_processing_block::prepare_output( source, input, results );
        }

        autocal_color_processing_block::autocal_color_processing_block(
            std::shared_ptr< auto_calibration > autocal
        )
            : generic_processing_block( "Auto Calibration (color)" )
            , _autocal{ autocal }
            , _is_enabled_opt( autocal->get_enabler_opt() )
        {
        }

        rs2::frame autocal_color_processing_block::process_frame( const rs2::frame_source& source, const rs2::frame& f )
        {
            // If is_enabled_opt is false, meaning this processing block is not active,
            // return the frame as is.
            if( auto is_enabled = _is_enabled_opt.lock() )
                if( !is_enabled->is_true() )
                    ; // return f;

            // Disregard framesets: we'll get those broken down into individual frames by generic_processing_block's on_frame
            if( f.is< rs2::frameset >() )
                return rs2::frame{};
            
            // We record each and every color frame
            _autocal->set_color_frame( f );

            // Return the frame as is!
            return f;
        }

        bool autocal_color_processing_block::should_process( const rs2::frame& frame )
        {
            return true;
        }
#if 0
        rs2::frame autocal_color_processing_block::prepare_output( const rs2::frame_source& source, rs2::frame input, std::vector<rs2::frame> results )
        {
            // The default prepare_output() will send the input back as the frame if the results are empty
            if( results.empty() )
                return rs2::frame{};

            return generic_processing_block::prepare_output( source, input, results );
        }
#endif
    } // librealsense::ivcam2
} // namespace librealsense
