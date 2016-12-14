// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.
#include <iomanip>      // for std::put_time

#include "hw-monitor.h"
#include "ds-private.h"

#pragma pack(push, 1) // All structs in this file are byte-aligned

enum class command : uint32_t // Command/response codes
{
    peek               = 0x11,
    poke               = 0x12,
    download_spi_flash = 0x1A,
    get_fwrevision     = 0x21
};

enum class command_modifier : uint32_t { direct = 0x10 }; // Command/response modifiers

#define SPI_FLASH_PAGE_SIZE_IN_BYTES                0x100
#define SPI_FLASH_SECTOR_SIZE_IN_BYTES              0x1000
#define SPI_FLASH_SIZE_IN_SECTORS                   256
#define SPI_FLASH_TOTAL_SIZE_IN_BYTES               (SPI_FLASH_SIZE_IN_SECTORS * SPI_FLASH_SECTOR_SIZE_IN_BYTES)
#define SPI_FLASH_PAGES_PER_SECTOR                  (SPI_FLASH_SECTOR_SIZE_IN_BYTES / SPI_FLASH_PAGE_SIZE_IN_BYTES)
#define SPI_FLASH_SECTORS_RESERVED_FOR_FIRMWARE     160
#define NV_NON_FIRMWARE_START                       (SPI_FLASH_SECTORS_RESERVED_FOR_FIRMWARE * SPI_FLASH_SECTOR_SIZE_IN_BYTES)
#define NV_ADMIN_DATA_N_ENTRIES                     9
#define NV_CALIBRATION_DATA_ADDRESS_INDEX           0
#define NV_NON_FIRMWARE_ROOT_ADDRESS                NV_NON_FIRMWARE_START
#define CAM_INFO_BLOCK_LEN 2048

namespace rsimpl {
    namespace ds
    {

        void xu_read(const uvc::device & device, uvc::extension_unit xu, control xu_ctrl, void * buffer, uint32_t length)
        {
            uvc::get_control_with_retry(device, xu, static_cast<int>(xu_ctrl), buffer, length);
        }

        void xu_write(uvc::device & device, uvc::extension_unit xu, control xu_ctrl, void * buffer, uint32_t length)
        {
            uvc::set_control_with_retry(device, xu, static_cast<int>(xu_ctrl), buffer, length);
        }

        struct CommandResponsePacket
        {
            command code; command_modifier modifier;
            uint32_t tag, address, value, reserved[59];
            CommandResponsePacket() { std::memset(this, 0, sizeof(CommandResponsePacket)); }
            CommandResponsePacket(command code, uint32_t address = 0, uint32_t value = 0) : code(code), modifier(command_modifier::direct), tag(12), address(address), value(value)
            {
                std::memset(reserved, 0, sizeof(reserved));
            }
        };

        CommandResponsePacket send_command_and_receive_response(uvc::device & device, const CommandResponsePacket & command)
        {
            CommandResponsePacket c = command, r;
            set_control(device, lr_xu, static_cast<int>(control::command_response), &c, sizeof(c));
            get_control(device, lr_xu, static_cast<int>(control::command_response), &r, sizeof(r));
            return r;
        }

        void bulk_usb_command(uvc::device & device, std::timed_mutex & mutex, unsigned char out_ep, uint8_t *out, size_t outSize, uint32_t & op, unsigned char in_ep, uint8_t * in, size_t & inSize, int timeout)
        {
            // write
            errno = 0;

            int outXfer;

            if (!mutex.try_lock_for(std::chrono::milliseconds(timeout))) throw std::runtime_error("timed_mutex::try_lock_for(...) timed out");
            std::lock_guard<std::timed_mutex> guard(mutex, std::adopt_lock);

            bulk_transfer(device, out_ep, out, (int)outSize, &outXfer, timeout); // timeout in ms

            // read
            if (in && inSize)
            {
                uint8_t buf[1024];  // TBD the size may vary

                errno = 0;

                bulk_transfer(device, in_ep, buf, sizeof(buf), &outXfer, timeout);
                if (outXfer < (int)sizeof(uint32_t)) throw std::runtime_error("incomplete bulk usb transfer");

                op = *(uint32_t *)buf;
                if (outXfer > (int)inSize) throw std::runtime_error("bulk transfer failed - user buffer too small");
                inSize = outXfer;
                memcpy(in, buf, inSize);
            }
        }

