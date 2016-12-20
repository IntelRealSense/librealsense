// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS_USE_V4L2_BACKEND

#include "backend.h"
#include "types.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <thread>
#include <utility> // for pair
#include <chrono>
#include <thread>
#include <atomic>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/usb/video.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <fts.h>
#include <regex>
#include <list>

#pragma GCC diagnostic ignored "-Wpedantic"
#include <libusb.h>
#pragma GCC diagnostic pop

#pragma GCC diagnostic ignored "-Woverflow"

namespace rsimpl
{
    namespace uvc
    {
        static void throw_error(const char * s)
        {
            std::ostringstream ss;
            ss << s << " error " << errno << ", " << strerror(errno);
            throw std::runtime_error(ss.str());
        }

        static void warn_error(const char * s)
        {
            LOG_ERROR(s << " error " << errno << ", " << strerror(errno));
        }

        static int xioctl(int fh, int request, void *arg)
        {
            int r=0;
            do {
                r = ioctl(fh, request, arg);
            } while (r < 0 && errno == EINTR);
            return r;
        }

        struct buffer { void * start; size_t length; };

        static std::string get_usb_port_id(libusb_device* usb_device)
        {
            std::string usb_bus = std::to_string(libusb_get_bus_number(usb_device));

            // As per the USB 3.0 specs, the current maximum limit for the depth is 7.
            const int max_usb_depth = 8;
            uint8_t usb_ports[max_usb_depth];
            std::stringstream port_path;
            int port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << "-" << std::to_string(usb_ports[i]);
            }

            return usb_bus + port_path.str();
        }

        static void get_usb_port_id_from_vid_pid(uint16_t vid, uint16_t pid, std::string& usb_port_id)
        {
            libusb_context * usb_context;
            int status = libusb_init(&usb_context);
            if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

            auto usb_dev_handle  = libusb_open_device_with_vid_pid(usb_context, vid, pid);
            if(!usb_dev_handle)
            {
                libusb_exit(usb_context);
                throw std::runtime_error(to_string() << "libusb_open_device_with_vid_pid(...)");
            }

            auto usb_device = libusb_get_device(usb_dev_handle);
            if(!usb_device) throw std::runtime_error(to_string() << "libusb_get_device(...)");

            usb_port_id = get_usb_port_id(usb_device);
            libusb_close(usb_dev_handle);
            libusb_exit(usb_context);
        }

        // hold all captured inputs in a specific trigger.
        class sensor_inputs {
        public:
            const std::list<callback_data>& get_input_data() const
            {
                return input_data;
            }

            // add input to the sensor inputs.
            void add_input(const hid_sensor& sensor, const hid_sensor_input& sensor_input, unsigned value) {
                input_data.push_front(callback_data{sensor, sensor_input, value});
            }
        private:
            std::list<callback_data> input_data;
        };

        // manage an IIO input. or what is called a scan.
        class hid_input {
        public:
            hid_input()
                : input(""),
                  device_path(""),
                  type(""),
                  index(-1),
                  enabled(false)
            {}

            // initialize the input by reading the input parameters.
            bool init(const std::string& iio_device_path, const std::string& input_name) {
                char buffer[1024];

                device_path = iio_device_path;
                std::string input_prefix = "in_";
                // validate if input includes th "in_" prefix. if it is . remove it.
                if (input_name.substr(0,input_prefix.size()) == input_prefix) {
                    input = input_name.substr(input_prefix.size(), input_name.size());
                } else {
                    input = input_name;
                }

                std::string input_suffix = "_en";
                // check if input contains the "en" suffix, if it is . remove it.
                if (input.substr(input.size()-input_suffix.size(), input_suffix.size()) == input_suffix) {
                    input = input.substr(0, input.size()-input_suffix.size());
                }

                // read scan type.
                std::ifstream device_type_file(device_path + "/scan_elements/in_" + input + "_type");
                if (!device_type_file) {
                    return false;
                }
                device_type_file.getline(buffer, sizeof(buffer));
                type = std::string(buffer);

                device_type_file.close();

                // read scan index.
                std::ifstream device_index_file(device_path + "/scan_elements/in_" + input + "_index");
                if (!device_index_file) {
                    return false;
                }

                device_index_file.getline(buffer, sizeof(buffer));
                index = std::stoi(buffer);

                device_index_file.close();

                // read enable state.
                std::ifstream device_enabled_file(device_path + "/scan_elements/in_" + input + "_en");
                if (!device_enabled_file) {
                    return false;
                }

                device_enabled_file.getline(buffer, sizeof(buffer));
                enabled = (std::stoi(buffer) == 0) ? false : true;

                device_enabled_file.close();
                return true;
            }

            // enable scan input. doing so cause the input to be part of the data provided in the polling.
            bool enable(bool is_enable) {
                auto input_data = is_enable ? 1 : 0;
                // open the element requested and enable and disable.
                std::ofstream iio_device_file(device_path + "/scan_elements/" + "in_" + input + "_en");

                if (!iio_device_file.is_open()) {
                    return false;
                }
                iio_device_file << input_data;
                iio_device_file.close();

                enabled = is_enable;

                return true;
            }

