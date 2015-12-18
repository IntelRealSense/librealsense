/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#include "r200-private.h"

#include <cstring>
#include <cmath>
#include <ctime>
#include <thread>

enum class control // UVC extension control codes
{
    command_response           = 1,
    iffley                     = 2,
    stream_intent              = 3,
    depth_units                = 4,
    min_max                    = 5,
    disparity                  = 6,
    rectification              = 7,
    emitter                    = 8,
    temperature                = 9,
    depth_params               = 10,
    last_error                 = 12,
    embedded_count             = 13,
    lr_exposure                = 14,
    lr_autoexposure_parameters = 15,
    sw_reset                   = 16,
    lr_gain                    = 17,
    lr_exposure_mode           = 18,
    disparity_shift            = 19,
    status                     = 20,
    lr_exposure_discovery      = 21,
    lr_gain_discovery          = 22,
    hw_timestamp               = 23,
};

enum class command : uint32_t // Command/response codes
{
    peek               = 0x11,
    poke               = 0x12,
    download_spi_flash = 0x1A,
    get_fwrevision     = 0x21,
};

enum class command_modifier : uint32_t { direct = 0x10 };

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

namespace rsimpl { namespace r200
{
    void xu_read(const uvc::device & device, control xu_ctrl, void * buffer, uint32_t length)
    {
        // Try reading an XU control, if it fails, retry several times
        for(int i=0; i<20; ++i)
        {
            try { get_control(device, 0, static_cast<int>(xu_ctrl), buffer, length); return; }
            catch(...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
        }
        get_control(device, 0, static_cast<int>(xu_ctrl), buffer, length);
    }

    void xu_write(uvc::device & device, control xu_ctrl, void * buffer, uint32_t length)
    {
        // Try writing an XU control, if it fails, retry several times
        for(int i=0; i<20; ++i)
        {
            try { set_control(device, 0, static_cast<int>(xu_ctrl), buffer, length); return; }
            catch(...) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
        }
        set_control(device, 0, static_cast<int>(xu_ctrl), buffer, length);
    }

    #pragma pack(push, 1)
    struct CommandPacket
    {
        command code;
        command_modifier modifier;
        uint32_t tag;
        uint32_t address;
        uint32_t value;
        uint32_t reserved[59];

        CommandPacket(command code, uint32_t address=0, uint32_t value=0) : code(code), modifier(command_modifier::direct), tag(12), address(address), value(value)
        {
            std::memset(reserved, 0, sizeof(reserved));
        }

    };

    struct ResponsePacket
    {
        uint32_t code;
        uint32_t modifier;
        uint32_t tag;
        uint32_t responseCode;
        uint32_t value;
        uint32_t revision[4];
        uint32_t reserved[55];

        ResponsePacket(uint32_t code = 0, uint32_t modifier = 0, uint32_t tag = 0, uint32_t responseCode = 0, uint32_t value = 0)
            : code(code), modifier(modifier), tag(tag), responseCode(responseCode), value(value)
        {
            std::memset(revision, 0, sizeof(revision));
            std::memset(reserved, 0, sizeof(reserved));
        }
    };
    #pragma pack(pop)

    void send_command(uvc::device & device, CommandPacket & command, ResponsePacket & response)
    {
        set_control(device, 0, static_cast<int>(control::command_response), &command, sizeof(command));
        get_control(device, 0, static_cast<int>(control::command_response), &response, sizeof(response));
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

        CommandPacket command(command::download_spi_flash, address);
        command.value = nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

        ResponsePacket response;

        send_command(dev, command, response);

        uint8_t *p = buffer;
        uint16_t spiLength = SPI_FLASH_PAGE_SIZE_IN_BYTES;
        for (unsigned int i = 0; i < nPages; ++i)
        {
            xu_read(dev, control::command_response, p, spiLength);
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

    void ReadCalibrationSector(uvc::device & deviceHandle, r200_calibration & cameraCalib, CameraHeaderInfo & cameraInfo)
    {
        uint8_t flashDataBuffer[SPI_FLASH_SECTOR_SIZE_IN_BYTES];

        if (!read_admin_sector(deviceHandle, flashDataBuffer, NV_CALIBRATION_DATA_ADDRESS_INDEX))
            throw std::runtime_error("Could not read calibration sector");

        memcpy(&cameraInfo, flashDataBuffer + CAM_INFO_BLOCK_LEN, sizeof(cameraInfo));

        #pragma pack(push, 1)
        struct RectifiedIntrinsics
        {
            big_endian<float> rfx, rfy;
            big_endian<float> rpx, rpy;
            big_endian<uint32_t> rw, rh;
            operator rs_intrinsics () const { return {(int)rw, (int)rh, rpx, rpy, rfx, rfy, RS_DISTORTION_NONE, {0,0,0,0,0}}; }
        };

        cameraCalib.version = reinterpret_cast<const big_endian<uint32_t> &>(flashDataBuffer);
        if(cameraCalib.version == 0)
        {
            struct UnrectifiedIntrinsicsV0
            {
                big_endian<float> fx, fy;
                big_endian<float> px, py;
                big_endian<double> k[5];
                big_endian<uint32_t> w, h;
                operator rs_intrinsics () const { return {(int)w, (int)h, px, py, fx, fy, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {k[0],k[1],k[2],k[3],k[4]}}; }
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

            const auto & calib = reinterpret_cast<const CameraCalibrationParametersV0 &>(flashDataBuffer);
            for(int i=0; i<3; ++i) cameraCalib.modesLR[i] = calib.modesLR[0][i];
            for(int i=0; i<2; ++i)
            {
                cameraCalib.intrinsicsThird[i] = calib.intrinsicsThird[i];
                for(int j=0; j<2; ++j) cameraCalib.modesThird[i][j] = calib.modesThird[0][i][j];
            }
            for(int i=0; i<9; ++i) cameraCalib.Rthird[i] = calib.Rthird[0][i];
            for(int i=0; i<3; ++i) cameraCalib.T[i] = calib.T[0][i];
            cameraCalib.B = calib.B[0];
        }
        else if(cameraCalib.version == 1 || cameraCalib.version == 2)
        {
            struct UnrectifiedIntrinsicsV2
            {
                big_endian<float> fx, fy;
                big_endian<float> px, py;
                big_endian<float> k[5];
                big_endian<uint32_t> w, h;
                operator rs_intrinsics () const { return {(int)w, (int)h, px, py, fx, fy, RS_DISTORTION_MODIFIED_BROWN_CONRADY, {k[0],k[1],k[2],k[3],k[4]}}; }
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

            const auto & calib = reinterpret_cast<const CameraCalibrationParametersV2 &>(flashDataBuffer);
            for(int i=0; i<3; ++i) cameraCalib.modesLR[i] = calib.modesLR[0][i];
            for(int i=0; i<2; ++i)
            {
                cameraCalib.intrinsicsThird[i] = calib.intrinsicsThird[i];
                for(int j=0; j<2; ++j) cameraCalib.modesThird[i][j] = calib.modesThird[0][i][j];
            }
            for(int i=0; i<9; ++i) cameraCalib.Rthird[i] = calib.Rthird[0][i];
            for(int i=0; i<3; ++i) cameraCalib.T[i] = calib.T[0][i];
            cameraCalib.B = calib.B[0];
        }
        else
        {
            throw std::runtime_error(to_string() << "Unsupported calibration version: " << cameraCalib.version);
        }
        #pragma pack(pop)
    }

    // Format a DSAPI timestamp in a human-readable fashion
    std::string unix_timestamp_to_human(double secondsSinceEpoch)
    {
        time_t time = (time_t)secondsSinceEpoch;
        char buffer[80];
        struct tm * pTime = gmtime(&time);
        if (pTime)
        {
            size_t i = strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&time));
            sprintf(buffer + i, ".%02d UTC", static_cast<int>(std::fmod(secondsSinceEpoch, 1.0) * 100));
            return buffer;
        }
        else
            return "";
    }

    void read_camera_info(uvc::device & device, r200_calibration & calib, CameraHeaderInfo & header)
    {
        ReadCalibrationSector(device, calib, header);

        LOG_INFO("Model: " << header.modelNumber);
        LOG_INFO("Firmware Version: " << read_firmware_version(device));
        LOG_INFO("Calibration Version: " << calib.version);
        LOG_INFO("Calibration Date: " << unix_timestamp_to_human(header.calibrationDate));
        LOG_INFO("Serial: " << header.serialNumber);
        LOG_INFO("Revision: " << header.revisionNumber);
        LOG_INFO("Camera Header Ver: " << header.cameraHeadContentsVersion);
        LOG_INFO("Baseline: " << header.nominalBaseline);
        LOG_INFO("OEM ID: " << header.OEMID);
        if (CURRENT_CAMERA_CONTENTS_VERSION_NUMBER != header.cameraHeadContentsVersion)
                LOG_WARNING("Device camera header does not match internal struct version: " << CURRENT_CAMERA_CONTENTS_VERSION_NUMBER);
    }

    std::string read_firmware_version(uvc::device & device)
    {
        CommandPacket command(command::get_fwrevision);

        ResponsePacket response;
        send_command(device, command, response);

        char fw[16];
        memcpy(fw, &response.revision, 16);
        return fw;
    }

    void set_stream_intent(uvc::device & device, uint8_t & intent)
    {
        xu_write(device, control::stream_intent, &intent, sizeof(intent));
    }

    void get_stream_status(const uvc::device & device, int & status)
    {
        uint8_t s[4] = {255, 255, 255, 255};
        xu_read(device, control::status, s, sizeof(uint32_t));
        status = rsimpl::pack(s[0], s[1], s[2], s[3]);
    }

    void get_last_error(const uvc::device & device, uint8_t & last_error)
    {
        xu_read(device, control::last_error, &last_error, sizeof(uint8_t));
    }

    void force_firmware_reset(uvc::device & device)
    {
        try
        {
            uint8_t reset = 1;
            xu_write(device, control::sw_reset, &reset, sizeof(uint8_t));
        }
        catch(...) {} // xu_write always throws during a control::SW_RESET, since the firmware is unable to send a proper response
    }

    void get_emitter_state(const uvc::device & device, bool is_streaming, bool is_depth_enabled, bool & state)
    {
        uint8_t byte = 0;
        xu_read(device, control::emitter, &byte, sizeof(byte));

        if(is_streaming) state = (byte & 1 ? true : false);
        else if(byte & 4) state = (byte & 2 ? true : false);
        else state = is_depth_enabled;
    }

    void set_emitter_state(uvc::device & device, bool state)
    {
        uint8_t newEmitterState = state ? 1 : 0;
        xu_write(device, control::emitter, &newEmitterState, sizeof(uint8_t));
    }

    void read_temperature(const uvc::device & device, int8_t & current, int8_t & min, int8_t & max, int8_t & min_fault)
    {
        uint8_t buf[4] = {0};
        xu_read(device, control::temperature, buf, sizeof(buf));
        current = buf[0];
        min = buf[1];
        max = buf[2];
        min_fault = buf[3];
    }

    void reset_temperature(uvc::device & device)
    {
        uint8_t buf[4] = {0};
        xu_write(device, control::temperature, buf, sizeof(buf));
    }

    void get_depth_units(const uvc::device & device, uint32_t & units)
    {
        xu_read(device, control::depth_units, &units, sizeof(units));
    }

    void set_depth_units(uvc::device & device, uint32_t units)
    {
        xu_write(device, control::depth_units, &units, sizeof(units));
    }

    void set_min_max_depth(uvc::device & device, uint16_t min_depth, uint16_t max_depth)
    {
        uint16_t values[] = {min_depth, max_depth};
        xu_write(device, control::min_max, values, sizeof(values));
    }

    void get_min_max_depth(const uvc::device & device, uint16_t & min_depth, uint16_t & max_depth)
    {
        uint16_t values[] = {0, 0};
        xu_read(device, control::min_max, values, sizeof(values));
        min_depth = values[0];
        max_depth = values[1];
    }

    void get_lr_gain(const uvc::device & device, uint32_t & rate, uint32_t & gain)
    {
        uint32_t values[] = {0, 0};
        xu_read(device, control::lr_gain, values, sizeof(values));
        rate = values[0];
        gain = values[1];
    }

    void set_lr_gain(uvc::device & device, uint32_t rate, uint32_t gain)
    {
        uint32_t values[] = {rate, gain};
        xu_write(device, control::lr_gain, values, sizeof(values));
    }

    void get_lr_exposure(const uvc::device & device, uint32_t & rate, uint32_t & exposure)
    {
        uint32_t values[] = {0, 0};
        xu_read(device, control::lr_exposure, values, sizeof(values));
        rate = values[0];
        exposure = values[1];
    }

    void set_lr_exposure(uvc::device & device, uint32_t rate, uint32_t exposure)
    {
        uint32_t values[] = {rate, exposure};
        xu_write(device, control::lr_exposure, values, sizeof(values));
    }

    void get_lr_auto_exposure_params(const uvc::device & device, auto_exposure_params & params)
    {
        xu_read(device, control::lr_autoexposure_parameters, &params, sizeof(params));
    }

    void set_lr_auto_exposure_params(uvc::device & device, auto_exposure_params params)
    {
        xu_write(device, control::lr_autoexposure_parameters, &params, sizeof(params));
    }

    void get_lr_exposure_mode(const uvc::device & device, uint32_t & mode)
    {
        uint8_t m; // 0 = EXPOSURE_MANUAL, 1 = EXPOSURE_AUTO
        xu_read(device, control::lr_exposure_mode, &m, sizeof(m));
        mode = m;
    }

    void set_lr_exposure_mode(uvc::device & device, uint32_t mode)
    {
        uint8_t m = mode;
        xu_write(device, control::lr_exposure_mode, &m, sizeof(m));
    }

    void get_depth_params(const uvc::device & device, depth_params & params)
    {
        xu_read(device, control::depth_params, &params, sizeof(params));
    }

    void set_depth_params(uvc::device & device, depth_params params)
    {
        xu_write(device, control::depth_params, &params, sizeof(params));
    }

    void get_disparity_mode(const uvc::device & device, disparity_mode & mode)
    {
        xu_read(device, control::disparity, &mode, sizeof(mode));
    }

    void set_disparity_mode(uvc::device & device, disparity_mode mode)
    {
        xu_write(device, control::disparity, &mode, sizeof(mode));
    }

    void get_disparity_shift(const uvc::device & device, uint32_t & shift)
    {
        xu_read(device, control::disparity_shift, &shift, sizeof(shift));
    }

    void set_disparity_shift(uvc::device & device, uint32_t shift)
    {
        xu_write(device, control::disparity_shift, &shift, sizeof(shift));
    }

	void get_register_value(uvc::device & device, uint32_t reg, uint32_t & value)
    {
		CommandPacket command(command::peek, reg);

        ResponsePacket response;
        send_command(device, command, response);

		value = response.value; 
    }

	void set_register_value(uvc::device & device, uint32_t reg, uint32_t value)
    {
		CommandPacket command(command::poke, reg, value);
        ResponsePacket response;
        send_command(device, command, response);
    }

    const depth_params depth_params::presets[] = {
        {5, 5, 192,  1,  512, 6, 24, 27,  7,   24}, // (DEFAULT) Default settings on chip. Similiar to the medium setting and best for outdoors.
        {5, 5,   0,  0, 1023, 0,  0,  0,  0, 2047}, // (OFF) Disable almost all hardware-based outlier removal
        {5, 5, 115,  1,  512, 6, 18, 25,  3,   24}, // (LOW) Provide a depthmap with a lower number of outliers removed, which has minimal false negatives.
        {5, 5, 185,  5,  505, 6, 35, 45, 45,   14}, // (MEDIUM) Provide a depthmap with a medium number of outliers removed, which has balanced approach.
        {5, 5, 175, 24,  430, 6, 48, 47, 24,   12}, // (OPTIMIZED) Provide a depthmap with a medium/high number of outliers removed. Derived from an optimization function.
        {5, 5, 235, 27,  420, 8, 80, 70, 90,   12}, // (HIGH) Provide a depthmap with a higher number of outliers removed, which has minimal false positives.
    };

} } // namespace rsimpl::r200