        bool read_device_pages(uvc::device & dev, uint32_t address, unsigned char * buffer, uint32_t nPages)
        {
            int addressTest = SPI_FLASH_TOTAL_SIZE_IN_BYTES - address - nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

            if (!nPages || addressTest < 0)
                return false;

            // This command allows the host to read a block of data from the SPI flash.
            // Once this command is processed by the DS4, further command messages will be treated as SPI data
            // and therefore will be read from flash. The size of the SPI data must be a multiple of 256 bytes.
            // This will repeat until the number of bytes specified in the ‘value’ field of the original command
            // message has been read.  At that point the DS4 will process command messages as expected.

            send_command_and_receive_response(dev, CommandResponsePacket(command::download_spi_flash, address, nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES));

            uint8_t *p = buffer;
            uint16_t spiLength = SPI_FLASH_PAGE_SIZE_IN_BYTES;
            for (unsigned int i = 0; i < nPages; ++i)
            {
                xu_read(dev, lr_xu, control::command_response, p, spiLength);
                p += SPI_FLASH_PAGE_SIZE_IN_BYTES;
            }
            return true;
        }

        void read_arbitrary_chunk(uvc::device & dev, uint32_t address, void * dataIn, int lengthInBytesIn)
        {
            unsigned char * data = (unsigned char *)dataIn;
            int lengthInBytes = lengthInBytesIn;
            unsigned char page[SPI_FLASH_PAGE_SIZE_IN_BYTES];
            int nPagesToRead;
            uint32_t startAddress = address;
            if (startAddress & 0xff)
            {
                // we are not on a page boundary
                startAddress = startAddress & ~0xff;
                uint32_t startInPage = address - startAddress;
                uint32_t lengthToCopy = SPI_FLASH_PAGE_SIZE_IN_BYTES - startInPage;
                if (lengthToCopy > (uint32_t)lengthInBytes)
                    lengthToCopy = lengthInBytes;
                read_device_pages(dev, startAddress, page, 1);
                memcpy(data, page + startInPage, lengthToCopy);
                lengthInBytes -= lengthToCopy;
                data += lengthToCopy;
                startAddress += SPI_FLASH_PAGE_SIZE_IN_BYTES;
            }

            nPagesToRead = lengthInBytes / SPI_FLASH_PAGE_SIZE_IN_BYTES;

            if (nPagesToRead > 0)
                read_device_pages(dev, startAddress, data, nPagesToRead);

            lengthInBytes -= (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);

            if (lengthInBytes)
            {
                // means we still have a remainder
                data += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
                startAddress += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
                read_device_pages(dev, startAddress, page, 1);
                memcpy(data, page, lengthInBytes);
            }
        }

        bool read_admin_sector(uvc::device & dev, unsigned char data[SPI_FLASH_SECTOR_SIZE_IN_BYTES], int whichAdminSector)
        {
            uint32_t adminSectorAddresses[NV_ADMIN_DATA_N_ENTRIES];

            read_arbitrary_chunk(dev, NV_NON_FIRMWARE_ROOT_ADDRESS, adminSectorAddresses, NV_ADMIN_DATA_N_ENTRIES * sizeof(adminSectorAddresses[0]));

            if (whichAdminSector >= 0 && whichAdminSector < NV_ADMIN_DATA_N_ENTRIES)
            {
                uint32_t pageAddressInBytes = adminSectorAddresses[whichAdminSector];
                return read_device_pages(dev, pageAddressInBytes, data, SPI_FLASH_PAGES_PER_SECTOR);
            }

            return false;
        }

