#ifndef PARSER_H_
#define PARSER_H_

void ParseConfigDescriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, WINUSB_INTERFACES **interfaces);
void FreeInterfaces(WINUSB_INTERFACES *interfaces);
uvc_error_t uvc_scan_control(winusb_uvc_device *dev, uvc_device_info_t *info);
#endif