// winusb_uvc.cpp : Defines the entry point for the console application.
//
#include "windows.h"
#include "tchar.h"
#include "utlist.h"
#include "SETUPAPI.H"
#include "winusb_internal.h"
#include "parser.h"
#include "winusb_uvc.h"

bool wait_for_async_operation(WINUSB_INTERFACE_HANDLE interfaceHandle, 
    int ep, OVERLAPPED &hOvl, ULONG &lengthTransferred, USHORT timeout)
{
    if (GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE))
        return true;

    auto lastResult = GetLastError();
    if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
    {
        WaitForSingleObject(hOvl.hEvent, timeout);
        auto res = GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE);
        if (res != 1)
        {
            return false;
        }
    }
    else
    {
        lengthTransferred = 0;
        WinUsb_ResetPipe(interfaceHandle, ep);
        return false;
    }

    return true;
}

void poll_interrupts(WINUSB_INTERFACE_HANDLE handle, int ep, uint16_t timeout)
{
    static const unsigned short interrupt_buf_size = 0x400;
    uint8_t buffer[interrupt_buf_size];                         /* 64 byte transfer buffer  - dedicated channel*/
    ULONG num_bytes = 0;                                        /* Actual bytes transferred. */
    OVERLAPPED hOvl;
    safe_handle sh(CreateEvent(nullptr, false, false, nullptr));
    hOvl.hEvent = sh.GetHandle();
    int res = WinUsb_ReadPipe(handle, ep, buffer, interrupt_buf_size, &num_bytes, &hOvl);
    if (0 == res)
    {
        auto lastError = GetLastError();
        if (lastError == ERROR_IO_PENDING)
        {
            auto sts = wait_for_async_operation(handle, ep, hOvl, num_bytes, timeout);
            lastError = GetLastError();
            if (lastError == ERROR_OPERATION_ABORTED)
            {
                perror("receiving interrupt_ep bytes failed");
                fprintf(stderr, "Error receiving message.\n");
            }
            if (!sts)
                return;
        }
        else
        {
            WinUsb_ResetPipe(handle, ep);
            perror("receiving interrupt_ep bytes failed");
            fprintf(stderr, "Error receiving message.\n");
            return;
        }

        if (num_bytes == 0)
            return;

        // TODO: Complete XU set instead of using retries
    }
    else
    {
        // todo exception
        perror("receiving interrupt_ep bytes failed");
        fprintf(stderr, "Error receiving message.\n");
    }
}

int uvc_get_ctrl_len(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl) {
    unsigned char buf[2];

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    int ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET,
        UVC_GET_LEN,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,
        buf,
        2);

    if (ret < 0)
        return ret;
    else
        return (unsigned short)SW_TO_SHORT(buf);
}

int uvc_get_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    return winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET, req_code,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,		// XXX saki
        static_cast<unsigned char*>(data),
        len);
}

int uvc_set_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    return winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_SET, UVC_SET_CUR,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,		// XXX saki
        static_cast<unsigned char*>(data),
        len);
}

uvc_error_t uvc_get_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code) {
    uint8_t mode_char;
    int ret;
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET, req_code,
        UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
        devh->deviceData.ctrl_if.bInterfaceNumber,
        (unsigned char *)&mode_char,
        sizeof(mode_char));

    if (ret == 1) {
        *mode = static_cast<uvc_device_power_mode>(mode_char);
        return UVC_SUCCESS;
    }
    else {
        return static_cast<uvc_error_t>(ret);
    }
}

uvc_error_t uvc_set_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode mode) {
    uint8_t mode_char = mode;
    int ret;

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_SET, UVC_SET_CUR,
        UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
        devh->deviceData.ctrl_if.bInterfaceNumber,
        (unsigned char *)&mode_char,
        sizeof(mode_char));

    if (ret == 1)
        return UVC_SUCCESS;
    else
        return static_cast<uvc_error_t>(ret);
}