            std::string get_name() const { return input; }
            std::string get_type() const { return type; }
            int get_index() const { return index; }
            bool is_enabled() const { return enabled; }
        private:
            std::string input;
            std::string device_path;
            std::string type;
            int index;
            bool enabled;
        };

        // declare device sensor with all of its inputs.
        class hid_backend {
        public:
            hid_backend()
                : vid(0),
                  pid(0),
                  iio_device(-1),
                  sensor_name(""),
                  callback(nullptr),
                  capturing(false)
            {}
            ~hid_backend() {
                stop_capture();

                // clear inputs.
                inputs.clear();
            }

            // initialize the device sensor. reading its name and all of its inputs.
            bool init(int device_vid, int device_pid, int device_iio_num)
            {
                vid = device_vid;
                pid = device_pid;
                iio_device = device_iio_num;

                std::ostringstream iio_device_path;
                iio_device_path << "/sys/bus/iio/devices/iio:device" << iio_device;

                std::ifstream iio_device_file(iio_device_path.str() + "/name");

                // find iio_device in file system.
                if (!iio_device_file.good()) {
                    return false;
                }

                char name_buffer[256];
                iio_device_file.getline(name_buffer,sizeof(name_buffer));
                sensor_name = std::string(name_buffer);

                iio_device_file.close();

                // read all available input of the iio_device
                read_device_inputs( iio_device_path.str() );
                return true;
            }

            // return an input by name.
            hid_input* get_input(std::string input_name) {
                for (auto& input : inputs) {
                    if(input->get_name() == input_name) {
                        return input;
                    }
                }

                return NULL;
            }

            // start capturing and polling.
            void start_capture(hid_callback sensor_callback) {
                if (capturing)
                    return;

                std::ostringstream iio_read_device_path;
                iio_read_device_path << "/dev/iio:device" << iio_device;

                //cout << iio_read_device_path.str() << endl;
                auto iio_read_device_path_str = iio_read_device_path.str();
                std::ifstream iio_device_file(iio_read_device_path_str);

                // find iio_device in file system.
                if (!iio_device_file.good()) {
                    throw std::runtime_error("iio hid device is busy or not found!");
                }

                iio_device_file.close();

                // count number of enabled count elements and sort by their index.
                create_channel_array();

                if (!write_integer_to_param("buffer/length", buf_len)) {
                    throw std::runtime_error("write_integer_to_param failed!");
                }
                if (!write_integer_to_param("buffer/enable", 1)) {
                    throw std::runtime_error("write_integer_to_param failed!");
                }

                callback = sensor_callback;
                capturing = true;
                hid_thread = std::unique_ptr<std::thread>(new std::thread([this, buf_len, iio_read_device_path_str](){
                    int fd = 0;
                    const auto max_retries = 10;
                    auto retries = 0;
                    while(++retries < max_retries)
                    {
                        if ((fd = open(iio_read_device_path_str.c_str(), O_RDONLY | O_NONBLOCK)) > 0)
                            break;

                        LOG_WARNING("open() failed!");
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    }

                    if ((retries == max_retries) && (fd <= 0))
                    {
                        LOG_ERROR("open() failed with all retries!");
                        write_integer_to_param("buffer/enable", 0);
                        callback = NULL;
                        channels.clear();
                        return;
                    }

                    do {
                        // each channel is 32 bit
                        auto channel_size = this->channels.size()*4;
                        char data[buf_len*16] = {};
                        sensor_inputs callback_data;
                        fd_set fds;
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        struct timeval tv = {0,10000};
                        if (select(fd + 1, &fds, NULL, NULL, &tv) < 0)
                        {
                            // TODO: write to log?
                            continue;
                        }

                        if (FD_ISSET(fd, &fds))
                        {
                            read(fd, data, channel_size);
                        }
                        else
                        {
                            // TODO: write to log?
                            continue;
                        }

                        auto *data_p = (int*) data;
                        auto i = 0;
                        for(auto& channel : this->channels) {
                            callback_data.add_input(hid_sensor{get_iio_device(), get_sensor_name()},
                                                    hid_sensor_input{channel->get_index(),
                                                    channel->get_name()}, data_p[i]);
                            i++;
                        }
                        auto cb_data = callback_data.get_input_data();
                        for (auto& elem : cb_data)
                        {
                            this->callback(elem);
                        }

                    } while(this->capturing);
                    close(fd);
                }));
            }

            void stop_capture() {
                if (!capturing)
                    return;

                capturing = false;
                hid_thread->join();
                write_integer_to_param("buffer/enable", 0);
                callback = NULL;
                channels.clear();
            }
            static bool sort_hids(hid_input* first, hid_input* second)
            {
                return (first->get_index() >= second->get_index());
            }

            bool create_channel_array() {
                // build enabled channels.
                for(auto& input : inputs) {
                    if (input->is_enabled())
                    {
                        channels.push_back(input);
                    }
                }

                channels.sort(sort_hids);
            }

            std::list<hid_input*>& get_inputs() { return inputs; }

            const std::string& get_sensor_name() const { return sensor_name; }

            int get_iio_device() const { return iio_device; }

