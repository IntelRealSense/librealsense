#include "R200.h"

#include <iostream>

namespace rsimpl { namespace r200
{
    enum { LR_FULL, LR_BIG, Z_FULL, Z_BIG, THIRD_HD, THIRD_VGA, NUM_INTRINSICS };
    static StaticCameraInfo get_r200_info()
    {
        StaticCameraInfo info;
        info.stream_subdevices[RS_INFRARED] = 0;
        info.stream_subdevices[RS_INFRARED_2] = 0;
        info.stream_subdevices[RS_DEPTH] = 1;
        info.stream_subdevices[RS_COLOR] = 2;
        for(auto fps : {30, 60, 90})
        {
            auto uvcFps = fps == 60 ? 59 : fps; // UVC sees the 60 fps mode as 59 fps

            // left/right modes on subdevice 0
            info.subdevice_modes.push_back({0, 640, 481, UVC_FRAME_FORMAT_Y8, uvcFps, {{RS_INFRARED, 640, 480, RS_Y8, fps, LR_FULL}}, &unpack_strided_image});
            info.subdevice_modes.push_back({0, 640, 481, UVC_FRAME_FORMAT_Y12I, uvcFps, {{RS_INFRARED, 640, 480, RS_Y8, fps, LR_FULL}, {RS_INFRARED_2, 640, 480, RS_Y8, fps, LR_FULL}}, &unpack_rly12_to_y8});
            info.subdevice_modes.push_back({0, 640, 373, UVC_FRAME_FORMAT_Y8, uvcFps, {{RS_INFRARED, 492, 372, RS_Y8, fps, LR_BIG}}, &unpack_strided_image});
            info.subdevice_modes.push_back({0, 640, 373, UVC_FRAME_FORMAT_Y12I, uvcFps, {{RS_INFRARED, 492, 372, RS_Y8, fps, LR_BIG}, {RS_INFRARED_2, 492, 372, RS_Y8, fps, LR_BIG}}, &unpack_rly12_to_y8});

            // z modes on subdevice 1
            info.subdevice_modes.push_back({1, 628, 469, UVC_FRAME_FORMAT_Z16, uvcFps, {{RS_DEPTH, 628, 468, RS_Z16, fps, Z_FULL}}, &unpack_strided_image});
            info.subdevice_modes.push_back({1, 628, 361, UVC_FRAME_FORMAT_Z16, uvcFps, {{RS_DEPTH, 480, 360, RS_Z16, fps, Z_BIG}}, &unpack_strided_image});

            // third modes on subdevice 2
            if(fps == 90) continue;
            info.subdevice_modes.push_back({2, 1920, 1080, UVC_FRAME_FORMAT_YUYV, uvcFps, {{RS_COLOR, 1920, 1080, RS_RGB8, fps, THIRD_HD}}, &unpack_yuyv_to_rgb});
            info.subdevice_modes.push_back({2, 640, 480, UVC_FRAME_FORMAT_YUYV, uvcFps, {{RS_COLOR, 640, 480, RS_RGB8, fps, THIRD_VGA}}, &unpack_yuyv_to_rgb});
        }
        return info;
    }

    R200Camera::R200Camera(uvc_context_t * ctx, uvc_device_t * device) : rs_camera(ctx, device, get_r200_info())
    {

    }
    
    R200Camera::~R200Camera()
    {
        force_firmware_reset(first_handle);
    }

    void R200Camera::EnableStreamPreset(int streamIdentifier, int preset)
    {
        switch(streamIdentifier)
        {
        case RS_DEPTH: EnableStream(streamIdentifier, 480, 360, 60, RS_Z16); break;
        case RS_COLOR: EnableStream(streamIdentifier, 640, 480, 60, RS_RGB8); break;
        case RS_INFRARED: EnableStream(streamIdentifier, 492, 372, 60, RS_Y8); break;
        case RS_INFRARED_2: EnableStream(streamIdentifier, 492, 372, 60, RS_Y8); break;
        default: throw std::runtime_error("unsupported stream");
        }
    }

    static rs_intrinsics MakeLeftRightIntrinsics(const RectifiedIntrinsics & i)
    {
        return {{(int)i.rw, (int)i.rh}, {i.rfx, i.rfy}, {i.rpx, i.rpy}, {0,0,0,0,0}, RS_NO_DISTORTION};
    }