// Sending control packet using vendor-defined control transfer directed to WinUSB interface
// WinUSB Setup packet info is according to Universal Serial Bus Specification:
// RequestType: Bits[4..0] - Receipient: device (0) interface (1) endpoint (2) other (3), 
//              Bits[6..5] - Type: Standard (0) Class (1) Vendor Specific (2) Reserved (3)
//              Bit [7]    - Direction: Host-to-device (0), Device-to-host (1)
// Request: Vendor defined request number
// Value: Vendor defined Value number (0x0000-0xFFFF)
// Index: If RequestType is directed to the device, index field is available for any vendor use.
//        If RequestType is directed to an interface, the WinUSB driver passes the interface number in the low byte of index so only the high byte is available for vendor use.
//        If RequestType is directed to an endpoint, index lower byte must be the endpoint address.
// Length: Number of bytes to transfer (0x0000-0xFFFF)
int winusb_SendControl(WINUSB_INTERFACE_HANDLE ihandle, int requestType, int request, int value, int index, unsigned char *buffer, int buflen)
{
    WINUSB_SETUP_PACKET setupPacket;
    ULONG lengthOutput;

    setupPacket.RequestType = requestType;
    setupPacket.Request = request;
    setupPacket.Value = value;
    setupPacket.Index = index;
    setupPacket.Length = buflen;

    if (!WinUsb_ControlTransfer(ihandle, setupPacket, buffer, buflen, &lengthOutput, NULL))
    {
        return -1;
    }
    else
    {
        return lengthOutput;
    }

    return 0;
}

uvc_error_t winusb_find_devices(const std::string &uvc_interface, int vid, int pid, winusb_uvc_device ***devs, int& devs_count)
{
    GUID guid;
    std::wstring guidWStr(uvc_interface.begin(), uvc_interface.end());
    CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid));

    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA DevData;
    winusb_uvc_device **list_internal;
    int num_uvc_devices = 0;
    DWORD dwSize = 0;
    DWORD dwMemberIdx = 0;
    uvc_error_t ret = UVC_SUCCESS;

    list_internal = (winusb_uvc_device **)malloc(sizeof(*list_internal));
    *list_internal = NULL;

    // Return a handle to device information set with connected IVCAM device
    hDevInfo = SetupDiGetClassDevs(&guid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        dwMemberIdx = 0;

        // Enumerates IVCAM device
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dwMemberIdx, &DevIntfData);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(2048);

        while (GetLastError() != ERROR_NO_MORE_ITEMS)
        {
            DevData.cbSize = sizeof(DevData);

            // Returns required buffer size for saving device interface details
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            // Returns details about connected device
            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData) == TRUE)
            {

                // Create a handle for I/O operations to the IVCAM device
                HANDLE Devicehandle = CreateFile(DevIntfDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    WINUSB_INTERFACE_HANDLE winusbHandle;

                    // Create WinUSB device handle for the IVCAM device
                    if (WinUsb_Initialize(Devicehandle, &winusbHandle) == TRUE)
                    {
                        USB_DEVICE_DESCRIPTOR deviceDescriptor;
                        ULONG returnLength;

                        // Returns IVCAM device descriptor which includes PID/VID info
                        if (!WinUsb_GetDescriptor(winusbHandle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), &returnLength))
                        {
                            printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
                            ret = UVC_ERROR_INVALID_PARAM;
                        }
                        else
                        {
                            if (deviceDescriptor.idVendor == vid && deviceDescriptor.idProduct == pid)
                            {
                                winusb_uvc_device *winUsbDevice = new winusb_uvc_device;
                                winUsbDevice->deviceHandle = NULL;
                                winUsbDevice->winusbHandle = NULL;
                                winUsbDevice->pid = pid;
                                winUsbDevice->vid = vid;
                                size_t devicePathLength = wcslen(DevIntfDetailData->DevicePath);
                                winUsbDevice->devPath = new WCHAR[devicePathLength + 1];
                                wcscpy_s(winUsbDevice->devPath, devicePathLength + 1, DevIntfDetailData->DevicePath);

                                num_uvc_devices++;
                                list_internal = (winusb_uvc_device **)realloc(list_internal, (num_uvc_devices + 1) * sizeof(*list_internal));
                                list_internal[num_uvc_devices - 1] = winUsbDevice;
                                list_internal[num_uvc_devices] = NULL;
                            }
                        }

                        WinUsb_Free(winusbHandle);
                    }
                    else
                    {
                        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
                        ret = UVC_ERROR_INVALID_PARAM;
                    }
                }
                else
                {
                    auto error = GetLastError();
                    printf("CreateFile failed - GetLastError = %d\n", GetLastError());
                    ret = UVC_ERROR_INVALID_PARAM;
                }

                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(Devicehandle);
                }
            }
            else
            {
                printf("SetupDiGetDeviceInterfaceDetail failed - GetLastError = %d\n", GetLastError());
                ret = UVC_ERROR_INVALID_PARAM;
            }

            // Continue looping on all connected IVCAM devices
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, ++dwMemberIdx, &DevIntfData);
        }

        if (DevIntfDetailData != NULL)
        {
            free(DevIntfDetailData);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

        *devs = list_internal;
        devs_count = num_uvc_devices;
    }
    else
    {
        printf("SetupDiGetClassDevs failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
    }

    return ret;
}