        ds_calibration read_calibration_and_rectification_parameters(const uint8_t(&flash_data_buffer)[SPI_FLASH_SECTOR_SIZE_IN_BYTES])
        {
            struct RectifiedIntrinsics
            {
                big_endian<float> rfx, rfy;
                big_endian<float> rpx, rpy;
                big_endian<uint32_t> rw, rh;
                operator rs_intrinsics () const { return{ (int)rw, (int)rh, rpx, rpy, rfx, rfy, RS_DISTORTION_NONE, {0,0,0,0,0} }; }
            };

            ds_calibration cameraCalib = {};
            cameraCalib.version = reinterpret_cast<const big_endian<uint32_t> &>(flash_data_buffer);
            if (cameraCalib.version == 0)
            {
                struct UnrectifiedIntrinsicsV0
                {
                    big_endian<float> fx, fy;
                    big_endian<float> px, py;
                    big_endian<double> k[5];
                    big_endian<uint32_t> w, h;
                    operator rs_intrinsics () const { return{ (int)w, (int)h, px, py, fx, fy, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {(float)k[0],(float)k[1],(float)k[2],(float)k[3],(float)k[4]} }; }
                };

                struct CameraCalibrationParametersV0
                {
                    enum { MAX_INTRIN_RIGHT = 2 };      ///< Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
                    enum { MAX_INTRIN_THIRD = 3 };      ///< Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
                    enum { MAX_MODES_LR = 4 };    ///< Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
                    enum { MAX_MODES_THIRD = 4 }; ///< Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, 640x480 and 320x240)

                    big_endian<uint32_t> versionNumber;
                    big_endian<uint16_t> numIntrinsicsRight;     ///< Number of right cameras < MAX_INTRIN_RIGHT_V0
                    big_endian<uint16_t> numIntrinsicsThird;     ///< Number of native resolutions of third camera < MAX_INTRIN_THIRD_V0
                    big_endian<uint16_t> numRectifiedModesLR;    ///< Number of rectified LR resolution modes < MAX_MODES_LR_V0
                    big_endian<uint16_t> numRectifiedModesThird; ///< Number of rectified Third resolution modes < MAX_MODES_THIRD_V0

                    UnrectifiedIntrinsicsV0 intrinsicsLeft;
                    UnrectifiedIntrinsicsV0 intrinsicsRight[MAX_INTRIN_RIGHT];
                    UnrectifiedIntrinsicsV0 intrinsicsThird[MAX_INTRIN_THIRD];

                    RectifiedIntrinsics modesLR[MAX_INTRIN_RIGHT][MAX_MODES_LR];
                    RectifiedIntrinsics modesThird[MAX_INTRIN_RIGHT][MAX_INTRIN_THIRD][MAX_MODES_THIRD];

                    big_endian<double> Rleft[MAX_INTRIN_RIGHT][9];
                    big_endian<double> Rright[MAX_INTRIN_RIGHT][9];
                    big_endian<double> Rthird[MAX_INTRIN_RIGHT][9];

                    big_endian<float> B[MAX_INTRIN_RIGHT];
                    big_endian<float> T[MAX_INTRIN_RIGHT][3];

                    big_endian<double> Rworld[9];
                    big_endian<float> Tworld[3];
                };

                const auto & calib = reinterpret_cast<const CameraCalibrationParametersV0 &>(flash_data_buffer);
                for (int i = 0; i < 3; ++i) cameraCalib.modesLR[i] = calib.modesLR[0][i];
                for (int i = 0; i < 2; ++i)
                {
                    cameraCalib.intrinsicsThird[i] = calib.intrinsicsThird[i];
                    for (int j = 0; j < 2; ++j) cameraCalib.modesThird[i][j] = calib.modesThird[0][i][j];
                }
                for (int i = 0; i < 9; ++i) cameraCalib.Rthird[i] = static_cast<float>(calib.Rthird[0][i]);
                for (int i = 0; i < 3; ++i) cameraCalib.T[i] = calib.T[0][i];
                cameraCalib.B = calib.B[0];
            }
            else if (cameraCalib.version == 1 || cameraCalib.version == 2)
            {
                struct UnrectifiedIntrinsicsV2
                {
                    big_endian<float> fx, fy;
                    big_endian<float> px, py;
                    big_endian<float> k[5];
                    big_endian<uint32_t> w, h;
                    operator rs_intrinsics () const { return{ (int)w, (int)h, px, py, fx, fy, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {k[0],k[1],k[2],k[3],k[4]} }; }
                };

                struct CameraCalibrationParametersV2
                {
                    enum { MAX_INTRIN_RIGHT = 2 }; // Max number right cameras supported (e.g. one or two, two would support a multi-baseline unit)
                    enum { MAX_INTRIN_THIRD = 3 }; // Max number native resolutions the third camera can have (e.g. 1920x1080 and 640x480)
                    enum { MAX_INTRIN_PLATFORM = 4 }; // Max number native resolutions the platform camera can have
                    enum { MAX_MODES_LR = 4 }; // Max number rectified LR resolution modes the structure supports (e.g. 640x480, 492x372 and 332x252)
                    enum { MAX_MODES_THIRD = 3 }; // Max number rectified Third resolution modes the structure supports (e.g. 1920x1080, 1280x720, etc)
                    enum { MAX_MODES_PLATFORM = 1 }; // Max number rectified Platform resolution modes the structure supports

                    big_endian<uint32_t> versionNumber;
                    big_endian<uint16_t> numIntrinsicsRight;
                    big_endian<uint16_t> numIntrinsicsThird;
                    big_endian<uint16_t> numIntrinsicsPlatform;
                    big_endian<uint16_t> numRectifiedModesLR;
                    big_endian<uint16_t> numRectifiedModesThird;
                    big_endian<uint16_t> numRectifiedModesPlatform;

                    UnrectifiedIntrinsicsV2 intrinsicsLeft;
                    UnrectifiedIntrinsicsV2 intrinsicsRight[MAX_INTRIN_RIGHT];
                    UnrectifiedIntrinsicsV2 intrinsicsThird[MAX_INTRIN_THIRD];
                    UnrectifiedIntrinsicsV2 intrinsicsPlatform[MAX_INTRIN_PLATFORM];

                    RectifiedIntrinsics modesLR[MAX_INTRIN_RIGHT][MAX_MODES_LR];
                    RectifiedIntrinsics modesThird[MAX_INTRIN_RIGHT][MAX_INTRIN_THIRD][MAX_MODES_THIRD];
                    RectifiedIntrinsics modesPlatform[MAX_INTRIN_RIGHT][MAX_INTRIN_PLATFORM][MAX_MODES_PLATFORM];

                    big_endian<float> Rleft[MAX_INTRIN_RIGHT][9];
                    big_endian<float> Rright[MAX_INTRIN_RIGHT][9];
                    big_endian<float> Rthird[MAX_INTRIN_RIGHT][9];
                    big_endian<float> Rplatform[MAX_INTRIN_RIGHT][9];

                    big_endian<float> B[MAX_INTRIN_RIGHT];
                    big_endian<float> T[MAX_INTRIN_RIGHT][3];
                    big_endian<float> Tplatform[MAX_INTRIN_RIGHT][3];

                    big_endian<float> Rworld[9];
                    big_endian<float> Tworld[3];
                };

                const auto & calib = reinterpret_cast<const CameraCalibrationParametersV2 &>(flash_data_buffer);
                for (int i = 0; i < 3; ++i) cameraCalib.modesLR[i] = calib.modesLR[0][i];
                for (int i = 0; i < 2; ++i)
                {
                    cameraCalib.intrinsicsThird[i] = calib.intrinsicsThird[i];
                    for (int j = 0; j < 2; ++j) cameraCalib.modesThird[i][j] = calib.modesThird[0][i][j];
                }
                for (int i = 0; i < 9; ++i) cameraCalib.Rthird[i] = calib.Rthird[0][i];
                for (int i = 0; i < 3; ++i) cameraCalib.T[i] = calib.T[0][i];
                cameraCalib.B = calib.B[0];
            }
            else
            {
                throw std::runtime_error(to_string() << "Unsupported calibration version: " << cameraCalib.version);
            }

            return cameraCalib;
        }

