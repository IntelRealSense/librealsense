#include <librealsense/R200/SPI.h>
#include <librealsense/R200/CalibIO.h>
#include <librealsense/R200/CameraHeader.h>

using namespace std;

namespace r200
{

/////////
// SPI //
/////////

struct SPI
{
    uint32_t adminSectorAddresses[NV_ADMIN_DATA_N_ENTRIES];
    
    bool read_pages(uvc_device_handle_t *devh, uint32_t address, unsigned char * buffer, uint32_t nPages)
    {
        int addressTest = SPI_FLASH_TOTAL_SIZE_IN_BYTES - address - nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;
        
        if (!nPages || addressTest < 0)
            return false;
        
        // SendCommand()
        
        CommandPacket command;
        command.code = COMMAND_DOWNLOAD_SPI_FLASH;
        command.modifier = COMMAND_MODIFIER_DIRECT;
        command.tag = 12;
        command.address = address;
        command.value = nPages * SPI_FLASH_PAGE_SIZE_IN_BYTES;
        
        unsigned int cmdLength = sizeof(CommandPacket);
        auto XUWriteCmdStatus = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, cmdLength);
        
        ResponsePacket response;
        unsigned int resLength = sizeof(ResponsePacket);
        auto XUReadCmdStatus = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, resLength, UVC_GET_CUR);
        
        std::cout << "Read SPI Command Status: " << ResponseCodeToString(response.responseCode) << std::endl;
        
        // End SendCommand()
        
        bool returnStatus = true;
        
        if (XUReadCmdStatus > 0)
        {
            uint8_t *p = buffer;
            uint16_t spiLength = SPI_FLASH_PAGE_SIZE_IN_BYTES;
            for (unsigned int i = 0; i < nPages; ++i)
            {
                int bytesReturned = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, p, spiLength, UVC_GET_CUR);
                p += SPI_FLASH_PAGE_SIZE_IN_BYTES;
            }
        }
        
        return returnStatus;
    }
    
    void read_arbitrary_chunk(uvc_device_handle_t *devh, uint32_t address, void * dataIn, int lengthInBytesIn)
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
            read_pages(devh, startAddress, page, 1);
            memcpy(data, page + startInPage, lengthToCopy);
            lengthInBytes -= lengthToCopy;
            data += lengthToCopy;
            startAddress += SPI_FLASH_PAGE_SIZE_IN_BYTES;
        }
        
        nPagesToRead = lengthInBytes / SPI_FLASH_PAGE_SIZE_IN_BYTES;
        
        if (nPagesToRead > 0)
            read_pages(devh, startAddress, data, nPagesToRead);
        
        lengthInBytes -= (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
        
        if (lengthInBytes)
        {
            // means we still have a remainder
            data += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
            startAddress += (nPagesToRead * SPI_FLASH_PAGE_SIZE_IN_BYTES);
            read_pages(devh, startAddress, page, 1);
            memcpy(data, page, lengthInBytes);
        }
    }
    
    
    bool read_admin_sector(uvc_device_handle_t *devh, unsigned char data[SPI_FLASH_SECTOR_SIZE_IN_BYTES], int whichAdminSector)
    {
        read_arbitrary_chunk(devh, NV_NON_FIRMWARE_ROOT_ADDRESS, adminSectorAddresses, NV_ADMIN_DATA_N_ENTRIES * sizeof(adminSectorAddresses[0]));
        
        if (whichAdminSector >= 0 && whichAdminSector < NV_ADMIN_DATA_N_ENTRIES)
        {
            uint32_t pageAddressInBytes = adminSectorAddresses[whichAdminSector];
            return read_pages(devh, pageAddressInBytes, data, SPI_FLASH_PAGES_PER_SECTOR);
        }
        
        return false;
    }
    
    int get_admin_sector_unused_copies(uvc_device_handle_t *devh, uint32_t sectorAddress, uint32_t * unusedCopiesPtr)
    {
        const uint32_t UNUSED_BITS_OFFSET = SPI_FLASH_SECTOR_SIZE_IN_BYTES - sizeof(uint32_t) - sizeof(uint32_t);
        
        uint32_t unusedCopies;
        uint32_t usedCopies;
        int usedCopiesCount = 0;
        uint32_t addressOfUnusedBits = sectorAddress + UNUSED_BITS_OFFSET;
        unsigned int i;
        
        read_arbitrary_chunk(devh, addressOfUnusedBits, &unusedCopies, sizeof(uint32_t));
        usedCopies = ~unusedCopies;
        
        for (i = 0; i < (sizeof(uint32_t) * 8); i++)
        {
            if (usedCopies & (1 << i)) usedCopiesCount++;
        }
        
        *unusedCopiesPtr = unusedCopies;
        return usedCopiesCount;
    }
    
    void read_admin_table(uvc_device_handle_t *devh, int whichAdminSector, int blockLength, void * data, int offset, int lengthToRead)
    {
        uint32_t address = adminSectorAddresses[whichAdminSector];
        uint32_t dummy;
        int usedCopiesCount = get_admin_sector_unused_copies(devh, address, &dummy);
        uint32_t addressInSector = address + (usedCopiesCount - 1) * blockLength + offset;
        read_arbitrary_chunk(devh, addressInSector, data, lengthToRead);
    }
    
};