// Return list of all connected IVCAM devices
uvc_error_t winusb_find_devices(winusb_uvc_device ***devs, int vid, int pid)
{
    // Intel(R) RealSense(TM) 415 Depth - MI 0
    // bInterfaceNumber 0 video control - endpoint 0x87 (FW->Host)
    // bInterfaceNumber 1 video stream - endpoint 0x82 (FW->Host)
    // bInterfaceNumber 2 video stream - endpoint 0x83 (FW->Host)
    //const GUID IVCAM_WIN_USB_DEVICE_GUID = { 0xe659c3ec, 0xbf3c, 0x48a5,{ 0x81, 0x92, 0x30, 0x73, 0xe8, 0x22, 0xd7, 0xcd } }; 

    // Intel(R) RealSense(TM) 415 RGB - MI 3
    // bInterfaceNumber 3 video control
    // bInterfaceNumber 4 video stream - endpoint 0x84 (FW->Host)
    const GUID IVCAM_WIN_USB_DEVICE_GUID = { 0x50537bc3, 0x2919, 0x452d,{ 0x88, 0xa9, 0xb1, 0x3b, 0xbf, 0x7d, 0x24, 0x59 } };

    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA DevData;
    winusb_uvc_device **list_internal;
    int num_uvc_devices = 0;
    DWORD dwSize = 0;
    DWORD dwMemberIdx = 0;
    uvc_error_t ret = UVC_SUCCESS;

    list_internal = (winusb_uvc_device **)malloc(sizeof(*list_internal));
    *list_internal = NULL;

    // Return a handle to device information set with connected IVCAM device
    hDevInfo = SetupDiGetClassDevs(&IVCAM_WIN_USB_DEVICE_GUID, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        dwMemberIdx = 0;

        // Enumerates IVCAM device
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &IVCAM_WIN_USB_DEVICE_GUID, dwMemberIdx, &DevIntfData);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(2048);

        while (GetLastError() != ERROR_NO_MORE_ITEMS)
        {
            DevData.cbSize = sizeof(DevData);

            // Returns required buffer size for saving device interface details
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            // Returns details about connected device
            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData) == TRUE)
            {

                // Create a handle for I/O operations to the IVCAM device
                HANDLE Devicehandle = CreateFile(DevIntfDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    WINUSB_INTERFACE_HANDLE winusbHandle;

                    // Create WinUSB device handle for the IVCAM device
                    if (WinUsb_Initialize(Devicehandle, &winusbHandle) == TRUE)
                    {
                        USB_DEVICE_DESCRIPTOR deviceDescriptor;
                        ULONG returnLength;

                        // Returns IVCAM device descriptor which includes PID/VID info
                        if (!WinUsb_GetDescriptor(winusbHandle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), &returnLength))
                        {
                            printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
                            ret = UVC_ERROR_INVALID_PARAM;
                        }
                        else
                        {
                            if (deviceDescriptor.idVendor == vid && deviceDescriptor.idProduct == pid)
                            {
                                winusb_uvc_device *winUsbDevice = new winusb_uvc_device;
                                winUsbDevice->deviceHandle = NULL;
                                winUsbDevice->winusbHandle = NULL;
                                winUsbDevice->pid = pid;
                                winUsbDevice->vid = vid;
                                size_t devicePathLength = wcslen(DevIntfDetailData->DevicePath);
                                winUsbDevice->devPath = new WCHAR[devicePathLength + 1];
                                wcscpy_s(winUsbDevice->devPath, devicePathLength + 1, DevIntfDetailData->DevicePath);

                                num_uvc_devices++;
                                list_internal = (winusb_uvc_device **)realloc(list_internal, (num_uvc_devices + 1) * sizeof(*list_internal));

                                list_internal[num_uvc_devices - 1] = winUsbDevice;
                                list_internal[num_uvc_devices] = NULL;
                            }
                        }

                        WinUsb_Free(winusbHandle);
                    }
                    else
                    {
                        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
                        ret = UVC_ERROR_INVALID_PARAM;
                    }
                }
                else
                {
                    printf("CreateFile failed - GetLastError = %d\n", GetLastError());
                    ret = UVC_ERROR_INVALID_PARAM;
                }

                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(Devicehandle);
                }
            }
            else
            {
                printf("SetupDiGetDeviceInterfaceDetail failed - GetLastError = %d\n", GetLastError());
                ret = UVC_ERROR_INVALID_PARAM;
            }

            // Continue looping on all connected IVCAM devices
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &IVCAM_WIN_USB_DEVICE_GUID, ++dwMemberIdx, &DevIntfData);
        }

        if (DevIntfDetailData != NULL)
        {
            free(DevIntfDetailData);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

        *devs = list_internal;
    }
    else
    {
        printf("SetupDiGetClassDevs failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
    }

    return ret;
}

