// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsNetDevice.h"

#include <api.h>
#include <librealsense2-net/rs_net.h>
#include "log.h"

#include "RsServer.h"

#include <chrono>
#include <list>
#include <thread>
#include <iostream>
#include <string>

rs2_device* rs2_create_net_device(int api_version, const char* address, rs2_error** error) BEGIN_API_CALL
{
    verify_version_compatibility(api_version);
    VALIDATE_NOT_NULL(address);

    std::string addr(address);

    // create sw device
    rs2::software_device sw_dev = rs2::software_device([](rs2_device*) {});
    // create IP instance
    rs_net_device* ip_dev = new rs_net_device(sw_dev, addr);
    // set client destruction functioun
    sw_dev.set_destruction_callback([ip_dev] { delete ip_dev; });
    // register device info to sw device
    DeviceData data = ip_dev->remote_sensors[0]->rtsp_client->getDeviceData();
    sw_dev.update_info(RS2_CAMERA_INFO_NAME, data.name + " IP Device");
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_IP_ADDRESS, addr);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER, data.serialNum);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, data.usbType);

    return sw_dev.get().get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, address)

std::shared_ptr<rs2_device> cdp(rs2_device* device, std::function<void(rs2_device*)> deleter)
{
    std::shared_ptr<rs2_device> dev(device, deleter);
    return dev;
}

// rs_server
rs_server* rs2_create_server(int api_version, rs2_device* dev, rs_server_params params, rs2_error** error)
{
    const rs2::device device(cdp(dev, [](rs2_device*) {}));

    return new server(device, params.iface, params.port);
}

int rs2_start_server(int api_version, rs_server* srv, rs2_error** error)
{
    if (!srv) return -1; // ERR_NOSERVER

    server* s = (server*)srv;
    s->start();

    return 0; // SUCCESS
}

int rs2_stop_server(int api_version, rs_server* srv, rs2_error** error)
{
    if (!srv) return -1; // ERR_NOSERVER

    server* s = (server*)srv;
    s->stop();

    return 0; // SUCCESS
}

int rs2_destroy_server(int api_version, rs_server* srv, rs2_error** error)
{
    if (!srv) return -1; // ERR_NOSERVER

    server* s = (server*)srv;
    delete [] s;

    return 0; // SUCCESS
}

#ifdef BUILD_EASYLOGGINGPP
#ifdef BUILD_SHARED_LIBS
INITIALIZE_EASYLOGGINGPP
#endif
char log_net_name[] = "librealsense";
static librealsense::logger_type<log_net_name> logger_net;
#endif
