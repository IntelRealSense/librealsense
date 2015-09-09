#include "r200-private.h"

#include <cstring>

#define CAMERA_XU_UNIT_ID                           2

#define STATUS_BIT_BOOT_DIAGNOSTIC_FAULT            (1 << 3)
#define STATUS_BIT_IFFLEY_CONSTANTS_VALID           (1 << 4)
#define STATUS_BIT_WATCHDOG_TIMER_RESET             (1 << 5)
#define STATUS_BIT_REC_BUFFER_OVERRUN               (1 << 6)
#define STATUS_BIT_CAM_DATA_FORMAT_ERROR            (1 << 7)
#define STATUS_BIT_CAM_FIFO_OVERFLOW                (1 << 8)
#define STATUS_BIT_REC_DIVIDED_BY_ZERO_ERROR        (1 << 9)
#define STATUS_BIT_UVC_HEADER_ERROR                 (1 << 10)
#define STATUS_BIT_EMITTER_FAULT                    (1 << 11)
#define STATUS_BIT_THERMAL_FAULT                    (1 << 12)
#define STATUS_BIT_REC_RUN_ENABLED                  (1 << 13)
#define STATUS_BIT_VDF_DEPTH_POINTER_STREAMING      (1 << 14)
#define STATUS_BIT_VDF_LR_POINTER_STREAMING         (1 << 15)
#define STATUS_BIT_VDF_WEBCAM_POINTER_STREAMING     (1 << 16)
#define STATUS_BIT_STREAMING_STATE                  (1 << 27) | (1 << 28) | (1 << 29) | (1 << 30)
#define STATUS_BIT_BUSY                             (1 << 31)

#define CONTROL_COMMAND_RESPONSE                    1
#define CONTROL_IFFLEY                              2
#define CONTROL_STREAM_INTENT                       3
#define CONTROL_DEPTH_UNITS                         4
#define CONTROL_MIN_MAX                             5
#define CONTROL_DISPARITY                           6
#define CONTROL_RECTIFICATION                       7
#define CONTROL_EMITTER                             8
#define CONTROL_TEMPERATURE                         9
#define CONTROL_DEPTH_PARAMS                        10
#define CONTROL_LAST_ERROR                          12
#define CONTROL_EMBEDDED_COUNT                      13
#define CONTROL_LR_EXPOSURE                         14
#define CONTROL_LR_AUTOEXPOSURE_PARAMETERS          15
#define CONTROL_SW_RESET                            16
#define CONTROL_LR_GAIN                             17
#define CONTROL_LR_EXPOSURE_MODE                    18
#define CONTROL_DISPARITY_SHIFT                     19
#define CONTROL_STATUS                              20
#define CONTROL_LR_EXPOSURE_DISCOVERY               21
#define CONTROL_LR_GAIN_DISCOVERY                   22
#define CONTROL_HW_TIMESTAMP                        23

#define COMMAND_DOWNLOAD_SPI_FLASH                  0x1A
#define COMMAND_PROTECT_FLASH                       0x1C
#define COMMAND_LED_ON                              0x14
#define COMMAND_LED_OFF                             0x15
#define COMMAND_GET_FWREVISION                      0x21
#define COMMAND_GET_SPI_PROTECT                     0x23
#define COMMAND_MODIFIER_DIRECT                     0x00000010

#define SPI_FLASH_PAGE_SIZE_IN_BYTES                        0x100
#define SPI_FLASH_SECTOR_SIZE_IN_BYTES                      0x1000
#define SPI_FLASH_SIZE_IN_SECTORS                           256
#define SPI_FLASH_TOTAL_SIZE_IN_BYTES                       (SPI_FLASH_SIZE_IN_SECTORS * SPI_FLASH_SECTOR_SIZE_IN_BYTES)
#define SPI_FLASH_PAGES_PER_SECTOR                          (SPI_FLASH_SECTOR_SIZE_IN_BYTES / SPI_FLASH_PAGE_SIZE_IN_BYTES)
#define SPI_FLASH_LENGTH_IN_PAGES(N_BYTES)                  ((N_BYTES + 0xFF) / SPI_FLASH_PAGE_SIZE_IN_BYTES)

#define SPI_FLASH_SECTORS_RESERVED_FOR_FIRMWARE             160
#define SPI_FLASH_START_OF_SECTORS_NOT_FOR_FIRMWARE         (SPI_FLASH_SECTORS_RESERVED_FOR_FIRMWARE * SPI_FLASH_SECTOR_SIZE_IN_BYTES)