        private:

            // read the IIO device inputs.
            bool read_device_inputs(std::string device_path) {
                DIR *dir = nullptr;
                struct dirent *dir_ent;

                auto scan_elements_path = device_path + "/scan_elements";
                // start enumerate the scan elemnts dir.
                dir = opendir(scan_elements_path.c_str());
                if (dir == NULL) {
                    return false;
                }

                // verify file format. should include in_ (input) and _en (enable).
                while ((dir_ent = readdir(dir)) != NULL) {
                    if (dir_ent->d_type != DT_DIR) {
                        std::string file(dir_ent->d_name);
                        std::string prefix = "in_";
                        std::string suffix = "_en";
                        if (file.substr(0,prefix.size()) == prefix &&
                            file.substr(file.size()-suffix.size(),suffix.size()) == suffix) {
                            // initialize input.
                            auto* new_input = new hid_input();
                            if (!new_input->init(device_path, file))
                            {
                                // fail to initialize this input. continue to the next one.
                                continue;
                            }

                            // push to input list.
                            inputs.push_front(new_input);
                        }

                    }
                }
            }

            // configure hid device via fd
            bool write_integer_to_param(const std::string& param,int value) {
                std::ostringstream iio_device_path;
                iio_device_path << "/sys/bus/iio/devices/iio:device" << iio_device << "/" << param;
                auto str = iio_device_path.str();

                std::ofstream iio_device_file(iio_device_path.str());

                if (!iio_device_file.good()) {
                    return false;
                }

                iio_device_file << value;

                iio_device_file.close();

                return true;
            }

            static const int buf_len = 100;
            int vid;
            int pid;
            int iio_device;
            std::string sensor_name;

            std::list<hid_input*> inputs;
            std::list<hid_input*> channels;
            hid_callback callback;
            std::atomic<bool> capturing;
            std::unique_ptr<std::thread> hid_thread;
        };

        class v4l_hid_device : public hid_device
        {
        public:
            v4l_hid_device(const hid_device_info& info)
            {
                _info.unique_id = "";
                v4l_hid_device::foreach_hid_device([&](const hid_device_info& hid_dev_info, const std::string& iio_device){
                    if (hid_dev_info.unique_id == info.unique_id)
                    {
                        _info = info;
                        iio_devices.push_back(iio_device);
                    }
                });

                if (_info.unique_id == "") throw std::runtime_error("hid device is no longer connected!");
            }

            ~v4l_hid_device()
            {
                for (auto& elem : _streaming_sensors)
                {
                    elem->stop_capture();
                }
            }

            void open()
            {
                for (auto& iio_device : iio_devices)
                {
                    auto device = std::unique_ptr<hid_backend>(new hid_backend());
                    if (device->init(std::stoul(_info.vid, nullptr, 16),
                                     std::stoul(_info.pid, nullptr, 16),
                                     std::stoul(iio_device, nullptr, 16)))
                    {
                        _hid_sensors.push_back(std::move(device));
                    }
                    else
                    {
                        for (auto& hid_sensor : _hid_sensors)
                        {
                            hid_sensor.reset();
                        }
                        _hid_sensors.clear();
                        throw std::runtime_error("Hid device is busy!");
                    }
                }
            }

            void close()
            {
                for (auto& hid_sensor : _hid_sensors)
                {
                    hid_sensor.reset();
                }
                _hid_sensors.clear();
            }

            std::vector<hid_sensor> get_sensors()
            {
                std::vector<hid_sensor> iio_sensors;
                for (auto& elem : _hid_sensors)
                {
                    iio_sensors.push_back(hid_sensor{elem->get_iio_device(), elem->get_sensor_name()});
                }
                return iio_sensors;
            }

            std::vector<hid_sensor_input> get_sensor_inputs(int sensor_iio)
            {
                std::vector<hid_sensor_input> sensor_inputs;
                for (auto& elem : _hid_sensors)
                {
                    if (elem->get_iio_device() == sensor_iio)
                    {
                        auto inputs = elem->get_inputs();
                        for (auto input : inputs)
                        {
                            sensor_inputs.push_back(hid_sensor_input{input->get_index(), input->get_name()});
                        }
                    }
                }
                return sensor_inputs;
            }

            void start_capture(const std::vector<int>& sensor_iio, hid_callback callback)
            {
                for (auto& iio : sensor_iio)
                {
                    for (auto& sensor : _hid_sensors)
                    {
                        if (sensor->get_iio_device() == iio)
                        {
                            auto inputs = sensor->get_inputs();
                            for (auto input : inputs)
                            {
                                input->enable(true);
                                _streaming_sensors.push_back(sensor.get());
                                break;
                            }
                        }
                    }

                    if (_streaming_sensors.empty())
                        LOG_ERROR("iio_sensor " + std::to_string(iio) + " not found!");
                }

                std::vector<hid_backend*> captured_sensors;
                try{
                for (auto& elem : _streaming_sensors)
                {
                    elem->start_capture(callback);
                    captured_sensors.push_back(elem);
                }
                }
                catch(...)
                {
                    for (auto& elem : captured_sensors)
                        elem->stop_capture();

                    _streaming_sensors.clear();
                    throw;
                }
            }