    static rs_intrinsics MakeDepthIntrinsics(const RectifiedIntrinsics & i)
    {
        return {{(int)i.rw-12, (int)i.rh-12}, {i.rfx, i.rfy}, {i.rpx-6, i.rpy-6}, {0,0,0,0,0}, RS_NO_DISTORTION};
    }

    static rs_intrinsics MakeColorIntrinsics(const UnrectifiedIntrinsics & i)
    {
        return {{(int)i.w, (int)i.h}, {i.fx,i.fy}, {i.px,i.py}, {i.k[0],i.k[1],i.k[2],i.k[3],i.k[4]}, RS_GORDON_BROWN_CONRADY_DISTORTION};
    }

    CalibrationInfo R200Camera::RetrieveCalibration()
    {
        CameraCalibrationParameters calib;
        CameraHeaderInfo header;
        read_camera_info(first_handle, calib, header);

        CalibrationInfo c;
        c.intrinsics.resize(NUM_INTRINSICS);
        c.intrinsics[LR_FULL] = MakeLeftRightIntrinsics(calib.modesLR[0]);
        c.intrinsics[LR_BIG] = MakeLeftRightIntrinsics(calib.modesLR[1]);
        c.intrinsics[Z_FULL] = MakeDepthIntrinsics(calib.modesLR[0]);
        c.intrinsics[Z_BIG] = MakeDepthIntrinsics(calib.modesLR[1]);
        c.intrinsics[THIRD_HD] = MakeColorIntrinsics(calib.intrinsicsThird[0]);
        c.intrinsics[THIRD_VGA] = MakeColorIntrinsics(calib.intrinsicsThird[1]);
        c.stream_poses[RS_DEPTH] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};
        c.stream_poses[RS_INFRARED] = c.stream_poses[RS_DEPTH];
        c.stream_poses[RS_INFRARED_2] = c.stream_poses[RS_DEPTH]; // TODO: Figure out the correct translation vector to put here
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) c.stream_poses[RS_COLOR].orientation(i,j) = calib.Rthird[0][i*3+j];
        for(int i=0; i<3; ++i) c.stream_poses[RS_COLOR].position[i] = calib.T[0][i] * 0.001f;
        c.stream_poses[RS_COLOR].position = c.stream_poses[RS_COLOR].orientation * c.stream_poses[RS_COLOR].position;
        c.depth_scale = 0.001f;
        return c;
    }

    void R200Camera::SetStreamIntent()
    {
        uint8_t streamIntent = 0;
        if(subdevices[0]) streamIntent |= STATUS_BIT_LR_STREAMING;
        if(subdevices[1]) streamIntent |= STATUS_BIT_Z_STREAMING;
        if(subdevices[2]) streamIntent |= STATUS_BIT_WEB_STREAMING;

        if(first_handle)
        {
            if (!set_stream_intent(first_handle, streamIntent)) throw std::runtime_error("could not set stream intent");
        }
    }
} }

////////////////////////////////
// Former HardwareIO contents //
////////////////////////////////

#include <iostream>

namespace rsimpl { namespace r200
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

    bool send_command(uvc_device_handle_t * device, CommandPacket * command, ResponsePacket * response)
    {
        uint32_t cmdSz = sizeof(CommandPacket);
        auto status = uvc_set_ctrl(device, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, command, cmdSz);
        if (status < 0)
        {
            uvc_perror((uvc_error_t) status, "SendCommand - uvc_set_ctrl");
            return false;
        }

        uint32_t resSz = sizeof(ResponsePacket);
        status = uvc_get_ctrl(device, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, response, resSz, UVC_GET_CUR);
        if (status < 0)
        {
            uvc_perror((uvc_error_t) status, "SendCommand - uvc_get_ctrl");
            return false;
        }
        return true;
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

    class DS4HardwareIO
    {
        CameraCalibrationParameters cameraCalibration;
        CameraHeaderInfo cameraInfo;

        uvc_device_handle_t * deviceHandle;

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

            if (!send_command(deviceHandle, &command, &response))
            {
                return false;
            }

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

        DS4HardwareIO(uvc_device_handle_t * devh) : deviceHandle(devh)
        {
            read_spi_flash_memory();
        }

        ~DS4HardwareIO()
        {

        }

        void LogDebugInfo(CameraCalibrationParameters & p, CameraHeaderInfo & h)
        {
            std::cout << "## Calibration Version: " << p.metadata.versionNumber << std::endl;
            std::cout << "## Serial: " << h.serialNumber << std::endl;
            std::cout << "## Model: " << h.modelNumber << std::endl;
            std::cout << "## Revision: " << h.revisionNumber << std::endl;
            std::cout << "## Head Version: " << h.cameraHeadContentsVersion << std::endl;
            std::cout << "## Baseline: " << h.nominalBaseline << std::endl;
            std::cout << "## OEM ID: " << h.OEMID << std::endl;
        }

        CameraCalibrationParameters GetCalibration() { return cameraCalibration; }
        CameraHeaderInfo GetCameraHeader() { return cameraInfo; }
    };

    inline uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }
    
