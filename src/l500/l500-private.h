// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include "option.h"
#include "core/extension.h"
#include "fw-update/fw-update-unsigned.h"

static const int MAX_NUM_OF_RGB_RESOLUTIONS = 5;
static const int MAX_NUM_OF_DEPTH_RESOLUTIONS = 5; 

namespace librealsense
{
    const uint16_t L500_RECOVERY_PID            = 0x0b55;
    const uint16_t L535_RECOVERY_PID            = 0x0B72;
    const uint16_t L500_USB2_RECOVERY_PID_OLD   = 0x0adc; // Units with old DFU_PAYLAOD on USB2 report ds5 PID (RS_USB2_RECOVERY_PID)
    const uint16_t L500_PID                     = 0x0b0d;
    const uint16_t L515_PID_PRE_PRQ             = 0x0b3d;
    const uint16_t L515_PID                     = 0x0b64;
    const uint16_t L535_PID                     = 0x0b68;

    class l500_device;

    namespace ivcam2
    {
        // L500 depth XU identifiers
        const uint8_t L500_HWMONITOR = 1;
        const uint8_t L500_DIGITAL_GAIN = 2; // Renamed from L500_AMBIENT
        const uint8_t L500_ERROR_REPORTING = 3;

        const uint32_t FLASH_SIZE = 0x00200000;
        const uint32_t FLASH_SECTOR_SIZE = 0x1000;

        const uint32_t FLASH_RW_TABLE_OF_CONTENT_OFFSET = 0x0017FE00;
        const uint32_t FLASH_RO_TABLE_OF_CONTENT_OFFSET = 0x001FFD00;
        const uint32_t FLASH_INFO_HEADER_OFFSET = 0x001FFF00;

        flash_info get_flash_info(const std::vector<uint8_t>& flash_buffer);

        const platform::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        const int REGISTER_CLOCK_0 = 0x9003021c;

        const uint16_t L515_IMU_TABLE   = 0x0243;  // IMU calibration table on L515

        enum fw_cmd : uint8_t
        {
            IRB                         = 0x03, //"Read from i2c ( 8x8 )"
            MRD                         = 0x01, //"Read Tensilica memory ( 32bit ). Output : 32bit dump"
            FRB                         = 0x09, //"Read from flash"
            FWB                         = 0x0A, //"Write to flash"
            FES                         = 0x0B, //"Erase flash sector"
            FEF                         = 0x0C, //"Erase flash full"
            GLD                         = 0x0F, //"LoggerCoreGetDataParams"
            GVD                         = 0x10, //"Get Version and Date"
            DFU                         = 0x1E, //"Go to DFU"
            HW_SYNC_EX_TRIGGER          = 0x19, // Enable (not default) HW sync; will disable freefall
            HW_RESET                    = 0x20, //"HW Reset"
            AMCSET                      = 0x2B, // Set options (L515)
            AMCGET                      = 0x2C, // Get options (L515)
            DELETE_TABLE                = 0x2E,
            TPROC_TRB_THRSLD_SET        = 0x35, // TPROC TRB threshold
            TPROC_USB_GRAN_SET          = 0x36, // TPROC USB granularity
            PFD                         = 0x3B, // Disable power features <Parameter1 Name="0 - Disable, 1 - Enable" />
            READ_TABLE                  = 0x43, // read table from flash, for example, read imu calibration table, read_table 0x243 0
            WRITE_TABLE                 = 0x44,
            DPT_INTRINSICS_GET          = 0x5A,
            TEMPERATURES_GET            = 0x6A,
            DPT_INTRINSICS_FULL_GET     = 0x7F,
            RGB_INTRINSIC_GET           = 0x81,
            RGB_EXTRINSIC_GET           = 0x82,
            FALL_DETECT_ENABLE          = 0x9D, // Enable (by default) free-fall sensor shutoff (0=disable; 1=enable)
            GET_SPECIAL_FRAME           = 0xA0, // Request auto-calibration (0) special frames (#)
            UNIT_AGE_SET                = 0x5B  // Sets the age of the unit in weeks
        };

#pragma pack(push, 1)
        // Table header returned by READ_TABLE before the actual table data
        struct table_header
        {
            uint8_t                 major;
            uint8_t                 minor;
            uint16_t                table_id;
            uint32_t                table_size;     // full size including: TOC header + TOC + actual tables
            uint32_t                reserved;       // 0xFFFFFFFF
            uint32_t                crc32;          // crc of all the actual table data excluding header/CRC
        };
#pragma pack(pop)