            void stop_capture()
            {
                for (auto& sensor : _hid_sensors)
                {
                    auto inputs = sensor->get_inputs();
                    for (auto input : inputs)
                    {
                        input->enable(false);
                        sensor->stop_capture();
                        break;
                    }
                }

                _streaming_sensors.clear();
            }

            static void foreach_hid_device(std::function<void(const hid_device_info&, const std::string&)> action)
            {
                const std::string root_path = "/sys/devices/pci0000:00";
                // convert to vector because fts_name needs a char*.
                std::vector<char> path(root_path.c_str(), root_path.c_str() + root_path.size() + 1);
                char *paths[] = { path.data(), NULL };
                // using FTS to read the directory structure of the root_path.
                FTS *tree = fts_open(paths, FTS_NOCHDIR, 0);

                if (!tree) {
                    throw std::runtime_error(to_string() << "fts_open returned null!");
                }

                FTSENT *node;
                while ((node = fts_read(tree))) {
                    if (node->fts_level > 0 && node->fts_name[0] == '.')
                        fts_set(tree, node, FTS_SKIP);
                    else if (node->fts_info & FTS_F) {
                        // for each iio:device , add to the device list. regex help identify device.
                        if (std::regex_search (node->fts_path, std::regex("iio:device[\\d]/name$"))) {
                            // validate device path and fetch device parameters.
                            std::smatch m;
                            auto device_path = std::string(node->fts_path);
                            if (!std::regex_search(device_path, m, std::regex("/sys/devices/pci0000:00/.*(\\S{1})-(\\S{1}).*(\\S{4}):(\\S{4}):(\\S{4}).(\\S{4})/(.*)/iio:device(\\d{1,3})/name$")))
                            {
                                throw std::runtime_error(to_string() << "HID enumeration failed. couldn't parse iio hid device path, regex is not valid!");
                            }

                            hid_device_info hid_dev_info;
                            // we are safe to get all parameters because regex is valid.
                            auto busnum = std::string(m[1]);
                            auto port_id = std::string(m[2]);
                            hid_dev_info.vid = m[4];
                            hid_dev_info.pid = m[5];

                            std::stringstream ss_vid(hid_dev_info.vid);
                            std::stringstream ss_pid(hid_dev_info.pid);
                            unsigned long long vid, pid;
                            ss_vid >> std::hex >> vid;
                            ss_pid >> std::hex >> pid;

                            get_usb_port_id_from_vid_pid(vid, pid, hid_dev_info.unique_id);
                            hid_dev_info.id = m[6];
                            hid_dev_info.device_path = device_path;
                            std::string iio_device = m[8];
                            action(hid_dev_info, iio_device);
                        }
                    }
                }
                fts_close(tree);
            }

        private:
            hid_device_info _info;
            std::vector<std::string> iio_devices;
            std::vector<std::unique_ptr<hid_backend>> _hid_sensors;
            std::vector<hid_backend*> _streaming_sensors;
        };

        class v4l_usb_device : public usb_device
        {
        public:
            v4l_usb_device(const usb_device_info& info)
            {
                int status = libusb_init(&_usb_context);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

                std::vector<usb_device_info> results;
                v4l_usb_device::foreach_usb_device(_usb_context,
                [&results, info, this](const usb_device_info& i, libusb_device* dev)
                {
                    if (i.unique_id == info.unique_id)
                    {
                        _usb_device = dev;
                        libusb_ref_device(dev);
                    }
                });

                _mi = info.mi;
            }

            ~v4l_usb_device()
            {
                if(_usb_device) libusb_unref_device(_usb_device);
                libusb_exit(_usb_context);
            }


            static std::string get_usb_port_id_from_usb_device(libusb_device* usb_device)
            {
                return get_usb_port_id(usb_device);
            }