    void read_camera_info(uvc_device_handle_t * device, CameraCalibrationParameters & calib, CameraHeaderInfo & header)
    {
        DS4HardwareIO internal(device);
        calib = internal.GetCalibration();
        header = internal.GetCameraHeader();
        internal.LogDebugInfo(calib, header);
    }

    std::string read_firmware_version(uvc_device_handle_t * device)
    {
        CommandPacket command;
        command.code = COMMAND_GET_FWREVISION;
        command.modifier = COMMAND_MODIFIER_DIRECT;
        command.tag = 12;

        ResponsePacket response;
        if (!send_command(device, &command, &response)) throw std::runtime_error("failed to read firmware version");

        char fw[16];
        memcpy(fw, &response.revision, 16);
        return fw;
    }

    bool xu_read(uvc_device_handle_t * device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        auto status = uvc_get_ctrl(device, CAMERA_XU_UNIT_ID, xu_ctrl, buffer, length, UVC_GET_CUR);
        if (status < 0)
        {
            uvc_perror((uvc_error_t) status, "xu_read - uvc_get_ctrl");
            return false;
        }
        return true;
    }

    bool xu_write(uvc_device_handle_t * device, uint64_t xu_ctrl, void * buffer, uint32_t length)
    {
        auto status = uvc_set_ctrl(device, CAMERA_XU_UNIT_ID, xu_ctrl, buffer, length);
        if (status < 0)
        {
            uvc_perror((uvc_error_t) status, "xu_write - uvc_set_ctrl");
            return false;
        }
        return true;
    }
    
    bool set_stream_intent(uvc_device_handle_t * device, uint8_t & intent)
    {
        return xu_write(device, CONTROL_STREAM_INTENT, &intent, sizeof(intent));
    }
    
    bool get_stream_status(uvc_device_handle_t * device, int & status)
    {
        uint8_t s[4] = {255, 255, 255, 255};
        if (!xu_read(device, CONTROL_STATUS, s, (int) sizeof(uint32_t)))
            return false;
        status = pack(s[0], s[1], s[2], s[3]);
        return true;
    }
    
    bool get_last_error(uvc_device_handle_t * device, uint8_t & last_error)
    {
        return xu_read(device, CONTROL_LAST_ERROR, &last_error, sizeof(uint8_t));
    }

    bool force_firmware_reset(uvc_device_handle_t * device)
    {
        uint8_t reset = 1;
        return xu_write(device, CONTROL_SW_RESET, &reset, sizeof(uint8_t));
    }

    bool get_emitter_state(uvc_device_handle_t * device, bool & state)
    {
        uint8_t byte = 0;
        if (!xu_read(device, CONTROL_EMITTER, &byte, sizeof(byte)))
            return false;

        if (byte & 4) state = (byte & 2 ? true : false);
        return true;
    }

    bool set_emitter_state(uvc_device_handle_t * device, bool state)
    {
        uint8_t newEmitterState = state ? 1 : 0;
        return xu_read(device, CONTROL_EMITTER, &newEmitterState, sizeof(uint8_t));
    }

    bool read_temperature(uvc_device_handle_t * device, int8_t & current, int8_t & min, int8_t & max, int8_t & min_fault)
    {
        uint32_t length = 4;
        uint8_t buf[4] = {0};
        if (!xu_read(device, CONTROL_TEMPERATURE, buf, length))
            return false;
        current = buf[0];
        min = buf[1];
        max = buf[2];
        min_fault = buf[3];
        return true;
    }

