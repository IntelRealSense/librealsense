/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "usbhost.h"
#include "usb_configuration.h"
#include "usbhost.h"
#include "usb_pipe.h"

#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <random>
#include <memory>
#include <sstream>
#include <thread>

namespace librealsense
{
    namespace usb_host
    {
        class device {
        private:
            static std::string toString(char *cstr);
            static std::string descriptorTypeToString(uint8_t dt);
            static std::string classToString(uint8_t cls);

            bool _pull_requests = true;
            std::unordered_map<int, std::shared_ptr<usb_pipe>> _pipes;
            std::shared_ptr<std::thread> _pull_thread;

        protected:
            usb_device *_handle;
            std::vector<usb_configuration> _configurations;
            const usb_device_descriptor *_usb_device_descriptor;
            std::string _sProduct;
            std::string _sManufacturer;
            std::string _sSerialNumber;
            std::string _name;
            size_t _desc_length;
            uint16_t _usb_conn_spec;

        public:
            device(usb_device *device);
            ~device();

            void claim_interface(int interface);

            usb_device* get_handle() const { return _handle; }
            const usb_device_descriptor *get_descriptor() const { return _usb_device_descriptor; };

            const std::string& get_name() { return _name; }
            const std::string& get_product() const { return _sProduct; }
            const std::string& get_manufacturer() const { return _sManufacturer; }
            const std::string& get_serial_number() const { return _sSerialNumber; }
            const std::string& get_device_description() const;

            int get_vid() { return usb_device_get_vendor_id(_handle); }
            int get_pid() { return usb_device_get_product_id(_handle); }
            int get_file_descriptor() { return usb_device_get_fd(_handle); }
            int get_descriptor_length() { return _desc_length; }
            uint16_t get_conn_spec() { return _usb_conn_spec; }

            int get_interfaces_count() { return _configurations.at(0).get_interfaces_count(); }
            int get_interfaces_associations_count() { return _configurations.at(0).get_interfaces_associations_count(); }
            const usb_interface& get_interface(int index) const {  return _configurations.at(0).get_interface(index); }
            const usb_interface_association & get_interface_association(int index) const { return _configurations.at(0).get_interface_association(index) ; }

            std::shared_ptr<usb_pipe> get_pipe(int ep_address) { return _pipes[ep_address]; }

            bool add_configuration(usb_configuration &usbConfiguration);

        private:
            void start();
            void stop();
            void build_tree();
            void add_pipe(uint8_t ep_address, std::shared_ptr<usb_pipe> pipe);
            int get_configuration_count() { return _configurations.size(); }
            const usb_configuration &get_configuration(int index) { return _configurations.at(index); }
            std::mutex m;

            template<typename T>
            void foreach_interface(T action) {
                for(int i = 0; i < get_interfaces_count(); i++)
                    action(get_interface(i));
            }
        };
    }
}
