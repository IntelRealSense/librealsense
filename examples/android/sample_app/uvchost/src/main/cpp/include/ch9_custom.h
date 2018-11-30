/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef _UAPI__LINUX_USB_CH9_H
#define _UAPI__LINUX_USB_CH9_H
#include <linux/types.h>
#include <asm/byteorder.h>
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DIR_OUT 0
#define USB_DIR_IN 0x80
#define USB_TYPE_MASK (0x03 << 5)
#define USB_TYPE_STANDARD (0x00 << 5)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_TYPE_CLASS (0x01 << 5)
#define USB_TYPE_VENDOR (0x02 << 5)
#define USB_TYPE_RESERVED (0x03 << 5)
#define USB_RECIP_MASK 0x1f
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_RECIP_DEVICE 0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02
#define USB_RECIP_OTHER 0x03
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_RECIP_PORT 0x04
#define USB_RECIP_RPIPE 0x05
#define USB_REQ_GET_STATUS 0x00
#define USB_REQ_CLEAR_FEATURE 0x01
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_SET_FEATURE 0x03
#define USB_REQ_SET_ADDRESS 0x05
#define USB_REQ_GET_DESCRIPTOR 0x06
#define USB_REQ_SET_DESCRIPTOR 0x07
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE 0x0A
#define USB_REQ_SET_INTERFACE 0x0B
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_SYNCH_FRAME 0x0C
#define USB_REQ_SET_SEL 0x30
#define USB_REQ_SET_ISOCH_DELAY 0x31
#define USB_REQ_SET_ENCRYPTION 0x0D
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_GET_ENCRYPTION 0x0E
#define USB_REQ_RPIPE_ABORT 0x0E
#define USB_REQ_SET_HANDSHAKE 0x0F
#define USB_REQ_RPIPE_RESET 0x0F
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_GET_HANDSHAKE 0x10
#define USB_REQ_SET_CONNECTION 0x11
#define USB_REQ_SET_SECURITY_DATA 0x12
#define USB_REQ_GET_SECURITY_DATA 0x13
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_REQ_SET_WUSB_DATA 0x14
#define USB_REQ_LOOPBACK_DATA_WRITE 0x15
#define USB_REQ_LOOPBACK_DATA_READ 0x16
#define USB_REQ_SET_INTERFACE_DS 0x17
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DEVICE_SELF_POWERED 0
#define USB_DEVICE_REMOTE_WAKEUP 1
#define USB_DEVICE_TEST_MODE 2
#define USB_DEVICE_BATTERY 2
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DEVICE_B_HNP_ENABLE 3
#define USB_DEVICE_WUSB_DEVICE 3
#define USB_DEVICE_A_HNP_SUPPORT 4
#define USB_DEVICE_A_ALT_HNP_SUPPORT 5
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DEVICE_DEBUG_MODE 6
#define TEST_J 1
#define TEST_K 2
#define TEST_SE0_NAK 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define TEST_PACKET 4
#define TEST_FORCE_EN 5
#define USB_DEVICE_U1_ENABLE 48
#define USB_DEVICE_U2_ENABLE 49
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DEVICE_LTM_ENABLE 50
#define USB_INTRF_FUNC_SUSPEND 0
#define USB_INTR_FUNC_SUSPEND_OPT_MASK 0xFF00
#define USB_INTRF_FUNC_SUSPEND_LP (1 << (8 + 0))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_INTRF_FUNC_SUSPEND_RW (1 << (8 + 1))
#define USB_INTRF_STAT_FUNC_RW_CAP 1
#define USB_INTRF_STAT_FUNC_RW 2
#define USB_ENDPOINT_HALT 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DEV_STAT_U1_ENABLED 2
#define USB_DEV_STAT_U2_ENABLED 3
#define USB_DEV_STAT_LTM_ENABLED 4
struct usb_ctrlrequest {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bRequestType;
    __u8 bRequest;
    __le16 wValue;
    __le16 wIndex;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __le16 wLength;
} __attribute__ ((packed));
#define USB_DT_DEVICE 0x01
#define USB_DT_CONFIG 0x02
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_STRING 0x03
#define USB_DT_INTERFACE 0x04
#define USB_DT_ENDPOINT 0x05
#define USB_DT_DEVICE_QUALIFIER 0x06
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_OTHER_SPEED_CONFIG 0x07
#define USB_DT_INTERFACE_POWER 0x08
#define USB_DT_OTG 0x09
#define USB_DT_DEBUG 0x0a
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_INTERFACE_ASSOCIATION 0x0b
#define USB_DT_SECURITY 0x0c
#define USB_DT_KEY 0x0d
#define USB_DT_ENCRYPTION_TYPE 0x0e
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_BOS 0x0f
#define USB_DT_DEVICE_CAPABILITY 0x10
#define USB_DT_WIRELESS_ENDPOINT_COMP 0x11
#define USB_DT_WIRE_ADAPTER 0x21
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_RPIPE 0x22
#define USB_DT_CS_RADIO_CONTROL 0x23
#define USB_DT_PIPE_USAGE 0x24
#define USB_DT_SS_ENDPOINT_COMP 0x30
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_CS_DEVICE (USB_TYPE_CLASS | USB_DT_DEVICE)
#define USB_DT_CS_CONFIG (USB_TYPE_CLASS | USB_DT_CONFIG)
#define USB_DT_CS_STRING (USB_TYPE_CLASS | USB_DT_STRING)
#define USB_DT_CS_INTERFACE (USB_TYPE_CLASS | USB_DT_INTERFACE)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_DT_CS_ENDPOINT (USB_TYPE_CLASS | USB_DT_ENDPOINT)
struct usb_descriptor_header {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__ ((packed));
struct usb_device_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __le16 bcdUSB;
    __u8 bDeviceClass;
    __u8 bDeviceSubClass;
    __u8 bDeviceProtocol;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bMaxPacketSize0;
    __le16 idVendor;
    __le16 idProduct;
    __le16 bcdDevice;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 iManufacturer;
    __u8 iProduct;
    __u8 iSerialNumber;
    __u8 bNumConfigurations;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__ ((packed));
#define USB_DT_DEVICE_SIZE 18
#define USB_CLASS_PER_INTERFACE 0
#define USB_CLASS_AUDIO 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CLASS_COMM 2
#define USB_CLASS_HID 3
#define USB_CLASS_PHYSICAL 5
#define USB_CLASS_STILL_IMAGE 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CLASS_PRINTER 7
#define USB_CLASS_MASS_STORAGE 8
#define USB_CLASS_HUB 9
#define USB_CLASS_CDC_DATA 0x0a
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CLASS_CSCID 0x0b
#define USB_CLASS_CONTENT_SEC 0x0d
#define USB_CLASS_VIDEO 0x0e
#define USB_CLASS_WIRELESS_CONTROLLER 0xe0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CLASS_MISC 0xef
#define USB_CLASS_APP_SPEC 0xfe
#define USB_CLASS_VENDOR_SPEC 0xff
#define USB_SUBCLASS_VENDOR_SPEC 0xff
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_config_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __le16 wTotalLength;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bNumInterfaces;
    __u8 bConfigurationValue;
    __u8 iConfiguration;
    __u8 bmAttributes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bMaxPower;
} __attribute__ ((packed));
#define USB_DT_CONFIG_SIZE 9
#define USB_CONFIG_ATT_ONE (1 << 7)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CONFIG_ATT_SELFPOWER (1 << 6)
#define USB_CONFIG_ATT_WAKEUP (1 << 5)
#define USB_CONFIG_ATT_BATTERY (1 << 4)
struct usb_string_descriptor {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bLength;
    __u8 bDescriptorType;
    __le16 wData[1];
} __attribute__ ((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_interface_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bInterfaceNumber;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bAlternateSetting;
    __u8 bNumEndpoints;
    __u8 bInterfaceClass;
    __u8 bInterfaceSubClass;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bInterfaceProtocol;
    __u8 iInterface;
} __attribute__ ((packed));
#define USB_DT_INTERFACE_SIZE 9
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_endpoint_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bEndpointAddress;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bmAttributes;
    __le16 wMaxPacketSize;
    __u8 bInterval;
    __u8 bRefresh;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bSynchAddress;
} __attribute__ ((packed));
#define USB_DT_ENDPOINT_SIZE 7
#define USB_DT_ENDPOINT_AUDIO_SIZE 9
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_NUMBER_MASK 0x0f
#define USB_ENDPOINT_DIR_MASK 0x80
#define USB_ENDPOINT_XFERTYPE_MASK 0x03
#define USB_ENDPOINT_XFER_CONTROL 0
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_XFER_ISOC 1
#define USB_ENDPOINT_XFER_BULK 2
#define USB_ENDPOINT_XFER_INT 3
#define USB_ENDPOINT_MAX_ADJUSTABLE 0x80
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_INTRTYPE 0x30
#define USB_ENDPOINT_INTR_PERIODIC (0 << 4)
#define USB_ENDPOINT_INTR_NOTIFICATION (1 << 4)
#define USB_ENDPOINT_SYNCTYPE 0x0c
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_SYNC_NONE (0 << 2)
#define USB_ENDPOINT_SYNC_ASYNC (1 << 2)
#define USB_ENDPOINT_SYNC_ADAPTIVE (2 << 2)
#define USB_ENDPOINT_SYNC_SYNC (3 << 2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_USAGE_MASK 0x30
#define USB_ENDPOINT_USAGE_DATA 0x00
#define USB_ENDPOINT_USAGE_FEEDBACK 0x10
#define USB_ENDPOINT_USAGE_IMPLICIT_FB 0x20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_ss_ep_comp_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bMaxBurst;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bmAttributes;
    __le16 wBytesPerInterval;
} __attribute__ ((packed));
#define USB_DT_SS_EP_COMP_SIZE 6
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_SS_MULT(p) (1 + ((p) & 0x3))
struct usb_qualifier_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __le16 bcdUSB;
    __u8 bDeviceClass;
    __u8 bDeviceSubClass;
    __u8 bDeviceProtocol;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bMaxPacketSize0;
    __u8 bNumConfigurations;
    __u8 bRESERVED;
} __attribute__ ((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_otg_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bmAttributes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__ ((packed));
#define USB_OTG_SRP (1 << 0)
#define USB_OTG_HNP (1 << 1)
struct usb_debug_descriptor {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDebugInEndpoint;
    __u8 bDebugOutEndpoint;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
struct usb_interface_assoc_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bFirstInterface;
    __u8 bInterfaceCount;
    __u8 bFunctionClass;
    __u8 bFunctionSubClass;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bFunctionProtocol;
    __u8 iFunction;
} __attribute__ ((packed));
struct usb_security_descriptor {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bLength;
    __u8 bDescriptorType;
    __le16 wTotalLength;
    __u8 bNumEncryptionTypes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
struct usb_key_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 tTKID[3];
    __u8 bReserved;
    __u8 bKeyData[0];
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_encryption_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bEncryptionType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENC_TYPE_UNSECURE 0
#define USB_ENC_TYPE_WIRED 1
#define USB_ENC_TYPE_CCM_1 2
#define USB_ENC_TYPE_RSA_1 3
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bEncryptionValue;
    __u8 bAuthKeyIndex;
} __attribute__((packed));
struct usb_bos_descriptor {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bLength;
    __u8 bDescriptorType;
    __le16 wTotalLength;
    __u8 bNumDeviceCaps;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
#define USB_DT_BOS_SIZE 5
struct usb_dev_cap_header {
    __u8 bLength;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bDescriptorType;
    __u8 bDevCapabilityType;
} __attribute__((packed));
#define USB_CAP_TYPE_WIRELESS_USB 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_wireless_cap_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDevCapabilityType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bmAttributes;
#define USB_WIRELESS_P2P_DRD (1 << 1)
#define USB_WIRELESS_BEACON_MASK (3 << 2)
#define USB_WIRELESS_BEACON_SELF (1 << 2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_WIRELESS_BEACON_DIRECTED (2 << 2)
#define USB_WIRELESS_BEACON_NONE (3 << 2)
    __le16 wPHYRates;
#define USB_WIRELESS_PHY_53 (1 << 0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_WIRELESS_PHY_80 (1 << 1)
#define USB_WIRELESS_PHY_107 (1 << 2)
#define USB_WIRELESS_PHY_160 (1 << 3)
#define USB_WIRELESS_PHY_200 (1 << 4)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_WIRELESS_PHY_320 (1 << 5)
#define USB_WIRELESS_PHY_400 (1 << 6)
#define USB_WIRELESS_PHY_480 (1 << 7)
    __u8 bmTFITXPowerInfo;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bmFFITXPowerInfo;
    __le16 bmBandGroup;
    __u8 bReserved;
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_CAP_TYPE_EXT 2
struct usb_ext_cap_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bDevCapabilityType;
    __le32 bmAttributes;
#define USB_LPM_SUPPORT (1 << 1)
#define USB_BESL_SUPPORT (1 << 2)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_BESL_BASELINE_VALID (1 << 3)
#define USB_BESL_DEEP_VALID (1 << 4)
#define USB_GET_BESL_BASELINE(p) (((p) & (0xf << 8)) >> 8)
#define USB_GET_BESL_DEEP(p) (((p) & (0xf << 12)) >> 12)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
} __attribute__((packed));
#define USB_DT_USB_EXT_CAP_SIZE 7
#define USB_SS_CAP_TYPE 3
struct usb_ss_cap_descriptor {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDevCapabilityType;
    __u8 bmAttributes;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_LTM_SUPPORT (1 << 1)
    __le16 wSpeedSupported;
#define USB_LOW_SPEED_OPERATION (1)
#define USB_FULL_SPEED_OPERATION (1 << 1)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_HIGH_SPEED_OPERATION (1 << 2)
#define USB_5GBPS_OPERATION (1 << 3)
    __u8 bFunctionalitySupport;
    __u8 bU1devExitLat;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __le16 bU2DevExitLat;
} __attribute__((packed));
#define USB_DT_USB_SS_CAP_SIZE 10
#define CONTAINER_ID_TYPE 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_ss_container_id_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDevCapabilityType;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bReserved;
    __u8 ContainerID[16];
} __attribute__((packed));
#define USB_DT_USB_SS_CONTN_ID_SIZE 20
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_wireless_ep_comp_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bMaxBurst;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bMaxSequence;
    __le16 wMaxStreamDelay;
    __le16 wOverTheAirPacketSize;
    __u8 bOverTheAirInterval;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bmCompAttributes;
#define USB_ENDPOINT_SWITCH_MASK 0x03
#define USB_ENDPOINT_SWITCH_NO 0
#define USB_ENDPOINT_SWITCH_SWITCH 1
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_ENDPOINT_SWITCH_SCALE 2
} __attribute__((packed));
struct usb_handshake {
    __u8 bMessageNumber;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 bStatus;
    __u8 tTKID[3];
    __u8 bReserved;
    __u8 CDID[16];
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 nonce[16];
    __u8 MIC[8];
} __attribute__((packed));
struct usb_connection_context {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __u8 CHID[16];
    __u8 CDID[16];
    __u8 CK[16];
} __attribute__((packed));
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum usb_device_speed {
    USB_SPEED_UNKNOWN = 0,
    USB_SPEED_LOW, USB_SPEED_FULL,
    USB_SPEED_HIGH,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
            USB_SPEED_WIRELESS,
    USB_SPEED_SUPER,
};
enum usb_device_state {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
            USB_STATE_NOTATTACHED = 0,
    USB_STATE_ATTACHED,
    USB_STATE_POWERED,
    USB_STATE_RECONNECTING,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
            USB_STATE_UNAUTHENTICATED,
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESS,
    USB_STATE_CONFIGURED,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
            USB_STATE_SUSPENDED
};
enum usb3_link_state {
    USB3_LPM_U0 = 0,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
            USB3_LPM_U1,
    USB3_LPM_U2,
    USB3_LPM_U3
};
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB3_LPM_DISABLED 0x0
#define USB3_LPM_U1_MAX_TIMEOUT 0x7F
#define USB3_LPM_U2_MAX_TIMEOUT 0xFE
#define USB3_LPM_DEVICE_INITIATED 0xFF
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
struct usb_set_sel_req {
    __u8 u1_sel;
    __u8 u1_pel;
    __le16 u2_sel;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
    __le16 u2_pel;
} __attribute__ ((packed));
#define USB3_LPM_MAX_U1_SEL_PEL 0xFF
#define USB3_LPM_MAX_U2_SEL_PEL 0xFFFF
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define USB_SELF_POWER_VBUS_MAX_DRAW 100
#endif
