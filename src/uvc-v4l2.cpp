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
        class named_mutex
        {
        public:
            named_mutex(const std::string& device_path, unsigned timeout)
                : _device_path(device_path),
                  _timeout(timeout) // TODO: try to lock with timeout
            {
                create_named_mutex(_device_path);
            }

            named_mutex(const named_mutex&) = delete;

            void lock() { aquire(); }
            void unlock() { release(); }

            bool try_lock()
            {
                auto ret = lockf(_fildes, F_TLOCK, 0);
                if (ret != 0)
                    return false;

                return true;
            }

            ~named_mutex()
            {
                try{
                    destroy_named_mutex();
                }
                catch(...)
                {

                }
            }

        private:
            void aquire()
            {
                auto ret = lockf(_fildes, F_LOCK, 0);
                if (ret != 0)
                    throw linux_backend_exception(to_string() << "Aquire failed");
            }

            void release()
            {
                auto ret = lockf(_fildes, F_ULOCK, 0);
                if (ret != 0)
                    throw linux_backend_exception(to_string() << "lockf(...) failed");
            }

            void create_named_mutex(const std::string& cam_id)
            {
                _fildes = open(cam_id.c_str(), O_RDWR);
                if (-1 == _fildes)
                    throw linux_backend_exception(to_string() << "open(...) failed");
            }

            void destroy_named_mutex()
            {
                auto ret = close(_fildes);
                if (0 != ret)
                    throw linux_backend_exception(to_string() << "close(...) failed");
            }

            std::string _device_path;
            unsigned _timeout;
            int _fildes;
        };

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
            uint8_t usb_ports[max_usb_depth] = {};
            std::stringstream port_path;
            int port_count = libusb_get_port_numbers(usb_device, usb_ports, max_usb_depth);

            for (size_t i = 0; i < port_count; ++i)
            {
                port_path << "-" << std::to_string(usb_ports[i]);
            }

            return usb_bus + port_path.str();
        }

        struct hid_input_info {
            std::string input = "";
            std::string device_path = "";
            int index = -1;
            bool enabled = false;

            unsigned big_endian = 0;
            unsigned bits_used = 0;
            unsigned bytes = 0;
            unsigned is_signed = 0;
            unsigned location = 0;
            unsigned shift = 0;
            uint64_t mask;
            // TODO: parse 'offset' and 'scale'
        };

        // manage an IIO input. or what is called a scan.
        class hid_input {
        public:
            hid_input() {}

            // initialize the input by reading the input parameters.
            bool init(const std::string& iio_device_path, const std::string& input_name) {
                char buffer[1024];

                info.device_path = iio_device_path;
                std::string input_prefix = "in_";
                // validate if input includes th "in_" prefix. if it is . remove it.
                if (input_name.substr(0,input_prefix.size()) == input_prefix) {
                    info.input = input_name.substr(input_prefix.size(), input_name.size());
                } else {
                    info.input = input_name;
                }

                std::string input_suffix = "_en";
                // check if input contains the "en" suffix, if it is . remove it.
                if (info.input.substr(info.input.size()-input_suffix.size(), input_suffix.size()) == input_suffix) {
                    info.input = info.input.substr(0, info.input.size()-input_suffix.size());
                }

                // read scan type.
                std::ifstream device_type_file(info.device_path + "/scan_elements/in_" + info.input + "_type");
                if (!device_type_file) {
                    return false;
                }

                device_type_file.getline(buffer, sizeof(buffer));
                unsigned pad_int;
                char sign_char, endia_nchar;
                // TODO: parse with regex
                auto ret = std::sscanf(buffer,
                                       "%ce:%c%u/%u>>%u",
                                       &endia_nchar,
                                       &sign_char,
                                       &info.bits_used,
                                       &pad_int,
                                       &info.shift);

                if (ret < 0){
                    return false;
                }

                device_type_file.close();

                info.big_endian = (endia_nchar == 'b');
                info.bytes = pad_int / 8;
                info.is_signed = (sign_char == 's');

                if (info.bits_used == 64)
                    info.mask = ~0;
                else
                    info.mask = (1ULL << info.bits_used) - 1;


                // read scan index.
                std::ifstream device_index_file(info.device_path + "/scan_elements/in_" + info.input + "_index");
                if (!device_index_file) {
                    return false;
                }

                device_index_file.getline(buffer, sizeof(buffer));
                info.index = std::stoi(buffer);

                device_index_file.close();

                // read enable state.
                std::ifstream device_enabled_file(info.device_path + "/scan_elements/in_" + info.input + "_en");
                if (!device_enabled_file) {
                    return false;
                }

                device_enabled_file.getline(buffer, sizeof(buffer));
                info.enabled = (std::stoi(buffer) == 0) ? false : true;

                device_enabled_file.close();
                return true;
            }

            // enable scan input. doing so cause the input to be part of the data provided in the polling.
            bool enable(bool is_enable) {
                auto input_data = is_enable ? 1 : 0;
                // open the element requested and enable and disable.
                std::ofstream iio_device_file(info.device_path + "/scan_elements/" + "in_" + info.input + "_en");

                if (!iio_device_file.is_open()) {
                    return false;
                }
                iio_device_file << input_data;
                iio_device_file.close();

                info.enabled = is_enable;

                return true;
            }

            const hid_input_info& get_hid_input_info() const { return info; }

        private:
            hid_input_info info;
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
                    if(input->get_hid_input_info().input == input_name) {
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
                    throw linux_backend_exception("iio hid device is busy or not found!");
                }

                iio_device_file.close();

                // count number of enabled count elements and sort by their index.
                create_channel_array();

                if (!write_integer_to_param("buffer/length", buf_len)) {
                    throw linux_backend_exception("write_integer_to_param failed!");
                }
                if (!write_integer_to_param("buffer/enable", 1)) {
                    throw linux_backend_exception("write_integer_to_param failed!");
                }

                callback = sensor_callback;
                capturing = true;
                hid_thread = std::unique_ptr<std::thread>(new std::thread([this, iio_read_device_path_str](){
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

                    const unsigned channel_size = get_channel_size();
                    const unsigned output_size = get_output_size();
                    auto raw_data_size = channel_size*buf_len;
                    do {
                        std::vector<uint8_t> raw_data(raw_data_size);
                        fd_set fds;
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);
                        auto read_size = 0;

                        struct timeval tv = {0,10000};
                        if (select(fd + 1, &fds, NULL, NULL, &tv) < 0)
                        {
                            // TODO: write to log?
                            continue;
                        }

                        if (FD_ISSET(fd, &fds))
                        {
                            read_size = read(fd, raw_data.data(), raw_data_size);
                            if (read_size < 0 )
                                continue;
                        }
                        else
                        {
                            // TODO: write to log?
                            continue;
                        }

                        // TODO: code refactoring to reduce latency
                        for (auto i = 0; i < read_size / channel_size; ++i)
                        {
                            auto p_raw_data = raw_data.data() + channel_size * i;
                            sensor_data sens_data{};
                            sens_data.sensor = hid_sensor{get_iio_device(), get_sensor_name()};
                            sens_data.data.resize(output_size);

                            // TODO: parse data dynamically
                            memcpy(sens_data.data.data() + 0, p_raw_data + 0, 2);
                            memcpy(sens_data.data.data() + 2, p_raw_data + 4, 2);
                            memcpy(sens_data.data.data() + 4, p_raw_data + 8, 2);
                            memcpy(sens_data.data.data() + 6, p_raw_data + 16, 8);
                            this->callback(sens_data);
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
                return (second->get_hid_input_info().index >= first->get_hid_input_info().index);
            }

            bool create_channel_array() {
                // build enabled channels.
                for(auto& input : inputs) {
                    if (input->get_hid_input_info().enabled)
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

            // calculate the storage size of a scan
            unsigned get_channel_size() const
            {
                assert(!channels.empty());
                auto bytes = 0;

                for (auto& elem : channels)
                {
                    auto input_info = elem->get_hid_input_info();
                    if (bytes % input_info.bytes == 0)
                    {
                        input_info.location = bytes;
                    }
                    else
                    {
                        input_info.location = bytes - bytes % input_info.bytes
                                              + input_info.bytes;
                    }

                    bytes = input_info.location + input_info.bytes;
                }

                return bytes;
            }

            // calculate the actual size of data
            unsigned get_output_size() const
            {
                assert(!channels.empty());
                auto bits_used = 0.;

                for (auto& elem : channels)
                {
                    auto input_info = elem->get_hid_input_info();
                    bits_used += input_info.bits_used;
                }

                return std::ceil(bits_used / CHAR_BIT);
            }

            // read the IIO device inputs.
            bool read_device_inputs(std::string device_path) {
                DIR *dir = nullptr;
                struct dirent *dir_ent = nullptr;

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

            static const unsigned buf_len = 128; // TODO
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

                if (_info.unique_id == "")
                    throw linux_backend_exception("hid device is no longer connected!");
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
                        throw linux_backend_exception("Hid device is busy!");
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
                const std::string root_path = "/sys/bus/iio/devices/iio:device";

                auto num = 0;

                std::stringstream ss;
                ss<<root_path<<num;

                struct stat st;
                while (stat(ss.str().c_str(),&st) == 0 && st.st_mode & S_IFDIR != 0)
                {
                    char device_path[PATH_MAX];

                    realpath(ss.str().c_str(), device_path);

                    std::smatch m;
                    std::string device_path1(device_path);

                    if (std::regex_search(device_path1, m, std::regex("/sys/devices/pci0000:00/.*(\\S{1})-(\\S{1}).*(\\S{4}):(\\S{4}):(\\S{4}).(\\S{4})/(.*)/iio:device")))
                     {
                        hid_device_info hid_dev_info{};
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

                        hid_dev_info.unique_id = busnum + "-" + port_id;
                        hid_dev_info.id = m[6];
                        hid_dev_info.device_path = device_path;


                        auto iio_device = std::to_string(num);
                        action(hid_dev_info, iio_device);
                     }
                    else
                    {
                        LOG_WARNING("couldn't parse iio device path: " << device_path);
                    }

                    ss.str("");
                    ss.clear(); // Clear state flags.

                    num++;
                    ss<<root_path<<num;
                }

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
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

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
                libusb_device ** list = nullptr;
                int status = libusb_get_device_list(usb_context, &list);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_get_device_list(...) returned " << libusb_error_name(status));

                for(int i=0; list[i]; ++i)
                {
                    libusb_device * usb_device = list[i];

                    auto parent_device = libusb_get_parent(usb_device);
                    if (parent_device)
                    {
                        usb_device_info info{};
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
                libusb_device_handle* usb_handle = nullptr;
                int status = libusb_open(_usb_device, &usb_handle);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_open(...) returned " << libusb_error_name(status));
                status = libusb_claim_interface(usb_handle, _mi);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_claim_interface(...) returned " << libusb_error_name(status));

                int actual_length;
                status = libusb_bulk_transfer(usb_handle, 1, const_cast<uint8_t*>(data.data()), data.size(), &actual_length, timeout_ms);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

                std::vector<uint8_t> result;


                if (require_response)
                {
                    result.resize(1024);
                    status = libusb_bulk_transfer(usb_handle, 0x81, const_cast<uint8_t*>(result.data()), result.size(), &actual_length, timeout_ms);
                    if(status < 0)
                        throw linux_backend_exception(to_string() << "libusb_bulk_transfer(...) returned " << libusb_error_name(status));

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
                    throw linux_backend_exception("uvcvideo kernel module is not loaded");
                }

                // Enumerate all subdevices present on the system
                DIR * dir = opendir("/sys/class/video4linux");
                if(!dir)
                    throw linux_backend_exception("Cannot access /sys/class/video4linux");

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
                        int busnum, devnum, parent_devnum, devpath;

                        auto dev_name = "/dev/" + name;

                        struct stat st = {};
                        if(stat(dev_name.c_str(), &st) < 0)
                        {
                            throw linux_backend_exception(to_string() << "Cannot identify '" << dev_name);
                        }
                        if(!S_ISCHR(st.st_mode))
                            throw linux_backend_exception(dev_name + " is no device");

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
                                        if(std::ifstream(path + "devpath") >> devpath)
                                        {
                                            good = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            path += "../";
                        }
                        if(!good)
                            throw linux_backend_exception("Failed to read busnum/devnum");

                        std::string modalias;
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/modalias") >> modalias))
                            throw linux_backend_exception("Failed to read modalias");
                        if(modalias.size() < 14 || modalias.substr(0,5) != "usb:v" || modalias[9] != 'p')
                            throw linux_backend_exception("Not a usb format modalias");
                        if(!(std::istringstream(modalias.substr(5,4)) >> std::hex >> vid))
                            throw linux_backend_exception("Failed to read vendor ID");
                        if(!(std::istringstream(modalias.substr(10,4)) >> std::hex >> pid))
                            throw linux_backend_exception("Failed to read product ID");
                        if(!(std::ifstream("/sys/class/video4linux/" + name + "/device/bInterfaceNumber") >> std::hex >> mi))
                            throw linux_backend_exception("Failed to read interface number");

                        uvc_device_info info{};
                        info.pid = pid;
                        info.vid = vid;
                        info.mi = mi;
                        info.id = dev_name;
                        info.device_path = std::string(buff);
                        info.unique_id = std::to_string(busnum) + "-" + std::to_string(devpath);
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
                default: throw linux_backend_exception(to_string() << "no v4l2 cid for option " << option);
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
                if (_name == "")
                    throw linux_backend_exception("device is no longer connected!");

                _named_mtx = std::unique_ptr<named_mutex>(new named_mutex(_name, 5000));
            }

            void capture_loop()
            {
                try
                {
                    while(_is_capturing)
                    {
                        poll();
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
                catch (const std::exception& ex)
                {
                    LOG_ERROR(ex.what());
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
                        throw linux_backend_exception("xioctl(VIDIOC_S_FMT) failed");
                    }

                    LOG_INFO("Trying to configure fourcc " << fourcc_to_string(fmt.fmt.pix.pixelformat));

                    v4l2_streamparm parm = {};
                    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    if(xioctl(_fd, VIDIOC_G_PARM, &parm) < 0)
                        throw linux_backend_exception("xioctl(VIDIOC_G_PARM) failed");

                    parm.parm.capture.timeperframe.numerator = 1;
                    parm.parm.capture.timeperframe.denominator = profile.fps;
                    if(xioctl(_fd, VIDIOC_S_PARM, &parm) < 0)
                        throw linux_backend_exception("xioctl(VIDIOC_S_PARM) failed");

                    // Init memory mapped IO
                    v4l2_requestbuffers req = {};
                    req.count = 4;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL)
                            throw linux_backend_exception(_name + " does not support memory mapping");
                        else
                            throw linux_backend_exception("xioctl(VIDIOC_REQBUFS) failed");
                    }
                    if(req.count < 2)
                    {
                        throw linux_backend_exception(to_string() << "Insufficient buffer memory on " << _name);
                    }

                    _buffers.resize(req.count);
                    for(size_t i = 0; i < _buffers.size(); ++i)
                    {
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        if(xioctl(_fd, VIDIOC_QUERYBUF, &buf) < 0)
                            throw linux_backend_exception("xioctl(VIDIOC_QUERYBUF) failed");

                        _buffers[i].length = buf.length;
                        _buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
                        if(_buffers[i].start == MAP_FAILED)
                            throw linux_backend_exception("mmap");
                    }
                    _profile = profile;
                    _callback = callback;
                }
                else
                {
                    throw wrong_api_call_sequence_exception("Device already streaming!");
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
                        if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0)
                            throw linux_backend_exception("xioctl(VIDIOC_QBUF) failed");
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
                    if(xioctl(_fd, VIDIOC_STREAMON, &type) < 0)
                        throw linux_backend_exception("xioctl(VIDIOC_STREAMON) failed");

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
                    if(xioctl(_fd, VIDIOC_STREAMOFF, &type) < 0)
                        throw linux_backend_exception("xioctl(VIDIOC_STREAMOFF) failed");
                }

                if (_callback)
                {

                    for(size_t i = 0; i < _buffers.size(); i++)
                    {
                        if(munmap(_buffers[i].start, _buffers[i].length) < 0)
                            throw linux_backend_exception("munmap failed");
                    }

                    // Close memory mapped IO
                    struct v4l2_requestbuffers req = {};
                    req.count = 0;
                    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    req.memory = V4L2_MEMORY_MMAP;
                    if(xioctl(_fd, VIDIOC_REQBUFS, &req) < 0)
                    {
                        if(errno == EINVAL)
                            LOG_ERROR(_name + " does not support memory mapping");
                        else
                            throw linux_backend_exception("xioctl(VIDIOC_REQBUFS) failed");
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
                fd_set fds{};
                FD_ZERO(&fds);
                FD_SET(_fd, &fds);

                struct timeval tv = {5,0};
                auto val = select(max_fd+1, &fds, NULL, NULL, &tv);
                if(val < 0)
                {
                    if (errno == EINTR)
                        return;

                    throw linux_backend_exception("select failed");
                }
                else if(val > 0)
                {
                    if(FD_ISSET(_fd, &fds))
                    {
                        FD_ZERO(&fds);
                        FD_SET(_fd, &fds);
                        v4l2_buffer buf = {};
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        if(xioctl(_fd, VIDIOC_DQBUF, &buf) < 0)
                        {
                            if(errno == EAGAIN)
                                return;

                            throw linux_backend_exception("xioctl(VIDIOC_DQBUF) failed");
                        }

                        frame_object fo { (int)_buffers[buf.index].length,
                                        _buffers[buf.index].start };

                        _callback(_profile, fo);

                        if(xioctl(_fd, VIDIOC_QBUF, &buf) < 0)
                            throw linux_backend_exception("xioctl(VIDIOC_QBUF) failed");
                    }
                    else
                    {
                        throw linux_backend_exception("FD_ISSET returned false");
                    }
                }
                else
                {
                    LOG_WARNING("Frames didn't arrived within 5 seconds");
                }

            }

            void set_power_state(power_state state) override
            {
                if (state == D0 && _state == D3)
                {
                    _fd = open(_name.c_str(), O_RDWR | O_NONBLOCK, 0);
                    if(_fd < 0)
                        throw linux_backend_exception(to_string() << "Cannot open '" << _name);

                    v4l2_capability cap = {};
                    if(xioctl(_fd, VIDIOC_QUERYCAP, &cap) < 0)
                    {
                        if(errno == EINVAL)
                            throw linux_backend_exception(_name + " is no V4L2 device");
                        else
                            throw linux_backend_exception("xioctl(VIDIOC_QUERYCAP) failed");
                    }
                    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
                        throw linux_backend_exception(_name + " is no video capture device");

                    if(!(cap.capabilities & V4L2_CAP_STREAMING))
                        throw linux_backend_exception(_name + " does not support streaming I/O");

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
                    if(close(_fd) < 0)
                        throw linux_backend_exception("close(...) failed");

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
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0)
                    throw linux_backend_exception("set_xu(...). xioctl(UVCIOC_CTRL_QUERY) failed");
            }
            void get_xu(const extension_unit& xu, uint8_t control, uint8_t* data, int size) const override
            {
                uvc_xu_control_query q = {static_cast<uint8_t>(xu.unit), control, UVC_GET_CUR,
                                          static_cast<uint16_t>(size), const_cast<uint8_t *>(data)};
                if(xioctl(_fd, UVCIOC_CTRL_QUERY, &q) < 0)
                    throw linux_backend_exception("get_xu(...). xioctl(UVCIOC_CTRL_QUERY) failed");
            }
            control_range get_xu_range(const extension_unit& xu, uint8_t control, int len) const override
            {
                control_range result{};
                __u16 size = 0;
                __u32 value = 0; // all of the real sense extended controls are up to 4 bytes
                                // checking return value for UVC_GET_LEN and allocating
                                // appropriately might be better
                __u8 * data = (__u8 *)&value;
                struct uvc_xu_control_query xquery = {};
                memset(&xquery, 0, sizeof(xquery));
                xquery.query = UVC_GET_LEN;
                xquery.size = 2; // size seems to always be 2 for the LEN query, but
                                 //doesn't seem to be documented. Use result for size
                                 //in all future queries of the same control number
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = (__u8 *)&size;

                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw linux_backend_exception("xioctl(UVC_GET_LEN) failed");
                }

                assert(size<=4);

                xquery.query = UVC_GET_MIN;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw linux_backend_exception("xioctl(UVC_GET_MIN) failed");
                }
                result.min = value;

                xquery.query = UVC_GET_MAX;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw linux_backend_exception("xioctl(UVC_GET_MAX) failed");
                }
                result.max = value;

                xquery.query = UVC_GET_DEF;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw linux_backend_exception("xioctl(UVC_GET_DEF) failed");
                }
                result.def = value;

                xquery.query = UVC_GET_RES;
                xquery.size = size;
                xquery.selector = control;
                xquery.unit = xu.unit;
                xquery.data = data;
                if(-1 == ioctl(_fd,UVCIOC_CTRL_QUERY,&xquery)){
                    throw linux_backend_exception("xioctl(UVC_GET_CUR) failed");
                }
                result.step = value;

                return result;
            }

            int get_pu(rs_option opt) const override
            {
                struct v4l2_control control = {get_cid(opt), 0};
                if (xioctl(_fd, VIDIOC_G_CTRL, &control) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_G_CTRL) failed");

                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt)  { control.value = (V4L2_EXPOSURE_MANUAL==control.value) ? 0 : 1; }
                return control.value;
            }

            void set_pu(rs_option opt, int value) override
            {
                struct v4l2_control control = {get_cid(opt), value};
                if (RS_OPTION_ENABLE_AUTO_EXPOSURE==opt) { control.value = value ? V4L2_EXPOSURE_APERTURE_PRIORITY : V4L2_EXPOSURE_MANUAL; }
                if (xioctl(_fd, VIDIOC_S_CTRL, &control) < 0)
                    throw linux_backend_exception("xioctl(VIDIOC_S_CTRL) failed");
            }

            control_range get_pu_range(rs_option option) const override
            {
                control_range range{};

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

                                    stream_profile p{};
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

            void lock() const override
            {
                _named_mtx->lock();
            }
            void unlock() const override
            {
                _named_mtx->unlock();
            }

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
            std::unique_ptr<named_mutex> _named_mtx;
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
                libusb_context * usb_context = nullptr;
                int status = libusb_init(&usb_context);
                if(status < 0)
                    throw linux_backend_exception(to_string() << "libusb_init(...) returned " << libusb_error_name(status));

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
