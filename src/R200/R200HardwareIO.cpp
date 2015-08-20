#include "HardwareIO.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace std;

namespace r200
{
    struct CommandPacket
    {
        CommandPacket(uint32_t code_ = 0, uint32_t modifier_ = 0, uint32_t tag_ = 0, uint32_t address_ = 0, uint32_t value_ = 0)
        : code(code_), modifier(modifier_), tag(tag_), address(address_), value(value_)
        {
        }

        uint32_t code = 0;
        uint32_t modifier = 0;
        uint32_t tag = 0;
        uint32_t address = 0;
        uint32_t value = 0;
        uint32_t reserved[59]{};
    };

    struct ResponsePacket
    {
        ResponsePacket(uint32_t code_ = 0, uint32_t modifier_ = 0, uint32_t tag_ = 0, uint32_t responseCode_ = 0, uint32_t value_ = 0)
        : code(code_), modifier(modifier_), tag(tag_), responseCode(responseCode_), value(value_)
        {
        }

        uint32_t code = 0;
        uint32_t modifier = 0;
        uint32_t tag = 0;
        uint32_t responseCode = 0;
        uint32_t value = 0;
        uint32_t revision[4]{};
        uint32_t reserved[55]{};
    };

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

    typedef struct
    {
        RoutineDescription rd[MAX_ROUTINES]; // Partition Table
        unsigned short erasedTable[SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES];
        unsigned short preserveTable[SPI_FLASH_SECTORS_RESERVED_FOR_ROUTINES];
    } RoutineStorageTables;

    // Bus group
    #define COMMAND_MODIFIER_DIRECT 0x00000010

    #define CAM_INFO_BLOCK_LEN 2048
    
    template<class T> void swap_endian_bytewise(T & value) { auto p = (uint8_t *)&value; for(size_t i=0; i<sizeof(T)/2; ++i) std::swap(p[i], p[sizeof(T)-i-1]); }
    void swap_endian(uint16_t & value) { swap_endian_bytewise(value); }
    void swap_endian(uint32_t & value) { swap_endian_bytewise(value); }
    void swap_endian(float & value) { swap_endian_bytewise(value); }
    template<class T, int N> void swap_endian(T (& value)[N]) { for(auto & elem : value) swap_endian(elem); }
    template<class T, class... U> void swap_endian(T & value, U & ... rest) { swap_endian(value); swap_endian(rest...); }
    void swap_endian(CalibrationMetadata & value) { swap_endian(value.versionNumber, value.numIntrinsicsRight, value.numIntrinsicsThird, value.numIntrinsicsPlatform, value.numRectifiedModesLR, value.numRectifiedModesThird, value.numRectifiedModesPlatform); }
    void swap_endian(UnrectifiedIntrinsics & value) { swap_endian(value.fx, value.fy, value.px, value.py, value.k, value.w, value.h); }
    void swap_endian(RectifiedIntrinsics & value) { swap_endian(value.rfx, value.rfy, value.rpx, value.rpy, value.rw, value.rh); }
    void swap_endian(CameraCalibrationParameters & value) { swap_endian(value.metadata, value.intrinsicsLeft, value.intrinsicsRight, value.intrinsicsThird, value.intrinsicsPlatform,
        value.modesLR, value.modesThird, value.modesPlatform, value.Rleft, value.Rright, value.Rthird, value.Rplatform, value.B, value.T, value.Tplatform, value.Rworld, value.Tworld); }

    class DS4HardwareIOInternal
    {
        CameraCalibrationParameters cameraCalibration;
        CameraHeaderInfo cameraInfo;

        uint8_t flashDataBuffer[SPI_FLASH_SECTOR_SIZE_IN_BYTES];
        uint8_t calibrationDataBuffer[CAM_INFO_BLOCK_LEN];

        uint32_t adminSectorAddresses[NV_ADMIN_DATA_N_ENTRIES];
        RoutineStorageTables rst;

        uvc_device_handle_t * deviceHandle;

        void ReadCalibrationSector()
        {
            if (!read_admin_sector(flashDataBuffer, NV_CALIBRATION_DATA_ADDRESS_INDEX))
                throw std::runtime_error("Could not read calibration sector");

            assert(sizeof(cameraCalibration) <= CAM_INFO_BLOCK_LEN);
            memcpy(&cameraCalibration, flashDataBuffer, CAM_INFO_BLOCK_LEN);
            swap_endian(cameraCalibration);

            memcpy(&cameraInfo, flashDataBuffer + CAM_INFO_BLOCK_LEN, sizeof(cameraInfo));
        }