///////////////////
// SPI Interface //
///////////////////

SPI_Interface::SPI_Interface(uvc_device_handle_t * devh) : deviceHandle(devh)
{
    spi = new SPI();
}
    
SPI_Interface::~SPI_Interface()
{
    delete spi;
}

void SPI_Interface::Initialize()
{
    rst = {0};
    
    // Setup admin table
    spi->read_admin_table(deviceHandle,
                   NV_IFFLEY_ROUTINE_TABLE_ADDRESS_INDEX,
                   SIZEOF_ROUTINE_DESCRIPTION_ERASED_AND_PRESERVE_TABLE,
                   rst.rd,
                   ROUTINE_DESCRIPTION_OFFSET,
                   SIZEOF_ROUTINE_DESCRIPTION_TABLE);
    
    ReadCalibrationSector();
}

void SPI_Interface::ReadCalibrationSector()
{
    // Read flash data
    bool readStatus = spi->read_admin_sector(deviceHandle, cameraHeader, NV_CALIBRATION_DATA_ADDRESS_INDEX);
    
    if (!readStatus)
        throw std::runtime_error("Could not read calibration sector");
    
    // Copy useful parts into calibration data
    memcpy(calibrationData, cameraHeader, CAM_INFO_BLOCK_LEN);

    // Parse into something usable
    parameters = ParseCalibrationParameters(calibrationData);
}

RectifiedIntrinsics SPI_Interface::GetZIntrinsics(int mode)
{
    return parameters.modesLR[mode];
}

void SPI_Interface::PrintHeaderInfo()
{
    r200::CameraHeaderInfo cameraInfo = {};
    memcpy(&cameraInfo, cameraHeader + CAM_INFO_BLOCK_LEN, static_cast<int>(sizeof(cameraInfo)));
    
    std::cout << "######## Serial: " << cameraInfo.serialNumber << std::endl;
    std::cout << "######## Model: " << cameraInfo.modelNumber << std::endl;
    std::cout << "######## Revision: " << cameraInfo.revisionNumber << std::endl;
    std::cout << "######## Calibrated: " << cameraInfo.calibrationDate << std::endl;
    std::cout << "######## Head Version: " << cameraInfo.cameraHeadContentsVersion << std::endl;
    std::cout << "######## Baseline: " << cameraInfo.nominalBaseline << std::endl;
    std::cout << "######## OEM ID: " << cameraInfo.OEMID << std::endl;
}

} // end namespace r200