#define SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES             64
#define SPI_FLASH_FIRST_ROUTINE_SECTOR                      (SPI_FLASH_SIZE_IN_SECTORS - SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES)

// 1 Mb total
#define NV_STORAGE_IN_BYTES                                 (SPI_FLASH_SECTOR_SIZE_IN_BYTES * SPI_FLASH_SIZE_IN_SECTORS)
#define NV_NON_FIRMWARE_START                               (SPI_FLASH_SECTORS_RESERVED_FOR_FIRMWARE * SPI_FLASH_SECTOR_SIZE_IN_BYTES)

#define NV_ADMIN_DATA_N_ENTRIES                             9
#define NV_CALIBRATION_DATA_ADDRESS_INDEX                   0
#define NV_IFFLEY_ROUTINE_TABLE_ADDRESS_INDEX               1

#define NV_NON_FIRMWARE_ROOT_ADDRESS                        NV_NON_FIRMWARE_START

#define UNUSED_ROUTINE(ENTRY)                               (ENTRY == UNINITIALIZED_ROUTINE_ENTRY || ENTRY == DELETED_ROUTINE_ENTRY)
typedef unsigned short RoutineDescription;

#define MAX_ROUTINES                                        256

#define SIZEOF_ROUTINE_DESCRIPTION_TABLE                    (MAX_ROUTINES * sizeof(RoutineDescription))
#define SIZEOF_ERASED_TABLE                                 (SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES * sizeof(unsigned short))
#define SIZEOF_PRESERVE_TABLE                               (SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES * sizeof(unsigned short))

#define SIZEOF_ROUTINE_DESCRIPTION_ERASED_AND_PRESERVE_TABLE (SIZEOF_ROUTINE_DESCRIPTION_TABLE + SIZEOF_ERASED_TABLE + SIZEOF_PRESERVE_TABLE)

#define ROUTINE_DESCRIPTION_OFFSET                          0
#define ERASED_TABLE_OFFSET                                 SIZEOF_ROUTINE_DESCRIPTION_TABLE
#define PRESERVE_TABLE_OFFSET                               (ERASED_TABLE_OFFSET + SIZEOF_ERASED_TABLE)

namespace rsimpl { namespace r200
{
    void send_command(uvc::device & device, CommandPacket & command, ResponsePacket & response)
    {
        device.set_control(CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, sizeof(command));
        device.get_control(CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, sizeof(response));
    }

    inline std::string ResponseCodeToString(uint32_t rc)
    {
        switch (rc)
        {
            case 0x10: return std::string("RESPONSE_OK"); break;
            case 0x11: return std::string("RESPONSE_TIMEOUT"); break;
            case 0x12: return std::string("RESPONSE_ACQUIRING_IMAGE"); break;
            case 0x13: return std::string("RESPONSE_IMAGE_BUSY"); break;
            case 0x14: return std::string("RESPONSE_ACQUIRING_SPI"); break;
            case 0x15: return std::string("RESPONSE_SENDING_SPI"); break;
            case 0x16: return std::string("RESPONSE_SPI_BUSY"); break;
            case 0x17: return std::string("RESPSONSE_UNAUTHORIZED"); break;
            case 0x18: return std::string("RESPONSE_ERROR"); break;
            case 0x19: return std::string("RESPONSE_CODE_END"); break;
            default: return "RESPONSE_UNKNOWN";
        }
    }

    typedef struct
    {
        RoutineDescription rd[MAX_ROUTINES]; // Partition Table
        unsigned short erasedTable[SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES];
        unsigned short preserveTable[SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES];
    } RoutineStorageTables;

    // Bus group
    #define COMMAND_MODIFIER_DIRECT 0x00000010

    #define CAM_INFO_BLOCK_LEN 2048

    class DS4HardwareIO
    {
        CameraCalibrationParameters cameraCalibration;
        CameraHeaderInfo cameraInfo;

        uvc::device & deviceHandle;

        void ReadCalibrationSector()
        {
            uint8_t flashDataBuffer[SPI_FLASH_SECTOR_SIZE_IN_BYTES];

            if (!read_admin_sector(flashDataBuffer, NV_CALIBRATION_DATA_ADDRESS_INDEX))
                throw std::runtime_error("Could not read calibration sector");

            assert(sizeof(cameraCalibration) <= CAM_INFO_BLOCK_LEN);
            memcpy(&cameraCalibration, flashDataBuffer, CAM_INFO_BLOCK_LEN);
            memcpy(&cameraInfo, flashDataBuffer + CAM_INFO_BLOCK_LEN, sizeof(cameraInfo));
        }

