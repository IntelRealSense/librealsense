/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "Common.h"
#include "libtm_message.h"


#define DEV_PID 0x2150
#define DEV_VID 0x03e7

#define EP_OUT 1

#define TIMEOUT_MS 60000
#define DEFAULT_CHUNKSZ 1024

#define DEV_INTERFACE 0

#define USB_ERR_NONE		0
#define USB_ERR_TIMEOUT		-1
#define USB_ERR_FAILED		-2
#define USB_ERR_INVALID		-3

namespace tracking
{
    typedef struct timespec highres_time_t;

    class loader
    {
    public:
        loader() : m_dev_handle(0), m_pid(DEV_PID), m_vid(DEV_VID)
        {
        }

        void set_pid(int pid) { m_pid = pid; }
        void set_vid(int vid) { m_vid = vid; }

        Status load_image(char * filename);

    private:

        Status send_file(const char *fname);

        libusb_device_handle *m_dev_handle;

        int m_pid, m_vid;
    };
}