        ds_head_content read_camera_head_contents(const uint8_t(&flash_data_buffer)[SPI_FLASH_SECTOR_SIZE_IN_BYTES], uint32_t & serial_number)
        {
            //auto header = reinterpret_cast<const CameraHeadContents &>(flash_data_buffer[CAM_INFO_BLOCK_LEN]);
            ds_head_content head_content = reinterpret_cast<const ds_head_content &>(flash_data_buffer[CAM_INFO_BLOCK_LEN]);

            serial_number = head_content.serial_number;

            LOG_INFO("Serial number                       = " << head_content.serial_number);
            LOG_INFO("Model number                        = " << head_content.imager_model_number);
            LOG_INFO("Revision number                     = " << head_content.module_revision_number);
            LOG_INFO("Camera head contents version        = " << head_content.camera_head_contents_version);
            if (head_content.camera_head_contents_version != ds_head_content::DS_HEADER_VERSION_NUMBER) LOG_WARNING("Camera head contents version != 12, data may be missing/incorrect");
            LOG_INFO("Module version                      = " << (int)head_content.module_version << "." << (int)head_content.module_major_version << "." << (int)head_content.module_minor_version << "." << (int)head_content.module_skew_version);
            LOG_INFO("OEM ID                              = " << head_content.oem_id);
            LOG_INFO("Lens type for left/right imagers    = " << head_content.lens_type);
            LOG_INFO("Lens type for third imager          = " << head_content.lens_type_third_imager);
            LOG_INFO("Lens coating for left/right imagers = " << head_content.lens_coating_type);
            LOG_INFO("Lens coating for third imager       = " << head_content.lens_coating_type_third_imager);
            LOG_INFO("Nominal baseline (left to right)    = " << head_content.nominal_baseline << " mm");
            LOG_INFO("Nominal baseline (left to third)    = " << head_content.nominal_baseline_third_imager << " mm");
            LOG_INFO("Built on "        << time_to_string(head_content.build_date)          << " UTC");
            LOG_INFO("Calibrated on "   << time_to_string(head_content.calibration_date)    << " UTC");
            return head_content;
        }

