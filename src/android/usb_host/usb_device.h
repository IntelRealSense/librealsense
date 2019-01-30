//
// Created by daniel on 10/16/2018.
//

#pragma once

#include "usb_host.h"
#include "android_debug.h"
#include "usb_configuration.h"
#include "usb_host.h"
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
        class usb_device {
        private:
            static std::string toString(char *cstr);
            static std::string descriptorTypeToString(uint8_t dt);
            static std::string classToString(uint8_t cls);

            bool _pull_requests = true;
            std::unordered_map<int, std::shared_ptr<usb_pipe>> _pipes;
            std::shared_ptr<std::thread> _pull_thread;

        protected:
            usb_device_handle *_handle;
            std::vector<usb_configuration> _configurations;
            const usb_device_descriptor *_usb_device_descriptor;
            std::string _sProduct;
            std::string _sManufacturer;
            std::string _sSerialNumber;

        public:
            usb_device();
            usb_device(usb_device_handle *device);
            ~usb_device();

            bool add_configuration(usb_configuration &usbConfiguration);
            const usb_device_descriptor *get_descriptor() const { return _usb_device_descriptor; };
            usb_device_handle *GetHandle() const { return _handle; }
            const std::string &GetStringProduct() const { return _sProduct; }
            const std::string &GetStringManufacturer() const { return _sManufacturer; }
            const std::string &GetStringSerialNumber() const { return _sSerialNumber; }
            std::string get_device_description() const;
            int GetVid() { return usb_device_get_vendor_id(_handle); }
            int GetPid() { return usb_device_get_product_id(_handle); }
            int get_interfaces_count() { return _configurations.at(0).get_interfaces_count(); }
            const usb_interface& get_interface(int index) const {  return _configurations.at(0).get_interface(index); }
            int get_interfaces_associations_count() { return _configurations.at(0).get_interfaces_associations_count(); }
            const usb_interface_association & get_interface_association(int index) const { return _configurations.at(0).get_interface_association(index) ; }
            std::shared_ptr<usb_pipe> get_pipe(int ep_address) { return _pipes[ep_address]; }

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
