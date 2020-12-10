#include "hidapi.h"

typedef struct hid_device rs_hid_device;
typedef struct hid_device_info rs_hid_device_info;

#define DEVICE_POWER_D0 2
#define DEVICE_POWER_D4 6

#define REPORT_ID_ACCELEROMETER_3D 1
#define REPORT_ID_GYROMETER_3D     2

#pragma pack(1)

struct REALSENSE_FEATURE_REPORT {
    unsigned char reportId;
    unsigned char connectionType;
    unsigned char sensorState;
    unsigned char power;
    unsigned char minReport;
    unsigned short report;
    unsigned short unknown;
};

struct REALSENSE_GYROMETER_REPORT {
    unsigned char reportId;
    unsigned char unknown;
    unsigned long timeStamp;
    unsigned short velocityX;
    unsigned short velocityY;
    unsigned short velocityZ;
    unsigned int customValue1;
    unsigned int customValue2;
    unsigned short customValue3;
    unsigned short customValue4;
    unsigned short customValue5;
    unsigned char customValue6;
    unsigned char customValue7;
};

struct REALSENSE_ACCELEROMETER_REPORT {
    unsigned char reportId;
    unsigned char unknown;
    unsigned long timeStamp;
    unsigned short accelerationAxisX;
    unsigned short accelerationAxisY;
    unsigned short accelerationAxisZ;
    unsigned int customValue1;
    unsigned int customValue2;
    unsigned short customValue3;
    unsigned short customValue4;
    unsigned short customValue5;
    unsigned char customValue6;
    unsigned char customValue7;
};

struct HID_REPORT {
    unsigned char reportId;
};

rs_hid_device_info *RS_hid_Enumerate(unsigned short vendorId, unsigned short ProductId);

int RS_hid_PowerDevice(rs_hid_device *hidDevice, unsigned char reportId);

rs_hid_device *RS_hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number);

int RS_hid_read(rs_hid_device *dev, unsigned char *data, size_t length);

void RS_hid_close(rs_hid_device *dev);