        bool read_pages(uint32_t address, unsigned char * buffer, uint32_t nPages)
        {
            int addressTest = SPI_FLASH_TOTAL_SIZE_IN_BYTES - address - nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

            if (!nPages || addressTest < 0)
                return false;

            // This command allows the host to read a block of data from the SPI flash.
            // Once this command is processed by the DS4, further command messages will be treated as SPI data
            // and therefore will be read from flash. The size of the SPI data must be a multiple of 256 bytes.
            // This will repeat until the number of bytes specified in the ‘value’ field of the original command
            // message has been read.  At that point the DS4 will process command messages as expected.

            CommandPacket command;
            command.code = COMMAND_DOWNLOAD_SPI_FLASH;
            command.modifier = COMMAND_MODIFIER_DIRECT;
            command.tag = 12;
            command.address = address;
            command.value = nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

            ResponsePacket response;

            send_command(deviceHandle, command, response);

            uint8_t *p = buffer;
            uint16_t spiLength = SPI_FLASH_PAGE_SIZE_IN_BYTES;
            for (unsigned int i = 0; i < nPages; ++i)
            {
                xu_read(deviceHandle, CONTROL_COMMAND_RESPONSE, p, spiLength);
                p += SPI_FLASH_PAGE_SIZE_IN_BYTES;
            }
            return true;
        }

        void read_arbitrary_chunk(uint32_t address, void * dataIn, int lengthInBytesIn)
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
                read_pages(startAddress, page, 1);
                memcpy(data, page + startInPage, lengthToCopy);
                lengthInBytes -= lengthToCopy;
                data += lengthToCopy;
                startAddress += SPI_FLASH_PAGE_SIZE_IN_BYTES;
            }

            nPagesToRead = lengthInBytes / SPI_FLASH_PAGE_SIZE_IN_BYTES;

            if (nPagesToRead > 0)
                read_pages(startAddress, data, nPagesToRead);

            lengthInBytes -= (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);

            if (lengthInBytes)
            {
                // means we still have a remainder
                data += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
                startAddress += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
                read_pages(startAddress, page, 1);
                memcpy(data, page, lengthInBytes);
            }
        }

        bool read_admin_sector(unsigned char data[SPI_FLASH_SECTOR_SIZE_IN_BYTES], int whichAdminSector)
        {
            uint32_t adminSectorAddresses[NV_ADMIN_DATA_N_ENTRIES];

            read_arbitrary_chunk(NV_NON_FIRMWARE_ROOT_ADDRESS, adminSectorAddresses, NV_ADMIN_DATA_N_ENTRIES * sizeof(adminSectorAddresses[0]));

            if (whichAdminSector >= 0 && whichAdminSector < NV_ADMIN_DATA_N_ENTRIES)
            {
                uint32_t pageAddressInBytes = adminSectorAddresses[whichAdminSector];
                return read_pages(pageAddressInBytes, data, SPI_FLASH_PAGES_PER_SECTOR);
            }

            return false;
        }

        int get_admin_sector_unused_copies(uint32_t sectorAddress, uint32_t * unusedCopiesPtr)
        {
            const uint32_t UNUSED_BITS_OFFSET = SPI_FLASH_SECTOR_SIZE_IN_BYTES - sizeof(uint32_t) - sizeof(uint32_t);

            uint32_t unusedCopies;
            uint32_t usedCopies;
            int usedCopiesCount = 0;
            uint32_t addressOfUnusedBits = sectorAddress + UNUSED_BITS_OFFSET;
            unsigned int i;

            read_arbitrary_chunk(addressOfUnusedBits, &unusedCopies, sizeof(uint32_t));
            usedCopies = ~unusedCopies;

            for (i = 0; i < (sizeof(uint32_t) * 8); i++)
            {
                if (usedCopies & (1 << i)) usedCopiesCount++;
            }

            *unusedCopiesPtr = unusedCopies;
            return usedCopiesCount;
        }