        static std::vector< byte >
        read_fw_table_raw( const hw_monitor & hwm, int table_id, hwmon_response & response )
        {
            std::vector< byte > res;
            command cmd( fw_cmd::READ_TABLE, table_id );
            auto data = hwm.send( cmd, &response );

            res.assign( data.data(), data.data() + data.size() );

            return res;
        }

        // Read a table from firmware and, if FW says the table is empty, optionally initialize it
        // using your own code...
        template< typename T >
        void read_fw_table( hw_monitor& hwm,
                            int table_id, T * ptable,
                            table_header * pheader = nullptr,
                            std::function< void() > init = nullptr )
        {
            hwmon_response response;
            std::vector< byte > data = read_fw_table_raw( hwm, table_id, response );
            size_t expected_size = sizeof( table_header ) + sizeof( T );
            switch( response )
            {
            case hwm_Success:
                if( data.size() != expected_size )
                    throw std::runtime_error( to_string()
                                              << "READ_TABLE (0x" << std::hex << table_id
                                              << std::dec << ") data size received= " << data.size()
                                              << " (expected " << expected_size << ")" );
                if( pheader )
                    *pheader = *(table_header *)data.data();
                if( ptable )
                    *ptable = *(T *)(data.data() + sizeof( table_header ));
                break;

            case hwm_TableIsEmpty:
                if( init )
                {
                    // Initialize a new table
                    init();
                    break;
                }
                // fall-thru!
                
            default:
                LOG_DEBUG( "Failed to read FW table 0x" << std::hex << table_id );
                command cmd( fw_cmd::READ_TABLE, table_id );
                throw invalid_value_exception( hwmon_error_string( cmd, response ) );
            }
        }

        // Write a table to firmware
        template< typename T >
        void write_fw_table( hw_monitor& hwm, uint16_t const table_id, T const & table, uint16_t const version = 0x0100 )
        {
            command cmd( fw_cmd::WRITE_TABLE, 0 );
            cmd.data.resize( sizeof( table_header ) + sizeof( table ) );

            table_header * h = (table_header *)cmd.data.data();
            h->major = version >> 8;
            h->minor = version & 0xFF;
            h->table_id = table_id;
            h->table_size = sizeof( T );
            h->reserved = 0xFFFFFFFF;
            h->crc32 = calc_crc32( (byte *)&table, sizeof( table ) );

            memcpy( cmd.data.data() + sizeof( table_header ), &table, sizeof( table ) );
            
            hwmon_response response;
            hwm.send( cmd, &response );
            switch( response )
            {
            case hwm_Success:
                break;

            default:
                LOG_DEBUG( "Failed to write FW table 0x" << std::hex << table_id << " " << sizeof( table ) << " bytes: " );
                throw invalid_value_exception( to_string() << "Failed to write FW table 0x" << std::hex << table_id << ": " << hwmon_error_string( cmd, response ));
            }
        }

        template< typename T >
        void read_fw_register(hw_monitor& hwm, T * preg, int const baseline_address )
        {
            command cmd( ivcam2::fw_cmd::MRD, baseline_address, baseline_address + sizeof( T ) );
            auto res = hwm.send( cmd );
            if( res.size() != sizeof( T ) )
                throw std::runtime_error( to_string()
                                          << "MRD data size received= " << res.size()
                                          << " (expected " << sizeof( T ) << ")" );
            if( preg )
                *preg = *(T *)res.data();
        }

#pragma pack(push, 1)
        struct ac_depth_results  // aka "Algo_AutoCalibration" in FW
        {
            static const int table_id = 0x240;
            static const uint16_t this_version = (RS2_API_MAJOR_VERSION << 12 | RS2_API_MINOR_VERSION << 4 | RS2_API_PATCH_VERSION);

            rs2_dsm_params params;

            ac_depth_results() {}
            ac_depth_results( rs2_dsm_params const & dsm_params ) : params( dsm_params ) {}
        };

        struct rgb_calibration_table
        {
            static const uint16_t table_id = 0x310;        // in flash
            static const uint16_t eeprom_table_id = 0x10;  // factory calibration - read-only

            uint16_t version;
            uint16_t type;
            uint32_t timestamp;
            uint16_t width;
            uint16_t height;
            uint16_t h_offset;
            uint16_t v_offset;
            struct /*intrinsics*/
            {
                float fx;  // focal length in X, normalize by [-1 1]
                float fy;  // focal length in Y, normalize by [-1 1]
                float px;  // Principal point in x, normalize by [-1 1]
                float py;  // Principal point in x, normalize by [-1 1]
                float sheer;
                float d[5];  // RGB forward distortion parameters (k1, k2, p1, p2, k3), brown model
            } intr;
            rs2_extrinsics extr;
            byte reserved[8];