    bool reset_temperature(uvc_device_handle_t * device)
    {
        uint32_t length = 4;
        uint8_t buf[4] = {0};
        if (!xu_write(device, CONTROL_TEMPERATURE, buf, length))
            return false;
        return true;
    }
    
    bool get_depth_units(uvc_device_handle_t * device, uint32_t & units)
    {
        return xu_read(device, CONTROL_DEPTH_UNITS, &units, sizeof(units));
    }
    
    bool set_depth_units(uvc_device_handle_t * device, uint32_t units)
    {
        return xu_write(device, CONTROL_DEPTH_UNITS, &units, sizeof(units));
    }
    
    bool get_min_max_depth(uvc_device_handle_t * device, uint16_t & min_depth, uint16_t & max_depth)
    {
        std::vector<uint16_t> minmax_values = {0, 0};
        if (!xu_read(device, CONTROL_MIN_MAX, minmax_values.data(), sizeof(minmax_values)))
            return false;
        min_depth = minmax_values[0];
        max_depth = minmax_values[1];
        return true;
    }
    
    bool get_min_max_depth(uvc_device_handle_t * device, uint16_t min_depth, uint16_t  max_depth)
    {
        std::vector<uint16_t> minmax_values = {min_depth, max_depth};
        if (!xu_read(device, CONTROL_MIN_MAX, minmax_values.data(), sizeof(minmax_values)))
            return false;
        return true;
    }
    
    bool get_lr_gain(uvc_device_handle_t * device, uint32_t & rate, uint32_t & gain)
    {
        std::vector<uint32_t> lr_gain_values = {0, 0};
        if (!xu_read(device, CONTROL_LR_GAIN, lr_gain_values.data(), sizeof(lr_gain_values)))
            return false;
        rate = lr_gain_values[0];
        gain = lr_gain_values[1];
        return true;
    }
    
    bool set_lr_gain(uvc_device_handle_t * device, uint32_t rate, uint32_t gain)
    {
        std::vector<uint32_t> lr_gain_values = {rate, gain};
        if (!xu_write(device, CONTROL_LR_GAIN, lr_gain_values.data(), sizeof(lr_gain_values)))
            return false;
        return true;
    }
    
    bool get_lr_exposure(uvc_device_handle_t * device, uint32_t & rate, uint32_t & exposure)
    {
        std::vector<uint32_t> lr_exposure_values = {0, 0};
        if (!xu_read(device, CONTROL_LR_EXPOSURE, lr_exposure_values.data(), sizeof(lr_exposure_values)))
            return false;
        rate = lr_exposure_values[0];
        exposure = lr_exposure_values[1];
        return true;
    }
    
    bool set_lr_exposure(uvc_device_handle_t * device, uint32_t rate, uint32_t exposure)
    {
        std::vector<uint32_t> lr_exposure_values = {rate, exposure};
        if (!xu_write(device, CONTROL_LR_EXPOSURE, lr_exposure_values.data(), sizeof(lr_exposure_values)))
            return false;
        return true;
    }
    
    bool get_lr_auto_exposure_params(uvc_device_handle_t * device, auto_exposure_params & params)
    {
        if (!xu_read(device, CONTROL_LR_AUTOEXPOSURE_PARAMETERS, &params, sizeof(params)))
            return false;
        return true;
    }
    
    bool set_lr_auto_exposure_params(uvc_device_handle_t * device, auto_exposure_params params)
    {
        if (!xu_write(device, CONTROL_LR_AUTOEXPOSURE_PARAMETERS, &params, sizeof(params)))
            return false;
        return true;
    }
    
    bool get_lr_exposure_mode(uvc_device_handle_t * device, int & mode)
    {
        uint8_t m;
        if (!xu_read(device, CONTROL_LR_EXPOSURE_MODE, &m, sizeof(m)))
            return false;
        mode = m;
        return true;
    }
    
    bool get_depth_params(uvc_device_handle_t * device, depth_params & params)
    {
        if (!xu_read(device, CONTROL_DEPTH_PARAMS, &params, sizeof(params)))
            return false;
        return true;
    }
    
    bool set_depth_params(uvc_device_handle_t * device, depth_params params)
    {
        if (!xu_write(device, CONTROL_DEPTH_PARAMS, &params, sizeof(params)))
            return false;
        return true;
    }

} } // namespace rsimpl::r200