        void read_admin_table(int blockLength, void * data, int offset, int lengthToRead)
        {
            uint32_t address = NV_IFFLEY_ROUTINE_TABLE_ADDRESS_INDEX;
            uint32_t dummy;
            int usedCopiesCount = get_admin_sector_unused_copies(address, &dummy);
            uint32_t addressInSector = address + (usedCopiesCount - 1) * blockLength + offset;
            read_arbitrary_chunk(addressInSector, data, lengthToRead);
        }

        void read_spi_flash_memory()
        {
            RoutineStorageTables rst = {0};

            // Setup admin table
            read_admin_table(SIZEOF_ROUTINE_DESCRIPTION_ERASED_AND_PRESERVE_TABLE,
                             rst.rd,
                             ROUTINE_DESCRIPTION_OFFSET,
                             SIZEOF_ROUTINE_DESCRIPTION_TABLE);

            ReadCalibrationSector();
        }

    public:

        DS4HardwareIO(uvc::device & devh) : deviceHandle(devh)
        {
            read_spi_flash_memory();
        }

        ~DS4HardwareIO()
        {

        }

        void LogDebugInfo(CameraCalibrationParameters & p, CameraHeaderInfo & h)
        {
            // std::cout << "## Calibration Version: " << p.metadata.versionNumber << std::endl;
            // std::cout << "## Serial: " << h.serialNumber << std::endl;
            // std::cout << "## Model: " << h.modelNumber << std::endl;
            // std::cout << "## Revision: " << h.revisionNumber << std::endl;
            // std::cout << "## Head Version: " << h.cameraHeadContentsVersion << std::endl;
            // std::cout << "## Baseline: " << h.nominalBaseline << std::endl;
            // std::cout << "## OEM ID: " << h.OEMID << std::endl;
        }