            void set_intrinsics( rs2_intrinsics const & );
            rs2_intrinsics get_intrinsics() const;
            rs2_extrinsics const & get_extrinsics() const { return extr; }
            void update_write_fields();
        };
#pragma pack(pop)

        // <Command Name="GVD" Opcode="0x10" Description="Get Version and Date">
        // See CommandsIVCAM2.xml for complete up-to-date fields
        enum gvd_fields
        {
            is_camera_locked_offset = 6,    // "eyeSafety": encompasses eeprom, flash, & registers
            fw_version_offset = 12,         // "FunctionalPayloadVersion"
            module_serial_offset = 60,      // "OpticalHeadModuleSN" -> RS2_CAMERA_INFO_SERIAL_NUMBER
            module_asic_serial_offset = 74  // "AsicModuleSerial" -> RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER & RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID
        };

        enum gvd_fields_size
        {
            // Keep sorted
            module_serial_size = 4,
            module_asic_serial_size = 6
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { L500_RECOVERY_PID,            "Intel RealSense L5xx Recovery"},
            { L535_RECOVERY_PID,            "Intel RealSense L5xx Recovery"},
            { L500_USB2_RECOVERY_PID_OLD,   "Intel RealSense L5xx Recovery"},
            { L500_PID,                     "Intel RealSense L500"},
            { L515_PID_PRE_PRQ,             "Intel RealSense L515 (pre-PRQ)"},
            { L515_PID,                     "Intel RealSense L515"},
            { L535_PID,                     "Intel RealSense L535"},

        };

        // Known FW error codes, if we poll for errors (RS2_OPTION_ERROR_POLLING_ENABLED)
        enum l500_notifications_types
        {
            success = 0,
            depth_not_available,
            overflow_infrared,
            overflow_depth,
            overflow_confidence,
            depth_stream_hard_error,
            depth_stream_soft_error,
            temp_warning,
            temp_critical,
            DFU_error,
            fall_detected = 12,
            ld_alarm = 14,
            hard_error = 15,
            ld_alarm_hard_error = 16,
            pzr_vbias_exceed_limit = 17,
            eye_safety_general_critical_error = 18,
            eye_safety_stuck_at_ld_error = 19,
            eye_safety_stuck_at_mode_sign_error = 20,
            eye_safety_stuck_at_vbst_error = 21,
            eye_safety_stuck_at_msafe_error = 22,
            eye_safety_stuck_at_ldd_snubber_error = 23,
            eye_safety_stuck_at_flash_otp_error = 24
        };

        // Each of the above is mapped to a string -- but only for those we identify as errors: warnings are
        // listed below as comments and are treated as unknown warnings...
        // NOTE: a unit-test in func/hw-errors/ directly uses this map and tests it
        const std::map< uint8_t, std::string > l500_fw_error_report = {
            { success,                      "Success" },
            { depth_not_available,          "Fatal error occur and device is unable \nto run depth stream" },
            { overflow_infrared,            "Overflow occur on infrared stream" },
            { overflow_depth,               "Overflow occur on depth stream" },
            { overflow_confidence,          "Overflow occur on confidence stream" },
            { depth_stream_hard_error,      "Receiver light saturation, stream stopped for 1 sec" },
            { depth_stream_soft_error,      "Error that may be overcome in few sec. \nStream stoped. May be recoverable" },
            { temp_warning,                 "Warning, temperature close to critical" },
            { temp_critical,                "Critical temperature reached" },
            { DFU_error,                    "DFU error" },
            //{10 ,                         "L500 HW report - unresolved type 10"},
            //{11 ,                         "L500 HW report - unresolved type 11"},
            { fall_detected,                "Fall detected stream stopped"  },
            //{13 ,                         "L500 HW report - unresolved type 13"},
            { ld_alarm,                     "Fatal error occurred (14)" },
            { hard_error,                   "Fatal error occurred (15)" },
            { ld_alarm_hard_error,          "Fatal error occurred (16)" },
            { pzr_vbias_exceed_limit,       "Fatal error occurred (17)" },
            { eye_safety_general_critical_error,        "Fatal error occurred (18)" },
            { eye_safety_stuck_at_ld_error,             "Fatal error occurred (19)" },
            { eye_safety_stuck_at_mode_sign_error,      "Fatal error occurred (20)" },
            { eye_safety_stuck_at_vbst_error,           "Fatal error occurred (21)" },
            { eye_safety_stuck_at_msafe_error,          "Fatal error occurred (22)" },
            { eye_safety_stuck_at_ldd_snubber_error,    "Fatal error occurred (23)" },
            { eye_safety_stuck_at_flash_otp_error,      "Fatal error occurred (24)" }
        };

#pragma pack(push, 1)
        struct pinhole_model
        {
            float2 focal_length;
            float2 principal_point;
        };