        bool read_pages(uint32_t address, unsigned char * buffer, uint32_t nPages)
        {
            int addressTest = SPI_FLASH_TOTAL_SIZE_IN_BYTES - address - nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

            if (!nPages || addressTest < 0)
                return false;

            //@tofix - could be refactored using generic xu read/write cycle function

            CommandPacket command;
            command.code = COMMAND_DOWNLOAD_SPI_FLASH;
            command.modifier = COMMAND_MODIFIER_DIRECT;
            command.tag = 12;
            command.address = address;
            command.value = nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;

            unsigned int cmdLength = sizeof(CommandPacket);
            auto XUWriteCmdStatus = uvc_set_ctrl(deviceHandle, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, cmdLength);
            if (XUWriteCmdStatus < 0) uvc_perror((uvc_error_t) XUWriteCmdStatus, "uvc_set_ctrl");

            ResponsePacket response;
            unsigned int resLength = sizeof(ResponsePacket);
            auto XUReadCmdStatus = uvc_get_ctrl(deviceHandle, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, resLength, UVC_GET_CUR);
            if (XUReadCmdStatus < 0) uvc_perror((uvc_error_t) XUReadCmdStatus, "uvc_get_ctrl");

            std::cout << "Read SPI Command Status: " << ResponseCodeToString(response.responseCode) << std::endl;

            if (XUReadCmdStatus > 0)
            {
                uint8_t *p = buffer;
                uint16_t spiLength = SPI_FLASH_PAGE_SIZE_IN_BYTES;
                for (unsigned int i = 0; i < nPages; ++i)
                {
                    int bytesReturned = uvc_get_ctrl(deviceHandle, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, p, spiLength, UVC_GET_CUR);
                    p += SPI_FLASH_PAGE_SIZE_IN_BYTES;
                }
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

        void read_admin_table(int whichAdminSector, int blockLength, void * data, int offset, int lengthToRead)
        {
            uint32_t address = adminSectorAddresses[whichAdminSector];
            uint32_t dummy;
            int usedCopiesCount = get_admin_sector_unused_copies(address, &dummy);
            uint32_t addressInSector = address + (usedCopiesCount - 1) * blockLength + offset;
            read_arbitrary_chunk(addressInSector, data, lengthToRead);
        }

    public:

        DS4HardwareIOInternal(uvc_device_handle_t * devh) : deviceHandle(devh)
        {
            rst = {0};

            // Setup admin table
            read_admin_table(NV_IFFLEY_ROUTINE_TABLE_ADDRESS_INDEX,
                             SIZEOF_ROUTINE_DESCRIPTION_ERASED_AND_PRESERVE_TABLE,
                             rst.rd,
                             ROUTINE_DESCRIPTION_OFFSET,
                             SIZEOF_ROUTINE_DESCRIPTION_TABLE);

            ReadCalibrationSector();
        }

        ~DS4HardwareIOInternal()
        {

        }

        void LogDebugInfo()
        {
            std::cout << "######## Serial: " << cameraInfo.serialNumber << std::endl;
            std::cout << "######## Model: " << cameraInfo.modelNumber << std::endl;
            std::cout << "######## Revision: " << cameraInfo.revisionNumber << std::endl;
            std::cout << "######## Calibrated: " << cameraInfo.calibrationDate << std::endl;
            std::cout << "######## Head Version: " << cameraInfo.cameraHeadContentsVersion << std::endl;
            std::cout << "######## Baseline: " << cameraInfo.nominalBaseline << std::endl;
            std::cout << "######## OEM ID: " << cameraInfo.OEMID << std::endl;

            auto params = cameraCalibration.modesLR[0];

            std::cout << "## Mode LR[0] Rectified Intrinsics: " << std::endl;
            std::cout << "## Calibration Version: " << cameraCalibration.metadata.versionNumber << std::endl;
            std::cout << "## rfx: " << params.rfx << std::endl;
            std::cout << "## rfy: " << params.rfy << std::endl;
            std::cout << "## rpx: " << params.rpx << std::endl;
            std::cout << "## rpy: " << params.rpy << std::endl;
            std::cout << "## rw:  " << params.rw << std::endl;
            std::cout << "## rh:  " << params.rh << std::endl;
        }

        CameraCalibrationParameters GetCalibration() { return cameraCalibration; }
        CameraHeaderInfo GetCameraHeader() { return cameraInfo; }
    };

    void read_camera_info(uvc_device_handle_t * device, CameraCalibrationParameters & calib, CameraHeaderInfo & header)
    {
        DS4HardwareIOInternal internal(device);
        internal.LogDebugInfo();
        calib = internal.GetCalibration();
        header = internal.GetCameraHeader();
    }

    int read_stream_status(uvc_device_handle_t *devh)
    {
        uint32_t status = 0xffffffff;

        size_t length = sizeof(uint32_t);

        // XU Read
        auto s = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_STATUS, &status, int(length), UVC_GET_CUR);

        if (s > 0)
        {
            return s;
        }

        if (s < 0) uvc_perror((uvc_error_t)s, "uvc_get_ctrl");

        return -1;
    }

    bool write_stream_intent(uvc_device_handle_t *devh, uint8_t intent)
    {
        size_t length = sizeof(uint8_t);

        // XU Write
        auto s = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_STREAM_INTENT, &intent, int(length));

        if (s > 0)
        {
            return true;
        }

        if (s < 0) uvc_perror((uvc_error_t)s, "uvc_set_ctrl");

        return false;
    }

    std::string read_firmware_version(uvc_device_handle_t * devh)
    {
        CommandPacket command;
        command.code = COMMAND_GET_FWREVISION;
        command.modifier = COMMAND_MODIFIER_DIRECT;
        command.tag = 12;

        unsigned int cmdLength = sizeof(CommandPacket);
        auto s = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, cmdLength);
        if (s < 0) uvc_perror((uvc_error_t)s, "uvc_set_ctrl");

        ResponsePacket response;
        unsigned int resLength = sizeof(ResponsePacket);
        s = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, resLength, UVC_GET_CUR);
        if (s < 0) uvc_perror((uvc_error_t)s, "uvc_get_ctrl");

        char fw[16];

        memcpy(fw, &response.revision, 16);

        return std::string(fw);
    }
    
} // end namespace r200
