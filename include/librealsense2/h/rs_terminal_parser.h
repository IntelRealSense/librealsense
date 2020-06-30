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
* \param[in] xml_content    content of the xml file needed for parsing
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                   pointer to created terminal parser object
*/
rs2_terminal_parser* rs2_create_terminal_parser(const char* xml_content, rs2_error** error);

/**
* \brief Deletes RealSense terminal parser.
* \param[in] terminal_parser            terminal parser to be deleted
*/
void rs2_delete_terminal_parser(rs2_terminal_parser* terminal_parser);

/**
* \brief Parses terminal command via RealSense terminal parser
* \param[in] terminal_parser        Terminal parser object
* \param[in] command                command to be sent to the hw monitor of the device
* \param[in] size_of_command        size of command to be sent to the hw monitor of the device
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           command to hw monitor, in hex
*/
rs2_raw_data_buffer* rs2_terminal_parse_command(rs2_terminal_parser* terminal_parser, 
    const char* command, unsigned int size_of_command, rs2_error** error);

/**
* \brief Parses terminal response via RealSense terminal parser
* \param[in] terminal_parser        Terminal parser object
* \param[in] command                command sent to the hw monitor of the device
* \param[in] size_of_command        size of the command to sent to the hw monitor of the device
* \param[in] response               response received by the hw monitor of the device
* \param[in] size_of_response       size of the response received by the hw monitor of the device
* \param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
* \return                           answer parsed
*/
rs2_raw_data_buffer* rs2_terminal_parse_response(rs2_terminal_parser* terminal_parser,
    const char* command, unsigned int size_of_command,
    const void* response, unsigned int size_of_response, rs2_error** error);




#ifdef __cplusplus
}
#endif
#endif
