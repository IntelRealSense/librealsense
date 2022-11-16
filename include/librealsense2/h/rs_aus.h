/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2022 Intel Corporation. All Rights Reserved. */

/** \file rs_aus.h
* \brief Exposes RealSense Anonymous Usage Statictics functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_AUS_H
#define LIBREALSENSE_RS2_AUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

    /**
    * set counter to be value. if no value provided, counter will be asigned to 0
    * \param[in]  counter   counter string name
    * \param[in]  value     integer value for the counter
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_set(const char* counter, int value, rs2_error** error);

    /**
    * increase counter by 1
    * \param[in]  counter   counter name to be increased
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_increment(const char* counter, rs2_error** error);

    /**
    * decrease counter by 1
    * \param[in]  counter   counter name to be increased
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_decrement( const char * counter, rs2_error ** error );

    /**
    * get counter value
    * \param[in]  counter   counter string name
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return     counter value
    */
    long rs2_aus_get(const char* counter, rs2_error** error);

    /**
    * starts timer
    * \param[in]  timer    timer to start
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_start(const char* timer, rs2_error** error);

    /**
    * stops timer
    * \param[in]  timer    timer to stop
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_stop(const char* timer, rs2_error** error);


    /**
    * get all defined counters names
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return     struct that includes vector with counters names as strings
    */
    const rs2_strings_list* rs2_aus_get_counters_list(rs2_error** error);

    /**
    * get size of rs2_aus_counters_names buffer
    * \param[in]  buffer   rs2_raw_data_buffer returned by rs2_aus_get_counters_list
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return     size of rs2_aus_counters_names buffer
    */
    int rs2_aus_get_counters_list_size(const rs2_strings_list* buffer, rs2_error** error);

    /**
    * delete rs2_aus_counters_names buffer
    * \param[in]  buffer   rs2_raw_data_buffer returned by rs2_aus_get_counters_list
    * \param[out] error    if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return
    */
    void rs2_aus_delete_counters_list(const rs2_strings_list* buffer);

    /**
    * Retrieve char array from rs2_aus_counters_names buffer at index i
    * \param[in]  buffer   rs2_aus_counters_names returned by rs2_aus_get_counters_names
    * \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return     char* from buffer at index i
    */
    const char* rs2_aus_get_counter_data(const rs2_strings_list* buffer, int i, rs2_error** error);


#ifdef __cplusplus
}
#endif
#endif

