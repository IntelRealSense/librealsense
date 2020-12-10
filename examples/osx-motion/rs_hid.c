#include "rs_hid.h"

#include <string.h>

rs_hid_device_info *RS_hid_Enumerate(unsigned short vendorId, unsigned short ProductId) {
    return (rs_hid_device_info *) hid_enumerate(vendorId, ProductId);
}

int RS_hid_PowerDevice(rs_hid_device *hidDevice, unsigned char reportId) {
    
    struct REALSENSE_FEATURE_REPORT featureReport;
    int ret;
    
    memset(&featureReport,0, sizeof(featureReport));
    featureReport.reportId = reportId;
    // Reading feature report.
    ret = hid_get_feature_report(hidDevice, (unsigned char *)
                                 &featureReport ,sizeof(featureReport) );
    
    if (ret == -1) {
        printf("fail to read feature report from device\n");
        return ret;
    }
    
    // change report to power the device to D0
    
    featureReport.power = DEVICE_POWER_D0;
    
    // Write feature report back.
    ret = hid_send_feature_report(hidDevice, &featureReport, sizeof(featureReport));
    
    if (ret == -1) {
        printf("fail to write feature report from device\n");
        return ret;
    }
    
    ret = hid_get_feature_report(hidDevice, (unsigned char *)
                                 &featureReport ,sizeof(featureReport) );
    
    if (ret == -1) {
        printf("fail to read feature report from device\n");
        return ret;
    }
    
    if (featureReport.power == DEVICE_POWER_D0) {
        printf("Device is powered up\n");
    } else {
        printf("Device is powered off. in state : %d\n", featureReport.power);
        return -1;
    }
    
    return 0;
}

rs_hid_device *RS_hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number)
{
    return hid_open(vendor_id, product_id, serial_number);
}

int RS_hid_read(rs_hid_device *dev, unsigned char *data, size_t length){
    return hid_read(dev, data, length);
}

void RS_hid_close(rs_hid_device *dev) {
    return hid_close(dev);
}
