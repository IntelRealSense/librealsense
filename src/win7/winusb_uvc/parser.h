#ifndef PARSER_H_
#define PARSER_H_

void ParseConfigDescriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, WINUSB_INTERFACES **interfaces);
void FreeWinusbInterfaces(WINUSB_INTERFACES *interfaces);
void winusb_free_device_info(uvc_device_info_t *info);
uvc_error_t uvc_scan_control(winusb_uvc_device *dev, uvc_device_info_t *info);
#endif