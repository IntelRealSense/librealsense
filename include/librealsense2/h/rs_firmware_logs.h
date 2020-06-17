/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

/** \file rs_firmware_logs.h
* \brief Exposes RealSense firmware logs functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_FIRMWARE_LOGS_H
#define LIBREALSENSE_RS2_FIRMWARE_LOGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

/**
* \brief Creates RealSense firmware log message.
* \param[in] dev	        Device from which the FW log will be taken using the created message
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   pointer to created empty firmware log message
*/
rs2_firmware_log_message* rs2_create_fw_log_message(rs2_device* dev, rs2_error** error);

/**
* \brief Gets RealSense firmware log.
* \param[in] dev	        Device from which the FW log should be taken
* \param[in] fw_log_msg	    Firmware log message object to be filled
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   true for success, false for failure - failure happens if no firmware log was sent by the hardware monitor
*/
int rs2_get_fw_log(rs2_device* dev, rs2_firmware_log_message** fw_log_msg, rs2_error** error);

/**
* \brief Gets RealSense flash log - this is a fw log that has been written in the device during the previous shutdown of the device
* \param[in] dev	        Device from which the FW log should be taken
* \param[in] fw_log_msg	    Firmware log message object to be filled
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   true for success, false for failure - failure happens if no firmware log was sent by the hardware monitor
*/
int rs2_get_flash_log(rs2_device* dev, rs2_firmware_log_message** fw_log_msg, rs2_error** error);

/**
* Delete RealSense firmware log message
* \param[in]  device    Realsense firmware log message to delete
*/
void rs2_delete_fw_log_message(rs2_firmware_log_message* msg);

/**
* \brief Gets RealSense firmware log message data.
* \param[in] msg	    firmware log message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               pointer to start of the firmware log message data
*/
const unsigned char* rs2_fw_log_message_data(rs2_firmware_log_message* msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log message size.
* \param[in] msg	    firmware log message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               size of the firmware log message data
*/
int rs2_fw_log_message_size(rs2_firmware_log_message* msg, rs2_error** error);


/**
* \brief Gets RealSense firmware log message timestamp.
* \param[in] msg	    firmware log message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               timestamp of the firmware log message
*/
unsigned int rs2_firmware_log_message_timestamp(rs2_firmware_log_message* msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log message severity.
* \param[in] msg	    firmware log message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               severity of the firmware log message data
*/
rs2_log_severity rs2_fw_log_message_timestamp(const rs2_firmware_log_message* msg, rs2_error** error);

/**
* \brief Initializes RealSense firmware logs parser in device.
* \param[in] dev	        Device from which the FW log will be taken
* \param[in] xml_content	content of the xml file needed for parsing
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   true for success, false for failure - failure happens if opening the xml from the xml_path input fails
*/
int rs2_init_fw_log_parser(rs2_device* dev, const char* xml_content, rs2_error** error);


/**
* \brief Creates RealSense firmware log parsed message.
* \param[in] dev	        Device from which the FW log will be taken using the created message
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   pointer to created empty firmware log message
*/
rs2_firmware_log_parsed_message* rs2_create_fw_log_parsed_message(rs2_device* dev, rs2_error** error);

/**
* \brief Deletes RealSense firmware log parsed message.
* \param[in] msg	        message to be deleted
*/
void rs2_delete_fw_log_parsed_message(rs2_firmware_log_parsed_message* fw_log_parsed_msg);


/**
* \brief Gets RealSense firmware log parser
* \param[in] dev	            Device from which the FW log will be taken
* \param[in] fw_log_msg	        firmware log message to be parsed
* \param[in] parsed_msg	        firmware log parsed message - place holder for the resulting parsed message
* \param[out] error             If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                       true for success, false for failure - failure happens if message could not be parsed
*/
int rs2_parse_firmware_log(rs2_device* dev, rs2_firmware_log_message* fw_log_msg, rs2_firmware_log_parsed_message* parsed_msg, rs2_error** error);

/**
* Delete RealSense firmware log parsed message
* \param[in]  device    Realsense firmware log parsed message to delete
*/
void rs2_delete_fw_log_parsed_message(rs2_firmware_log_parsed_message* fw_log_parsed_msg);

/**
* \brief Gets RealSense firmware log parsed message message.
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               message of the firmware log parsed message
*/
const char* rs2_get_fw_log_parsed_message(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log parsed message file name.
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return               file name of the firmware log parsed message
*/
const char* rs2_get_fw_log_parsed_file_name(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log parsed message thread name.
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           thread name of the firmware log parsed message
*/
const char* rs2_get_fw_log_parsed_thread_name(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log parsed message severity.
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           severity of the firmware log parsed message
*/
rs2_log_severity rs2_get_fw_log_parsed_severity(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log parsed message relevant line (in the file that is returned by rs2_get_fw_log_parsed_file_name).
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           line number of the firmware log parsed message
*/
unsigned int rs2_get_fw_log_parsed_line(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

/**
* \brief Gets RealSense firmware log parsed message timestamp
* \param[in] fw_log_parsed_msg	    firmware log parsed message object
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           timestamp of the firmware log parsed message
*/
unsigned int rs2_get_fw_log_parsed_timestamp(rs2_firmware_log_parsed_message* fw_log_parsed_msg, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