        struct distortion
        {
            float radial_k1;
            float radial_k2;
            float tangential_p1;
            float tangential_p2;
            float radial_k3;
        };

        struct pinhole_camera_model
        {
            uint32_t width;
            uint32_t height;
            pinhole_model ipm;
            distortion distort;
        };

        struct intrinsic_params
        {
            pinhole_camera_model pinhole_cam_model; //(Same as standard intrinsic)
            float2 zo;
            float znorm;
        };

        struct intrinsic_per_resolution
        {
            intrinsic_params raw;
            intrinsic_params world;
        };

        struct resolutions_depth
        {
            uint16_t reserved16;
            uint8_t reserved8;
            uint8_t num_of_resolutions;
            intrinsic_per_resolution intrinsic_resolution[MAX_NUM_OF_DEPTH_RESOLUTIONS]; //Dynamic number of entries according to num of resolutions
        };

        struct orientation
        {
            uint8_t hscan_direction;
            uint8_t vscan_direction;
            uint16_t reserved16;
            uint32_t reserved32;
            float depth_offset;
        };

        struct intrinsic_depth
        {
            orientation orient;
            resolutions_depth resolution;
        };

        struct resolutions_rgb
        {
            uint16_t reserved16;
            uint8_t reserved8;
            uint8_t num_of_resolutions;
            pinhole_camera_model intrinsic_resolution[MAX_NUM_OF_RGB_RESOLUTIONS]; //Dynamic number of entries according to num of resolutions
        };

        struct rgb_common
        {
            float sheer;
            uint32_t reserved32;
        };

        struct intrinsic_rgb
        {
            rgb_common common;
            resolutions_rgb resolution;
        };

        struct temperatures {
            double LDD_temperature;  // Laser Diode Driver
            double MC_temperature;
            double MA_temperature;
            double APD_temperature;
            double HUM_temperature;
            double AlgoTermalLddAvg_temperature;

            temperatures()
                : LDD_temperature(0.)
                , MC_temperature(0.)
                , MA_temperature(0.)
                , APD_temperature(0.)
                , HUM_temperature(0.)
                , AlgoTermalLddAvg_temperature(0.) { }
        };

        //FW versions >= 1.5.0.0 added to the response vector the nest AVG value
        struct extended_temperatures : public temperatures 
        {
            double nest_avg; // NEST = Noise Estimation

            extended_temperatures() : temperatures(), nest_avg(.0) {}
        };
#pragma pack( pop )

        rs2_extrinsics get_color_stream_extrinsic(const std::vector<uint8_t>& raw_data);
        
        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
        const platform::uvc_device_info& info, platform::usb_device_info& result);

        class l500_temperature_options : public readonly_option
        {
        public:
            float query() const override;

            option_range get_range() const override { return option_range{ 0, 100, 0, 0 }; }

            bool is_enabled() const override { return true; }

            const char * get_description() const override { return _description.c_str(); }

            explicit l500_temperature_options(l500_device *l500_depth_dev,
                                               rs2_option opt,
                                               const std::string& description );

        private:
            rs2_option _option;
            l500_device *_l500_depth_dev;
            std::string _description;
        };

        // Noise estimation option
        class nest_option : public readonly_option
        {
        public:
            float query() const override;

            option_range get_range() const override { return option_range{ 0, 4100, 0, 0 }; }

            bool is_enabled() const override { return true; }

            const char * get_description() const override { return _description.c_str(); }

            explicit nest_option(l500_device * l500_depth_dev, const std::string & description)
                : _l500_depth_dev(l500_depth_dev)
                , _description(description) {};

        private:
            l500_device *_l500_depth_dev;
            std::string _description;
        };

        class l500_timestamp_reader : public frame_timestamp_reader
        {
            static const int pins = 3;
            mutable std::vector<size_t> counter;
            std::shared_ptr<platform::time_service> _ts;
            mutable std::recursive_mutex _mtx;
        public:
            l500_timestamp_reader(std::shared_ptr<platform::time_service> ts)
                : counter(pins), _ts(ts)
            {
                reset();
            }

            void reset() override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                for (size_t i = 0; i < pins; ++i)
                {
                    counter[i] = 0;
                }
            }

