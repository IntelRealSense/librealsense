#ifndef PARSER_H_
#define PARSER_H_

void ParseConfigDescriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, winusb_uvc_interfaces **interfaces);
void FreeWinusbInterfaces(winusb_uvc_interfaces *interfaces);
void winusb_free_device_info(winusb_uvc_device_info_t *info);
uvc_error_t uvc_scan_control(winusb_uvc_device *dev, winusb_uvc_device_info_t *info);
#endif