            static void foreach_usb_device(libusb_context* usb_context, std::function<void(
                                                                const usb_device_info&,
                                                                libusb_device*)> action)
            {
                // Obtain libusb_device_handle for each device
                libusb_device ** list;
                int status = libusb_get_device_list(usb_context, &list);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));
                for(int i=0; list[i]; ++i)
                {
                    libusb_device * usb_device = list[i];
                    auto parent_device = libusb_get_parent(usb_device);
                    if (parent_device)
                    {
                        usb_device_info info;
                        std::stringstream ss;
                        info.unique_id = get_usb_port_id(usb_device);
                        action(info, usb_device);
                    }
                }
                libusb_free_device_list(list, 1);
            }

            std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) override
            {
                libusb_device_handle* usb_handle;
                int status = libusb_open(_usb_device, &usb_handle);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_open(...) returned " << libusb_error_name(status));
                status = libusb_claim_interface(usb_handle, _mi);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));

                int actual_length;
                status = libusb_bulk_transfer(usb_handle, 1, const_cast<uint8_t*>(data.data()), data.size(), &actual_length, timeout_ms);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                std::vector<uint8_t> result;


                if (require_response)
                {
                    result.resize(1024);
                    status = libusb_bulk_transfer(usb_handle, 0x81, const_cast<uint8_t*>(result.data()), result.size(), &actual_length, timeout_ms);
                    if(status < 0) throw std::runtime_error(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));
                    result.resize(actual_length);
                }

                libusb_close(usb_handle);

                return result;
            }

        private:
            libusb_context* _usb_context;
            libusb_device* _usb_device;
            int _mi;
        };

        class v4l_uvc_device : public uvc_device
        {
        public:
            static void foreach_uvc_device(
                    std::function<void(const uvc_device_info&,
                                       const std::string&)> action)
            {
                // Check if the uvcvideo kernel module is loaded
                std::ifstream modules("/proc/modules");
                std::string modulesline;
                std::regex regex("uvcvideo.* - Live.*");
                std::smatch match;
                bool module_found = false;


                while(std::getline(modules, modulesline) && !module_found)
                {
                    module_found = std::regex_match(modulesline, match, regex);
                }

                if(!module_found)
                {
                    throw std::runtime_error("uvcvideo kernel module is not loaded");
                }

                // Enumerate all subdevices present on the system
                DIR * dir = opendir("/sys/class/video4linux");
                if(!dir) throw std::runtime_error("Cannot access /sys/class/video4linux");
                while (dirent * entry = readdir(dir))
                {
                    std::string name = entry->d_name;
                    if(name == "." || name == "..") continue;

                    // Resolve a pathname to ignore virtual video devices
                    std::string path = "/sys/class/video4linux/" + name;
                    char buff[PATH_MAX] = {0};
                    if (realpath(path.c_str(), buff) != NULL)
                    {
                        std::string real_path = std::string(buff);
                        if (real_path.find("virtual") != std::string::npos)
                            continue;
                    }

                    try
                    {
                        int vid, pid, mi;
                        int busnum, devnum, parent_devnum;

                        auto dev_name = "/dev/" + name;

                        struct stat st;
                        if(stat(dev_name.c_str(), &st) < 0)
                        {
                            std::ostringstream ss; ss << "Cannot identify '" << dev_name << "': " << errno << ", " << strerror(errno);
                            throw std::runtime_error(ss.str());
                        }
                        if(!S_ISCHR(st.st_mode)) throw std::runtime_error(dev_name + " is no device");

                        // Search directory and up to three parent directories to find busnum/devnum
                        std::ostringstream ss; ss << "/sys/dev/char/" << major(st.st_rdev) << ":" << minor(st.st_rdev) << "/device/";
                        auto path = ss.str();
                        bool good = false;
                        for(int i=0; i<=3; ++i)
                        {
                            if(std::ifstream(path + "busnum") >> busnum)
                            {
                                if(std::ifstream(path + "devnum") >> devnum)
                                {
                                    if(std::ifstream(path + "../devnum") >> parent_devnum)
                                    {
                                        good = true;
                                        break;
                                    }
                                }
                            }
                            path += "../";
                        }
                        if(!good) throw std::runtime_error("Failed to read busnum/devnum");

                        std::string modalias;
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/modalias") >> modalias))
                            throw std::runtime_error("Failed to read modalias");
                        if(modalias.size() < 14 || modalias.substr(0,5) != "usb:v" || modalias[9] != 'p')
                            throw std::runtime_error("Not a usb format modalias");
                        if(!(std::istringstream(modalias.substr(5,4)) >> std::hex >> vid))
                            throw std::runtime_error("Failed to read vendor ID");
                        if(!(std::istringstream(modalias.substr(10,4)) >> std::hex >> pid))
                            throw std::runtime_error("Failed to read product ID");
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/bInterfaceNumber") >> std::hex >> mi))
                            throw std::runtime_error("Failed to read interface number");

                        uvc_device_info info;
                        info.pid = pid;
                        info.vid = vid;
                        info.mi = mi;
                        info.id = dev_name;
                        info.device_path = std::string(buff);
                        get_usb_port_id_from_vid_pid(vid, pid, info.unique_id);
                        action(info, dev_name);
                    }
                    catch(const std::exception & e)
                    {
                        LOG_INFO("Not a USB video device: " << e.what());
                    }
                }
                closedir(dir);
            }

            static uint32_t get_cid(rs_option option)
            {
                switch(option)
                {
                case RS_OPTION_BACKLIGHT_COMPENSATION: return V4L2_CID_BACKLIGHT_COMPENSATION;
                case RS_OPTION_BRIGHTNESS: return V4L2_CID_BRIGHTNESS;
                case RS_OPTION_CONTRAST: return V4L2_CID_CONTRAST;
                case RS_OPTION_EXPOSURE: return V4L2_CID_EXPOSURE_ABSOLUTE; // Is this actually valid? I'm getting a lot of VIDIOC error 22s...
                case RS_OPTION_GAIN: return V4L2_CID_GAIN;
                case RS_OPTION_GAMMA: return V4L2_CID_GAMMA;
                case RS_OPTION_HUE: return V4L2_CID_HUE;
                case RS_OPTION_SATURATION: return V4L2_CID_SATURATION;
                case RS_OPTION_SHARPNESS: return V4L2_CID_SHARPNESS;
                case RS_OPTION_WHITE_BALANCE: return V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                case RS_OPTION_ENABLE_AUTO_EXPOSURE: return V4L2_CID_EXPOSURE_AUTO; // Automatic gain/exposure control
                case RS_OPTION_ENABLE_AUTO_WHITE_BALANCE: return V4L2_CID_AUTO_WHITE_BALANCE;
                default: throw std::runtime_error(to_string() << "no v4l2 cid for option " << option);
                }
            }

            v4l_uvc_device(const uvc_device_info& info)
                : _name(""), _info(),
                  _is_capturing(false),
                  _is_alive(true),
                  _thread(nullptr)
            {
                foreach_uvc_device([&info, this](const uvc_device_info& i, const std::string& name)
                {
                    if (i == info)
                    {
                        _name = name;
                        _info = i;
                        _device_path = i.device_path;
                    }
                });
                if (_name == "") throw std::runtime_error("device is no longer connected!");


            }

            void capture_loop()
            {
                while(_is_capturing)
                {
                    poll();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            ~v4l_uvc_device()
            {
                _is_capturing = false;
                if (_thread) _thread->join();
            }

            void probe_and_commit(stream_profile profile, frame_callback callback) override
            {
                if(!_is_capturing && !_callback)
                {
                    v4l2_format fmt = {};
                    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    fmt.fmt.pix.width       = profile.width;
                    fmt.fmt.pix.height      = profile.height;
                    fmt.fmt.pix.pixelformat = (const big_endian<int> &)profile.format;
                    fmt.fmt.pix.field       = V4L2_FIELD_NONE;
                    if(xioctl(_fd, VIDIOC_S_FMT, &fmt) < 0)
                    {
                        throw_error("VIDIOC_S_FMT");
                    }

                    LOG_INFO("Trying to configure fourcc " << fourcc_to_string(fmt.fmt.pix.pixelformat));

                    v4l2_streamparm parm = {};
                    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_G_PARM, &parm) < 0) throw_error("VIDIOC_G_PARM");
                    parm.parm.capture.timeperframe.numerator = 1;
                    parm.parm.capture.timeperframe.denominator = profile.fps;
                    if(xioctl(_fd, VIDIOC_S_PARM, &parm) < 0) throw_error("VIDIOC_S_PARM");

                    // Init memory mapped IO
                    v4l2_requestbuffers req = {};
                    req.count = 4;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) throw std::runtime_error(_name + " does not support memory mapping");
                        else throw_error("VIDIOC_REQBUFS");
                    }
                    if(req.count < 2)
                    {
                        throw std::runtime_error("Insufficient buffer memory on " + _name);
                    }

                    _buffers.resize(req.count);
                    for(size_t i = 0; i < _buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(_fd, VIDIOC_QUERYBUF, &buf) < 0) throw_error("VIDIOC_QUERYBUF");

                        _buffers[i].length = buf.length;
                        _buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
                        if(_buffers[i].start == MAP_FAILED) throw_error("mmap");
                    }
                    _profile = profile;
                    _callback = callback;
                }
                else
                {
                    throw std::runtime_error("Device already streaming!");
                }
            }

            void play() override
            {
                if(!_is_capturing)
                {
                    // Start capturing
                    for(size_t i = 0; i < _buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                    }

                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    for(int i=0; i<10; ++i)
                    {
                        if (xioctl(_fd, VIDIOC_STREAMON, &type) < 0)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        else break;
                    }
                    if(xioctl(_fd, VIDIOC_STREAMON, &type) < 0) throw_error("VIDIOC_STREAMON");

                    _is_capturing = true;
                    _thread = std::unique_ptr<std::thread>(new std::thread([this](){ capture_loop(); }));
                }
            }

            void stop(stream_profile) override
            {
                if(_is_capturing)
                {
                    _is_capturing = false;
                    _thread->join();
                    _thread.reset();

                    // Stop streamining
                    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0) warn_error("VIDIOC_STREAMOFF");
                }

                if (_callback)
                {

                    for(size_t i = 0; i < _buffers.size(); i++)
                    {
                        if(munmap(_buffers[i].start, _buffers[i].length) < 0) warn_error("munmap");
                    }

                    // Close memory mapped IO
                    struct v4l2_requestbuffers req = {};
                    req.count = 0;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL) LOG_ERROR(_name + " does not support memory mapping");
                        else warn_error("VIDIOC_REQBUFS");
                    }

                    _callback = nullptr;
                }
            }

            std::string fourcc_to_string(uint32_t id) const
            {
                uint32_t device_fourcc = id;
                char fourcc_buff[sizeof(device_fourcc)+1];
                memcpy(fourcc_buff, &device_fourcc, sizeof(device_fourcc));
                fourcc_buff[sizeof(device_fourcc)] = 0;
                return fourcc_buff;
            }

            void poll()
            {
                int max_fd = _fd;
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(_fd, &fds);

                struct timeval tv = {0,10000};
                if(select(max_fd + 1, &fds, NULL, NULL, &tv) < 0)
                {
                    if (errno == EINTR) return;
                    throw_error("select");
                }

                if(FD_ISSET(_fd, &fds))
                {
                    v4l2_buffer buf = {};
                    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buf.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                    {
                        if(errno == EAGAIN) return;
                        throw_error("VIDIOC_DQBUF");
                    }

                    frame_object fo { (int)_buffers[buf.index].length,
                                      _buffers[buf.index].start };

                    _callback(_profile, fo);

                    if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0) throw_error("VIDIOC_QBUF");
                }
            }

            void set_power_state(power_state state) override
            {
                if (state == D0 && _state == D3)
                {
                    _fd = open(_name.c_str(), O_RDWR | O_NONBLOCK, 0);
                    if(_fd < 0)
                    {
                        std::ostringstream ss; ss << "Cannot open '" << _name << "': " << errno << ", " << strerror(errno);
                        throw std::runtime_error(ss.str());
                    }

                    v4l2_capability cap = {};
                    if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
                    {
                        if(errno == EINVAL) throw std::runtime_error(_name + " is no V4L2 device");
                        else throw_error("VIDIOC_QUERYCAP");
                    }
                    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) throw std::runtime_error(_name + " is no video capture device");
                    if(!(cap.capabilities & V4L2_CAP_STREAMING)) throw std::runtime_error(_name + " does not support streaming I/O");

                    // Select video input, video standard and tune here.
                    v4l2_cropcap cropcap = {};
                    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_CROPCAP, &cropcap) == 0)
                    {
                        v4l2_crop crop = {};
                        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        crop.c = cropcap.defrect; // reset to default
                        if(xioctl(_fd, VIDIOC_S_CROP, &crop) < 0)
                        {
                            switch (errno)
                            {
                            case EINVAL: break; // Cropping not supported
                            default: break; // Errors ignored
                            }
                        }
                    } else {} // Errors ignored
                }
                if (state == D3 && _state == D0)
                {
                    stop(_profile);
                    if(close(_fd) < 0) warn_error("close");
                    _fd = 0;
                }
                _state = state;
            }
            power_state get_power_state() const override { return _state; }

            void init_xu(const extension_unit& xu) override {}
            void set_xu(const extension_unit& xu, uint8_t control, const uint8_t* data, int size) override
            {
                uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_SET_CUR,
                                          static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_SET_CUR");
            }
            void get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override
            {
                uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_GET_CUR,
                                          static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0) throw_error("UVCIOC_CTRL_QUERY:UVC_GET_CUR");
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override
            {
                control_range result;
                __u16 size = 0;
                __u32 value = 0; // all of the real sense extended controls are up to 4 bytes
                                // checking return value for UVC_GET_LEN and allocating
                                // appropriately might be better
                __u8 * data = (__u8 *)&value;
                struct uvc_xu_control_query xquery;
                memset(&xquery, 0, sizeof(xquery));
                xquery.query = UVC_GET_LEN;
                xquery.size = 2; // size seems to always be 2 for the LEN query, but
                                 //doesn't seem to be documented. Use result for size
                                 //in all future queries of the same control number
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = (__u8 *)&size;

                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_LEN");
                }

                assert(size<=4);

                xquery.query = UVC_GET_MIN;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MIN");
                }
                result.min = value;

                xquery.query = UVC_GET_MAX;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_MAX");
                }
                result.max = value;

                xquery.query = UVC_GET_DEF;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_DEF");
                }
                result.def = value;

                xquery.query = UVC_GET_RES;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw std::runtime_error(to_string() << " ioctl failed on UVC_GET_CUR");
                }
                result.step = value;

                return result;
            }

            int get_pu(rs_option opt) const override
            {
                struct v4l2_control control = {get_cid(opt), 0};
                if (xioctl(_fd, VIDIOC_G_CTRL, &control) < 0) throw_error("VIDIOC_G_CTRL");
                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt)  { control.value = (V4L2_EXPOSURE_MANUAL==control.value) ? 0 : 1; }
                return control.value;
            }

            void set_pu(rs_option opt, int value) override
            {
                struct v4l2_control control = {get_cid(opt), value};
                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt) { control.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL; }
                if (xioctl(_fd, VIDIOC_S_CTRL, &control) < 0) throw_error("VIDIOC_S_CTRL");
            }

            control_range get_pu_range(rs_option option) const override
            {
                control_range range;

                // Auto controls range is trimed to {0,1} range
                if(option >= RS_OPTION_ENABLE_AUTO_EXPOSURE && option <= RS_OPTION_ENABLE_AUTO_WHITE_BALANCE)
                {
                    range.min  = 0;
                    range.max  = 1;
                    range.step = 1;
                    range.def  = 1;
                    return range;
                }

                struct v4l2_queryctrl query = {};
                query.id = get_cid(option);
                if (xioctl(_fd, VIDIOC_QUERYCTRL, &query) < 0)
                {
                    // Some controls (exposure, auto exposure, auto hue) do not seem to work on V4L2
                    // Instead of throwing an error, return an empty range. This will cause this control to be omitted on our UI sample.
                    // TODO: Figure out what can be done about these options and make this work
                    query.minimum = query.maximum = 0;
                }

                range.min  = query.minimum;
                range.max  = query.maximum;
                range.step = query.step;
                range.def  = query.default_value;

                return range;
            }

            std::vector<stream_profile> get_profiles() const override
            {

                std::vector<stream_profile> results;

                // Retrieve the caps one by one, first get pixel format, then sizes, then
                // frame rates. See http://linuxtv.org/downloads/v4l-dvb-apis for reference.
                v4l2_fmtdesc pixel_format = {};
                pixel_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                while (ioctl(_fd, VIDIOC_ENUM_FMT, &pixel_format) == 0)
                {
                    v4l2_frmsizeenum frame_size = {};
                    frame_size.pixel_format = pixel_format.pixelformat;

                    uint32_t fourcc = (const big_endian<int> &)pixel_format.pixelformat;

                    if (pixel_format.pixelformat == 0)
                    {
                        LOG_WARNING("Unknown pixel-format " << pixel_format.description << "!");

                        const std::string s(to_string() << "!" << pixel_format.description);
                        std::regex rgx("!([0-9a-f]+)-.*");
                        std::smatch match;

                        if (std::regex_search(s.begin(), s.end(), match, rgx))
                        {
                            std::stringstream ss;
                            ss <<  match[1];
                            int id;
                            ss >> std::hex >> id;
                            fourcc = (const big_endian<int> &)id;

                            auto format_str = fourcc_to_string(id);
                            LOG_WARNING("Pixel format " << pixel_format.description << " likely requires patch for fourcc code " << format_str << "!");
                        }
                    }
                    else
                    {
                        LOG_INFO("Recognized pixel-format " << pixel_format.description << "!");
                    }

                    while (ioctl(_fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) == 0)
                    {
                        v4l2_frmivalenum frame_interval = {};
                        frame_interval.pixel_format = pixel_format.pixelformat;
                        frame_interval.width = frame_size.discrete.width;
                        frame_interval.height = frame_size.discrete.height;
                        while (ioctl(_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frame_interval) == 0)
                        {
                            if (frame_interval.type == V4L2_FRMIVAL_TYPE_DISCRETE)
                            {
                                if (frame_interval.discrete.numerator != 0)
                                {
                                    auto fps =
                                        static_cast<float>(frame_interval.discrete.denominator) /
                                        static_cast<float>(frame_interval.discrete.numerator);

                                    stream_profile p;
                                    p.format = fourcc;
                                    p.width = frame_size.discrete.width;
                                    p.height = frame_size.discrete.height;
                                    p.fps = fps;
                                    results.push_back(p);
                                }
                            }

                            ++frame_interval.index;
                        }

                         ++frame_size.index;
                    }

                    ++pixel_format.index;
                }
                return results;
            }

            void lock() const override {}
            void unlock() const override {}

            std::string get_device_location() const override { return _device_path; }

        private:
            power_state _state = D3;
            std::string _name;
            std::string _device_path;
            uvc_device_info _info;
            int _fd;
            std::vector<buffer> _buffers;
            stream_profile _profile;
            frame_callback _callback;
            std::atomic<bool> _is_capturing;
            std::atomic<bool> _is_alive;
            std::unique_ptr<std::thread> _thread;
        };

        class v4l_backend : public backend
        {
        public:
            std::shared_ptr<uvc_device> create_uvc_device(uvc_device_info info) const override
            {
                return std::make_shared<uvc::retry_controls_work_around>(
                        std::make_shared<v4l_uvc_device>(info));
            }
            std::vector<uvc_device_info> query_uvc_devices() const override
            {
                std::vector<uvc_device_info> results;
                v4l_uvc_device::foreach_uvc_device(
                [&results](const uvc_device_info& i, const std::string&)
                {
                    results.push_back(i);
                });
                return results;
            }

            std::shared_ptr<usb_device> create_usb_device(usb_device_info info) const override
            {
                return std::make_shared<v4l_usb_device>(info);
            }
            std::vector<usb_device_info> query_usb_devices() const override
            {
                libusb_context * usb_context;
                int status = libusb_init(&usb_context);
                if(status < 0) throw std::runtime_error(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

                std::vector<usb_device_info> results;
                v4l_usb_device::foreach_usb_device(usb_context,
                [&results](const usb_device_info& i, libusb_device* dev)
                {
                    results.push_back(i);
                });
                libusb_exit(usb_context);

                return results;
            }

            std::shared_ptr<hid_device> create_hid_device(hid_device_info info) const override
            {
                return std::make_shared<v4l_hid_device>(info);
            }

            std::vector<hid_device_info> query_hid_devices() const override
            {
                std::map<std::string, hid_device_info> hid_device_info_map;
                v4l_hid_device::foreach_hid_device([&](const hid_device_info& hid_dev_info, const std::string&){
                    hid_device_info_map.insert(std::make_pair(hid_dev_info.unique_id, hid_dev_info));
                });

                std::vector<hid_device_info> results;
                for (auto&& elem : hid_device_info_map)
                {
                    results.push_back(elem.second);
                }

                return results;
            }
        };

        std::shared_ptr<backend> create_backend()
        {
            return std::make_shared<v4l_backend>();
        }

    }
}

#endif