        CameraCalibrationParameters GetCalibration() { return cameraCalibration; }
        CameraHeaderInfo GetCameraHeader() { return cameraInfo; }
    };

    void read_camera_info(uvc::device & device, CameraCalibrationParameters & calib, CameraHeaderInfo & header)
    {
        DS4HardwareIO internal(device);
        calib = internal.GetCalibration();
        header = internal.GetCameraHeader();
        //internal.LogDebugInfo(calib, header);
    }

    std::string read_firmware_version(uvc::device & device)
    {
        CommandPacket command;
        command.code = COMMAND_GET_FWREVISION;
        command.modifier = COMMAND_MODIFIER_DIRECT;
        command.tag = 12;

        ResponsePacket response;
        send_command(device, command, response);

        char fw[16];
        memcpy(fw, &response.revision, 16);
        return fw;
    }

    void xu_read(const uvc::device & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        device.get_control(CAMERA_XU_UNIT_ID, xu_ctrl, buffer, length);
    }

    void xu_write(uvc::device & device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        device.set_control(CAMERA_XU_UNIT_ID, xu_ctrl, buffer, length);
    }

    void set_stream_intent(uvc::device & device, uint8_t & intent)
    {
        xu_write(device, CONTROL_STREAM_INTENT, &intent, sizeof(intent));
    }

    void get_stream_status(const uvc::device & device, int & status)
    {
        uint8_t s[4] = {255, 255, 255, 255};
        xu_read(device, CONTROL_STATUS, s, sizeof(uint32_t));
        status = rsimpl::pack(s[0], s[1], s[2], s[3]);
    }

    void get_last_error(const uvc::device & device, uint8_t & last_error)
    {
        xu_read(device, CONTROL_LAST_ERROR, &last_error, sizeof(uint8_t));
    }

    void force_firmware_reset(uvc::device & device)
    {
        try
        {
            uint8_t reset = 1;
            xu_write(device, CONTROL_SW_RESET, &reset, sizeof(uint8_t));
        }
        catch(...) {} // xu_write always throws during a CONTROL_SW_RESET, since the firmware is unable to send a proper response
    }

    void get_emitter_state(const uvc::device & device, bool & state)
    {
        uint8_t byte = 0;
        xu_read(device, CONTROL_EMITTER, &byte, sizeof(byte));
        if (byte & 4) state = (byte & 2 ? true : false); // TODO: Figure out what should actually be done here
    }

    void set_emitter_state(uvc::device & device, bool state)
    {
        uint8_t newEmitterState = state ? 1 : 0;
        xu_read(device, CONTROL_EMITTER, &newEmitterState, sizeof(uint8_t));
    }

    void read_temperature(uvc::device & device, int8_t & current, int8_t & min, int8_t & max, int8_t & min_fault)
    {
        uint8_t buf[4] = {0};
        xu_read(device, CONTROL_TEMPERATURE, buf, sizeof(buf));
        current = buf[0];
        min = buf[1];
        max = buf[2];
        min_fault = buf[3];
    }

    void reset_temperature(uvc::device & device)
    {
        uint8_t buf[4] = {0};
        xu_write(device, CONTROL_TEMPERATURE, buf, sizeof(buf));
    }

    void get_depth_units(const uvc::device & device, uint32_t & units)
    {
        xu_read(device, CONTROL_DEPTH_UNITS, &units, sizeof(units));
    }

    void set_depth_units(uvc::device & device, uint32_t units)
    {
        xu_write(device, CONTROL_DEPTH_UNITS, &units, sizeof(units));
    }

    void set_min_max_depth(uvc::device & device, uint16_t min_depth, uint16_t max_depth)
    {
        uint16_t values[] = {min_depth, max_depth};
        xu_write(device, CONTROL_MIN_MAX, values, sizeof(values));
    }

    void get_min_max_depth(const uvc::device & device, uint16_t & min_depth, uint16_t & max_depth)
    {
        uint16_t values[] = {0, 0};
        xu_read(device, CONTROL_MIN_MAX, values, sizeof(values));
        min_depth = values[0];
        max_depth = values[1];
    }

    void get_lr_gain(const uvc::device & device, uint32_t & rate, uint32_t & gain)
    {
        uint32_t values[] = {0, 0};
        xu_read(device, CONTROL_LR_GAIN, values, sizeof(values));
        rate = values[0];
        gain = values[1];
    }

    void set_lr_gain(uvc::device & device, uint32_t rate, uint32_t gain)
    {
        uint32_t values[] = {rate, gain};
        xu_write(device, CONTROL_LR_GAIN, values, sizeof(values));
    }

    void get_lr_exposure(const uvc::device & device, uint32_t & rate, uint32_t & exposure)
    {
        uint32_t values[] = {0, 0};
        xu_read(device, CONTROL_LR_EXPOSURE, values, sizeof(values));
        rate = values[0];
        exposure = values[1];
    }

    void set_lr_exposure(uvc::device & device, uint32_t rate, uint32_t exposure)
    {
        uint32_t values[] = {rate, exposure};
        xu_write(device, CONTROL_LR_EXPOSURE, values, sizeof(values));
    }

    void get_lr_auto_exposure_params(const uvc::device & device, auto_exposure_params & params)
    {
        xu_read(device, CONTROL_LR_AUTOEXPOSURE_PARAMETERS, &params, sizeof(params));
    }

    void set_lr_auto_exposure_params(uvc::device & device, auto_exposure_params params)
    {
        xu_write(device, CONTROL_LR_AUTOEXPOSURE_PARAMETERS, &params, sizeof(params));
    }

    void get_lr_exposure_mode(const uvc::device & device, uint32_t & mode)
    {
        uint8_t m; // 0 = EXPOSURE_MANUAL, 1 = EXPOSURE_AUTO
        xu_read(device, CONTROL_LR_EXPOSURE_MODE, &m, sizeof(m));
        mode = m;
    }

    void set_lr_exposure_mode(uvc::device & device, uint32_t mode)
    {
        xu_write(device, CONTROL_LR_EXPOSURE_MODE, &mode, sizeof(mode));
    }

    void get_depth_params(const uvc::device & device, depth_params & params)
    {
        xu_read(device, CONTROL_DEPTH_PARAMS, &params, sizeof(params));
    }

    void set_depth_params(uvc::device & device, depth_params params)
    {
        xu_write(device, CONTROL_DEPTH_PARAMS, &params, sizeof(params));
    }

    void get_disparity_mode(const uvc::device & device, disparity_mode & mode)
    {
        xu_read(device, CONTROL_DISPARITY, &mode, sizeof(mode));
    }

    void set_disparity_mode(uvc::device & device, disparity_mode mode)
    {
        xu_write(device, CONTROL_DISPARITY, &mode, sizeof(mode));
    }

    void get_disparity_shift(const uvc::device & device, uint32_t & shift)
    {
        xu_read(device, CONTROL_DISPARITY_SHIFT, &shift, sizeof(shift));
    }

    void set_disparity_shift(uvc::device & device, uint32_t shift)
    {
        xu_write(device, CONTROL_DISPARITY_SHIFT, &shift, sizeof(shift));
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
