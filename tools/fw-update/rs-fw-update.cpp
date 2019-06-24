// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

using namespace TCLAP;

#define WAIT_FOR_DEVICE_TIMEOUT 10

std::vector<uint8_t> read_fw_file(std::string file_path)
{
    std::vector<uint8_t> rv;

    std::ifstream file(file_path, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        rv.resize(file.tellg());

        file.seekg(0, std::ios::beg);
        file.read((char*)rv.data(), rv.size());
        file.close();
    }

    return rv;
}

void print_device_info(rs2::device d)
{
    std::map<rs2_camera_info, std::string> camera_info;

    for (int i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = (rs2_camera_info)i;
        camera_info[info] = d.supports(info) ? d.get_info(info) : "unknown";
    }

    std::cout << "Name: " << camera_info[RS2_CAMERA_INFO_NAME] <<
        ", serial number: " << camera_info[RS2_CAMERA_INFO_SERIAL_NUMBER] <<
        ", ASIC serial number: " << camera_info[RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER] <<
        ", firmware version: " << camera_info[RS2_CAMERA_INFO_FIRMWARE_VERSION] <<
        ", USB type: " << camera_info[RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR] << std::endl;
}

void update(rs2::update_device fwu_dev, std::vector<uint8_t> fw_image)
{  
    std::cout << std::endl << "firmware update started"<< std::endl << std::endl;

    fwu_dev.update(fw_image, [&](const float progress)
    {
        printf("\rfirmware update progress: %d[%%]", (int)(progress * 100));
    });
    std::cout << std::endl << std::endl << "firmware update done" << std::endl;
}

void list_devices(rs2::context ctx)
{
    auto devs = ctx.query_devices();
    if (devs.size() == 0)
    {
        std::cout << std::endl << "there are no connected devices" << std::endl;
        return;
    }

    std::cout << std::endl << "connected devices:" << std::endl;


    int counter = 0;
    for (auto&& d : devs)
    {
        std::cout << ++counter << ") ";
        print_device_info(d);
    }
}

int main(int argc, char** argv) try
{
    rs2::context ctx;

    std::condition_variable cv;
    std::mutex mutex;
    std::string selected_serial_number;

    rs2::device new_device;
    rs2::update_device new_fw_update_device;

    bool done = false;

    CmdLine cmd("librealsense rs-fw-update tool", ' ', RS2_API_VERSION_STR);

    SwitchArg list_devices_arg("l", "list_devices", "List all available devices");
    SwitchArg recover_arg("r", "recover", "Recover all connected devices which are in recovery mode");
    ValueArg<std::string> file_arg("f", "file", "Path of the firmware image file", false, "", "string");
    ValueArg<std::string> serial_number_arg("s", "serial_number", "The serial number of the device to be update, this is mandetory if more than one device is connected", false, "", "string");

    cmd.add(list_devices_arg);
    cmd.add(recover_arg);
    cmd.add(file_arg);
    cmd.add(serial_number_arg);

    cmd.parse(argc, argv);

    bool recovery_request = recover_arg.getValue();

    if (list_devices_arg.isSet())
    {
        list_devices(ctx);
        return EXIT_SUCCESS;
    }

    if (serial_number_arg.isSet())
    {
        selected_serial_number = serial_number_arg.getValue();
        std::cout << std::endl << "search for device with serial number: " << selected_serial_number << std::endl;
    }

    if (!file_arg.isSet())
    {
        std::cout << std::endl << "firmware file must be selected" << std::endl << std::endl;
        return EXIT_FAILURE;
    }

    std::string fw_file_path = file_arg.getValue();
    std::vector<uint8_t> fw_image = read_fw_file(fw_file_path);

    if (fw_image.size() == 0)
    {
        std::cout << std::endl << "failed to read firmware file" << std::endl << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << std::endl << "update to FW: " << fw_file_path << std::endl;

    std::string asic_serial_number;

    // Recovery
    bool recovery_executed = false;
    if (recover_arg.isSet())
    {
        auto devs = ctx.query_devices(RS2_PRODUCT_LINE_DEPTH);
        for (auto&& d : devs)
        {
            if (!d.is<rs2::update_device>())
                continue;
            try
            {
                std::cout << std::endl << "recovering device: " << std::endl;
                print_device_info(d);
                update(d, fw_image);
                recovery_executed = true;
            }
            catch (...)
            {
                std::cout << std::endl << "failed to recover device" << std::endl;
            }
        }
        if (recovery_executed)
        {
            std::cout << std::endl << "recovery done" << std::endl;
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    // Update device
    ctx.set_devices_changed_callback([&](rs2::event_information& info)
    {
        if (info.get_new_devices().size() == 0)
        {
            return;
        }

        for (auto&& d : info.get_new_devices())
        {
            std::lock_guard<std::mutex> lk(mutex);
            if (d.is<rs2::update_device>() && (d.get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER) == asic_serial_number))
                new_fw_update_device = d;
            else
                new_device = d;
        }
        if(new_fw_update_device || new_device)
            cv.notify_one();
    });

    auto devs = ctx.query_devices(RS2_PRODUCT_LINE_DEPTH);

    if (!serial_number_arg.isSet() && devs.size() > 1)
    {
        std::cout << std::endl << "more than one device is connected, serial number must be selected" << std::endl << std::endl;
        return EXIT_FAILURE;
    }

    bool device_found = false;

    for (auto&& d : devs)
    {
        if (!d.is<rs2::updatable>() || !(d.supports(RS2_CAMERA_INFO_SERIAL_NUMBER) && d.supports(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER)))
            continue;

        if (d.supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))
        {
            std::string usb_type = d.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR);
            if (usb_type.find("2.") != std::string::npos) {
                std::cout << std::endl << "Warning! the camera is connected via USB 2 port, in case the process fails, connect the camera to a USB 3 port and try again" << std::endl;
            }
        }

        asic_serial_number = d.get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER);

        auto sn = d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        if (sn != selected_serial_number && devs.size() != 1)
            continue;
        device_found = true;
        auto fw = d.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION);

        std::cout << std::endl << "updating device: " << std::endl;
        print_device_info(d);

        d.as<rs2::updatable>().enter_update_state();

        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, std::chrono::seconds(WAIT_FOR_DEVICE_TIMEOUT), [&] { return new_fw_update_device; }))
        {
            std::cout << std::endl << "failed to locate a device in FW update mode" << std::endl;
            return EXIT_FAILURE;
        }

        update(new_fw_update_device, fw_image);
        done = true;
        break;
    }

    std::unique_lock<std::mutex> lk(mutex);
    cv.wait_for(lk, std::chrono::seconds(WAIT_FOR_DEVICE_TIMEOUT), [&] { return !done || new_device; });

    if (!device_found)
    {
        if(serial_number_arg.isSet())
            std::cout << std::endl << "couldn't find the requested serial number" << std::endl;
        else if (devs.size() == 1)
        {
            std::cout << std::endl << "nothing to do, run again with -h for help" << std::endl;
        }
    }

    if (done)
    {
        auto devs = ctx.query_devices();
        for (auto&& d : devs)
        {
            auto sn = d.supports(RS2_CAMERA_INFO_SERIAL_NUMBER) ? d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "unknown";
            if (sn != selected_serial_number)
                continue;

            auto fw = d.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION) ? d.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) : "unknown";
            std::cout << std::endl << "device " << sn << " successfully updated to FW: " << fw << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