            rs2_time_t get_frame_timestamp(const std::shared_ptr<frame_interface>&) override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                return _ts->get_time();
            }

            unsigned long long get_frame_counter(const std::shared_ptr<frame_interface>& frame) const override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                size_t pin_index = 0;
                if (frame->get_stream()->get_format() == RS2_FORMAT_Z16)
                    pin_index = 1;
                else if (frame->get_stream()->get_stream_type() == RS2_STREAM_CONFIDENCE)
                    pin_index = 2;

                return ++counter[pin_index];
            }

            rs2_timestamp_domain get_frame_timestamp_domain(const std::shared_ptr<frame_interface>&) const override
            {
                return RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
            }
        };

        class l500_timestamp_reader_from_metadata : public frame_timestamp_reader
        {
            std::unique_ptr<l500_timestamp_reader> _backup_timestamp_reader;
            bool one_time_note;
            mutable std::recursive_mutex _mtx;
            arithmetic_wraparound<uint32_t, uint64_t > ts_wrap;

        protected:

            bool has_metadata_ts(const std::shared_ptr<frame_interface>& frame) const
            {
                // Metadata support for a specific stream is immutable
                auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
                const bool has_md_ts = [&] { std::lock_guard<std::recursive_mutex> lock(_mtx);
                return ((f->additional_data.metadata_size >= platform::uvc_header_size) && ((byte*)f->additional_data.metadata_blob.data())[0] >= platform::uvc_header_size);
                }();

                return has_md_ts;
            }

            bool has_metadata_fc(const std::shared_ptr<frame_interface>& frame) const
            {
                // Metadata support for a specific stream is immutable
                auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
                const bool has_md_frame_counter = [&] { std::lock_guard<std::recursive_mutex> lock(_mtx);
                return ((f->additional_data.metadata_size > platform::uvc_header_size) && ((byte*)f->additional_data.metadata_blob.data())[0] > platform::uvc_header_size);
                }();

                return has_md_frame_counter;
            }

        public:
            l500_timestamp_reader_from_metadata(std::shared_ptr<platform::time_service> ts) :_backup_timestamp_reader(nullptr), one_time_note(false)
            {
                _backup_timestamp_reader = std::unique_ptr<l500_timestamp_reader>(new l500_timestamp_reader(ts));
                reset();
            }

            rs2_time_t get_frame_timestamp(const std::shared_ptr<frame_interface>& frame) override;

            unsigned long long get_frame_counter(const std::shared_ptr<frame_interface>& frame) const override;

            void reset() override;

            rs2_timestamp_domain get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const override;
        };

        /* For RS2_OPTION_FREEFALL_DETECTION_ENABLED */
        class freefall_option : public bool_option
        {
        public:
            freefall_option( hw_monitor& hwm, bool enabled = true );

            bool is_enabled() const override { return _enabled; }
            virtual void enable( bool = true );

            virtual void set( float value ) override;
            virtual const char * get_description() const override
            {
                return "When enabled (default), the sensor will turn off if a free-fall is detected";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
            hw_monitor & _hwm;
            bool _enabled;
        };

        /*  For RS2_OPTION_INTER_CAM_SYNC_MODE
            Not an advanced control: always off after camera startup (reset).
            When enabled, the freefall control should turn off.
        */
        class hw_sync_option : public bool_option
        {
        public:
            hw_sync_option( hw_monitor& hwm, std::shared_ptr< freefall_option > freefall_opt );

            virtual void set( float value ) override;
            virtual const char* get_description() const override
            {
                return "Enable multi-camera hardware synchronization mode (disabled on startup); not compatible with free-fall detection";
            }
            virtual void enable_recording( std::function<void( const option& )> record_action ) override { _record_action = record_action; }

        private:
            std::function<void( const option& )> _record_action = []( const option& ) {};
            hw_monitor& _hwm;
            std::shared_ptr< freefall_option > _freefall_opt;
        };

        rs2_sensor_mode get_resolution_from_width_height(int width, int height);

        // Helper function that should be used when multiple FW calls needs to be made.
        // This function change the USB power to D0 (Operational) using the invoke_power function
        // activate the received function and power down the state to D3 (Idle)
        template<class T>
        auto group_multiple_fw_calls(synthetic_sensor &s, T action)
            -> decltype(action())
        {
            auto &us =  dynamic_cast<uvc_sensor&>(*s.get_raw_sensor());
            
            return us.invoke_powered([&](platform::uvc_device& dev) { return action(); });
        }

        class ac_trigger;
    } // librealsense::ivcam2
} // namespace librealsense
