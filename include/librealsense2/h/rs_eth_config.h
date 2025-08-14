/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2025 RealSense, Inc. All Rights Reserved. */

/** \file rs_eth_config.h
 * \brief
 * Exposes RealSense ethernet configuration functionality for C compilers
 */

#ifndef LIBREALSENSE_RS2_ETH_CONFIG_H
#define LIBREALSENSE_RS2_ETH_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "rs_types.h"

/** \brief Ethernet link priority options */
typedef enum rs2_eth_link_priority
{
    RS2_LINK_PRIORITY_USB_ONLY = 0,  /**< Use USB connection only */
    RS2_LINK_PRIORITY_ETH_ONLY = 1,  /**< Use Ethernet connection only */
    RS2_LINK_PRIORITY_ETH_FIRST = 2, /**< Prefer Ethernet, fallback to USB */
    RS2_LINK_PRIORITY_USB_FIRST = 3, /**< Prefer USB, fallback to Ethernet */

    RS2_LINK_PRIORITY_DYNAMIC_BIT = 0x10, /**< Dynamic switching bit flag */
    RS2_LINK_PRIORITY_DYNAMIC_ETH_FIRST = RS2_LINK_PRIORITY_ETH_FIRST | RS2_LINK_PRIORITY_DYNAMIC_BIT, /**< Dynamic Ethernet-first priority */
    RS2_LINK_PRIORITY_DYNAMIC_USB_FIRST = RS2_LINK_PRIORITY_USB_FIRST | RS2_LINK_PRIORITY_DYNAMIC_BIT, /**< Dynamic USB-first priority */
    RS2_LINK_PRIORITY_COUNT /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_eth_link_priority;

const char * rs2_eth_link_priority_to_string( rs2_eth_link_priority priority );

/** \brief IP address structure for IPv4 addresses */
typedef uint8_t rs2_ip_address[4];


// Device capability check functions
/**
    * Check if device supports ethernet configuration
    * \param[in] device    RealSense device
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return              Non-zero if device supports ethernet configuration, zero otherwise
    */
int rs2_supports_eth_config( const rs2_device * device, rs2_error ** error );

/**
    * Get Ethernet link speed
    * \param[in] device    RealSense device
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return              Current Ethernet link speed Mbps. 0 if link is not available.
    */
int rs2_get_link_speed( const rs2_device * device, rs2_error ** error );

/**
    * Get link priority
    * \param[in] device    RealSense device
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return              Current link priority setting
    */
rs2_eth_link_priority rs2_get_link_priority( const rs2_device * device, rs2_error ** error );

/**
    * Set link priority
    * \param[in] device    RealSense device
    * \param[in] priority  Link priority to set
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_link_priority( const rs2_device * device, rs2_eth_link_priority priority, rs2_error ** error );

/**
    * Get IP address
    * \param[in] device    RealSense device
    * \param[out] configured_ip  Configured IP address to populate
    * \param[out] actual_ip      Actual IP address to populate. Might be different from configured_ip if DHCP is enabled
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_get_ip_address( const rs2_device * device, rs2_ip_address configured_ip, rs2_ip_address actual_ip, rs2_error ** error );

/**
    * Set IP address
    * \param[in] device    RealSense device
    * \param[in] ip        IP address to set
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_ip_address( const rs2_device * device, const rs2_ip_address ip, rs2_error ** error );

/**
    * Get network mask
    * \param[in] device    RealSense device
    * \param[out] configured_netmask  Configured network mask to populate
    * \param[out] actual_netmask      Actual network mask to populate. Might be different from configured_ip if DHCP is enabled
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_get_netmask( const rs2_device * device, rs2_ip_address configured_netmask, rs2_ip_address actual_netmask, rs2_error ** error );

/**
    * Set network mask
    * \param[in] device    RealSense device
    * \param[in] netmask   Network mask to set
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_netmask( const rs2_device * device, const rs2_ip_address netmask, rs2_error ** error );

/**
    * Get gateway address
    * \param[in] device    RealSense device
    * \param[out] configured_gateway  Configured gateway address to populate
    * \param[out] actual_gateway      Actual gateway address to populate. Might be different from configured_ip if DHCP is enabled
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_get_gateway( const rs2_device * device, rs2_ip_address configured_gateway, rs2_ip_address actual_gateway, rs2_error ** error );

/**
    * Set gateway address
    * \param[in] device    RealSense device
    * \param[in] gateway   Gateway address to set
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_gateway( const rs2_device * device, const rs2_ip_address gateway, rs2_error ** error );

/**
    * Get DHCP configuration
    * \param[in] device    RealSense device
    * \param[out] enabled  DHCP enabled flag (0=off, non-zero=on)
    * \param[out] timeout  DHCP timeout threshold in seconds (0-255)
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_get_dhcp_config( const rs2_device * device, int * enabled, unsigned int * timeout, rs2_error ** error );

/**
    * Set DHCP configuration
    * \param[in] device    RealSense device
    * \param[in] enabled   DHCP enabled flag (0=off, non-zero=on)
    * \param[in] timeout   DHCP timeout threshold in seconds (0-255)
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_dhcp_config( const rs2_device * device, int enabled, unsigned int timeout, rs2_error ** error );

/**
    * Get MTU (Maximum Transmission Unit)
    * \param[in] device    RealSense device
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return              Current MTU in bytes
    */
unsigned int rs2_get_mtu( const rs2_device * device, rs2_error ** error );

/**
    * Set MTU (Maximum Transmission Unit)
    * \param[in] device    RealSense device
    * \param[in] mtu       MTU in bytes (500-9000, in 500 byte steps)
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_mtu( const rs2_device * device, unsigned int mtu, rs2_error ** error );

/**
    * Get transmission delay
    * \param[in] device    RealSense device
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return              Current transmission delay in microseconds
    */
unsigned int rs2_get_transmission_delay( const rs2_device * device, rs2_error ** error );

/**
    * Set transmission delay
    * \param[in] device    RealSense device
    * \param[in] delay     Transmission delay in microseconds (0-144, in 3us steps)
    * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
void rs2_set_transmission_delay( const rs2_device * device, unsigned int delay, rs2_error ** error );


#ifdef __cplusplus
}
#endif

#endif /* LIBREALSENSE_RS2_ETH_CONFIG_H */
