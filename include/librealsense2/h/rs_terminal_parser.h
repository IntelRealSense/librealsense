/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

/** \file rs_terminal.h
* \brief Exposes RealSense terminal functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_TERMINAL_H
#define LIBREALSENSE_RS2_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

/**
* \brief Creates RealSense terminal parser.
* \param[in] xml_path	    path to xml file needed for parsing
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   pointer to created terminal parser object
*/
rs2_terminal_parser* rs2_create_terminal_parser(const char* xml_path, rs2_error** error);

/**
* \brief Deletes RealSense terminal parser.
* \param[in] terminal_parser	        terminal parser to be deleted
*/
void rs2_delete_terminal_parser(rs2_terminal_parser* terminal_parser);

/**
* \brief Parsed answer from RealSense terminal parser
* \param[in] terminal_parser	    Terminal parser object
* \param[in] device	                device from which command should be sent
* \param[in] command	            command to be sent to the hw monitor of the device
* \param[in] size_of_command_str	size of command to be sent to the hw monitor of the device
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           answeer from hw monitor, parsed
*/
rs2_raw_data_buffer* rs2_terminal_parse_command(rs2_terminal_parser* terminal_parser, rs2_device* device, const char* command, unsigned int size_of_command_str, rs2_error** error);



#ifdef __cplusplus
}
#endif
#endif