        ds_info read_camera_info(uvc::device & device)
        {
            uint8_t flashDataBuffer[SPI_FLASH_SECTOR_SIZE_IN_BYTES];
            if (!read_admin_sector(device, flashDataBuffer, NV_CALIBRATION_DATA_ADDRESS_INDEX)) throw std::runtime_error("Could not read calibration sector");

            ds_info cam_info = {};

            try
            {
                cam_info.calibration = read_calibration_and_rectification_parameters(flashDataBuffer);
                cam_info.head_content = read_camera_head_contents(flashDataBuffer, cam_info.calibration.serial_number);
            }
            catch (std::runtime_error &err){ LOG_ERROR("Failed to read camera info " << err.what()); }
            catch (...){ LOG_ERROR("Failed to read camera info, may not work correctly"); }

            return cam_info;
        }

        std::string read_firmware_version(uvc::device & device)
        {
            auto response = send_command_and_receive_response(device, CommandResponsePacket(command::get_fwrevision));
            return reinterpret_cast<const char *>(response.reserved);
        }

        std::string read_isp_firmware_version(uvc::device & device)
        {
            auto response = send_command_and_receive_response(device, CommandResponsePacket(command::get_fwrevision));
            return to_string() << "0x" << std::hex << response.reserved[4];
        }

        void set_stream_intent(uvc::device & device, uint8_t & intent)
        {
            xu_write(device, lr_xu, control::stream_intent, intent);
        }

        void get_stream_status(const uvc::device & device, int & status)
        {
            uint8_t s[4] = { 255, 255, 255, 255 };
            xu_read(device, lr_xu, control::status, s, sizeof(uint32_t));
            status = rsimpl::pack(s[0], s[1], s[2], s[3]);
        }

        void force_firmware_reset(uvc::device & device)
        {
            try
            {
                uint8_t reset = 1;
                xu_write(device, lr_xu, control::sw_reset, &reset, sizeof(uint8_t));
            }
            catch (...) {} // xu_write always throws during a control::SW_RESET, since the firmware is unable to send a proper response
        }

