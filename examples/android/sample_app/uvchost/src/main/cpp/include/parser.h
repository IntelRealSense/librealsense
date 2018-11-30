#pragma once

void ParseConfigDescriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, WINUSB_INTERFACES **interfaces);
void FreeUsbInterfaces(WINUSB_INTERFACES *interfaces);
void FreeStreamInterfaces(uvc_streaming_interface_t * streamInterfaces);
uvc_error_t uvc_scan_control(usb_uvc_device *dev, uvc_device_info_t *info);