uvc_error_t winusb_open(winusb_uvc_device *device)
{
    USB_CONFIGURATION_DESCRIPTOR cfgDesc;
    WINUSB_INTERFACES* interfaces = NULL;
    UCHAR *descriptors = NULL;
    uvc_error_t ret = UVC_SUCCESS;
    ULONG returnLength = 0;

    device->associateHandle = NULL;
    device->streams = NULL;

    // Create a handle for I/O operations to the IVCAM device
    device->deviceHandle = CreateFile(device->devPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (device->deviceHandle == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    // Create WinUSB device handle for the IVCAM device
    if (!WinUsb_Initialize(device->deviceHandle, &device->winusbHandle))
    {
        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    // Returns IVCAM configuration descriptor
    if (!WinUsb_GetDescriptor(device->winusbHandle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&cfgDesc, sizeof(cfgDesc), &returnLength))
    {
        printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    descriptors = new UCHAR[cfgDesc.wTotalLength];

    // Returns IVCAM configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
    if (!WinUsb_GetDescriptor(device->winusbHandle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, descriptors, cfgDesc.wTotalLength, &returnLength))
    {
        printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }
    memset(&device->deviceData, 0, sizeof(uvc_device_info_t));
    device->deviceData.config = cfgDesc;

    // Iterate over all descriptors and parse all Interface and Endpoint descriptors
    ParseConfigDescriptors((USB_CONFIGURATION_DESCRIPTOR *)descriptors, &interfaces);
    device->deviceData.interfaces = interfaces;

    if (descriptors)
    {
        delete descriptors;
        descriptors = NULL;
    }

    // Fill fields of uvc_device_info on device
    ret = uvc_scan_control(device, &device->deviceData);
    if (ret != UVC_SUCCESS)
    {
        printf("uvc_scan_control failed\n");
        goto fail;
    }

    return ret;

fail:

    if (descriptors)
    {
        delete descriptors;
    }

    if (device)
    {
        winusb_free_device_info(&device->deviceData);

        if (device->deviceHandle)
        {
            CloseHandle(device->deviceHandle);
            device->deviceHandle = NULL;
        }

        if (device->winusbHandle)
        {
            WinUsb_Free(device->winusbHandle);
            device->winusbHandle = NULL;
        }
    }

    return ret;
}


uvc_error_t winusb_close(winusb_uvc_device *device)
{
    uvc_error_t ret = UVC_SUCCESS;

    if (device != NULL)
    {
        winusb_free_device_info(&device->deviceData);
        memset(&device->deviceData, 0, sizeof(uvc_device_info_t));

        if (device->winusbHandle != NULL)
        {
            WinUsb_Free(device->winusbHandle);
            device->winusbHandle = NULL;
        }

        if (device->associateHandle != NULL)
        {
            WinUsb_Free(device->associateHandle);
            device->associateHandle = NULL;
        }

        if (device->deviceHandle != NULL)
        {
            CloseHandle(device->deviceHandle);
            device->deviceHandle = NULL;
        }

        device->streams = NULL;

        free(device->devPath);
        free(device);
    }
    else
    {
        printf("Error: device == NULL\n");
        ret = UVC_ERROR_NO_DEVICE;
    }

    return ret;
}

int winusb_init() {
    return 0;
}

void winusb_deinit() {

}

bool read_all_uvc_descriptors(winusb_uvc_device *device, PUCHAR buffer, ULONG bufferLength, PULONG lengthReturned) 
{
    UCHAR b[2048];
    ULONG returnLength;

    /*WinUsb_GetAssociatedInterface(device->deviceHande, 1
        , &iHandle);*/

    if (!WinUsb_GetDescriptor(device->deviceHandle,
        USB_CONFIGURATION_DESCRIPTOR_TYPE,
        0,
        0,
        b,
        2048,
        &returnLength))
    {
        printf("GetLastError = %d\n", GetLastError());
    }

    printf("success");

    return 0;
}