        bool get_emitter_state(const uvc::device & device, bool is_streaming, bool is_depth_enabled)
        {
            auto byte = xu_read<uint8_t>(device, lr_xu, control::emitter);
            if (is_streaming) return (byte & 1 ? true : false);
            else if (byte & 4) return (byte & 2 ? true : false);
            else return is_depth_enabled;
        }

        void set_emitter_state(uvc::device & device, bool state)
        {
            xu_write(device, lr_xu, control::emitter, uint8_t(state ? 1 : 0));
        }

        void get_register_value(uvc::device & device, uint32_t reg, uint32_t & value)
        {
            value = send_command_and_receive_response(device, CommandResponsePacket(command::peek, reg)).value;
        }
        void set_register_value(uvc::device & device, uint32_t reg, uint32_t value)
        {
            send_command_and_receive_response(device, CommandResponsePacket(command::poke, reg, value));
        }

        const dc_params dc_params::presets[] = {
            {5, 5, 192,  1,  512, 6, 24, 27,  7,   24}, // (DEFAULT) Default settings on chip. Similar to the medium setting and best for outdoors.
            {5, 5,   0,  0, 1023, 0,  0,  0,  0, 2047}, // (OFF) Disable almost all hardware-based outlier removal
            {5, 5, 115,  1,  512, 6, 18, 25,  3,   24}, // (LOW) Provide a depthmap with a lower number of outliers removed, which has minimal false negatives.
            {5, 5, 185,  5,  505, 6, 35, 45, 45,   14}, // (MEDIUM) Provide a depthmap with a medium number of outliers removed, which has balanced approach.
            {5, 5, 175, 24,  430, 6, 48, 47, 24,   12}, // (OPTIMIZED) Provide a depthmap with a medium/high number of outliers removed. Derived from an optimization function.
            {5, 5, 235, 27,  420, 8, 80, 70, 90,   12}, // (HIGH) Provide a depthmap with a higher number of outliers removed, which has minimal false positives.
        };

    } // namespace rsimpl::ds

    namespace zr300
    {
        //const uvc::extension_unit lr_xu = {0, 2, 1, {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}};

        const uvc::guid MOTION_MODULE_USB_DEVICE_GUID = { 0xC0B55A29, 0xD7B6, 0x436E,{ 0xA6, 0xEF, 0x2E, 0x76, 0xED, 0x0A, 0xBC, 0xA5 } };
        const unsigned short motion_module_interrupt_interface = 0x2; // endpint to pull sensors data continuously (interrupt transmit)

        uint8_t get_fisheye_external_trigger(const uvc::device & device)
        {
            return ds::xu_read<uint8_t>(device, fisheye_xu, ds::control::fisheye_xu_ext_trig);
        }

        void set_fisheye_external_trigger(uvc::device & device, uint8_t ext_trig)
        {
            ds::xu_write(device, fisheye_xu, ds::control::fisheye_xu_ext_trig, &ext_trig, sizeof(ext_trig));
        }

        uint8_t get_fisheye_strobe(const uvc::device & device)
        {
            return ds::xu_read<uint8_t>(device, fisheye_xu, ds::control::fisheye_xu_strobe);
        }

        void set_fisheye_strobe(uvc::device & device, uint8_t strobe)
        {
            ds::xu_write(device, fisheye_xu, ds::control::fisheye_xu_strobe, &strobe, sizeof(strobe));
        }

        uint16_t get_fisheye_exposure(const uvc::device & device)
        {
            return ds::xu_read<uint16_t>(device, fisheye_xu, ds::control::fisheye_exposure);
        }

        void set_fisheye_exposure(uvc::device & device, uint16_t exposure)
        {
            ds::xu_write(device, fisheye_xu, ds::control::fisheye_exposure, &exposure, sizeof(exposure));
        }

        void claim_motion_module_interface(uvc::device & device)
        {
            claim_aux_interface(device, MOTION_MODULE_USB_DEVICE_GUID, motion_module_interrupt_interface);
        }

    } // namespace rsimpl::zr300
}

#pragma pack(pop)
