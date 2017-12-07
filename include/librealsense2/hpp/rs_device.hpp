// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_DEVICE_HPP
#define LIBREALSENSE_RS2_DEVICE_HPP

#include "rs_types.hpp"
#include "rs_sensor.hpp"

namespace rs2
{
    class context;
    class device_list;
    class pipeline_profile;
    class device_hub;

    class device
    {
    public:
        /**
        * returns the list of adjacent devices, sharing the same physical parent composite device
        * \return            the list of adjacent devices
        */
        std::vector<sensor> query_sensors() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_sensor_list> list(
                rs2_query_sensors(_dev.get(), &e),
                rs2_delete_sensor_list);
            error::handle(e);

            auto size = rs2_get_sensors_count(list.get(), &e);
            error::handle(e);

            std::vector<sensor> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs2_sensor> dev(
                    rs2_create_sensor(list.get(), i, &e),
                    rs2_delete_sensor);
                error::handle(e);

                sensor rs2_dev(dev);
                results.push_back(rs2_dev);
            }

            return results;
        }

        template<class T>
        T first()
        {
            for (auto&& s : query_sensors())
            {
                if (auto t = s.as<T>()) return t;
            }
            throw rs2::error("Could not find requested sensor type!");
        }

        /**
        * check if specific camera info is supported
        * \param[in] info    the parameter to check for support
        * \return                true if the parameter both exist and well-defined for the specific device
        */
        bool supports(rs2_camera_info info) const
        {
            rs2_error* e = nullptr;
            auto is_supported = rs2_supports_device_info(_dev.get(), info, &e);
            error::handle(e);
            return is_supported > 0;
        }

        /**
        * retrieve camera specific information, like versions of various internal components
        * \param[in] info     camera info type to retrieve
        * \return             the requested camera info string, in a format specific to the device model
        */
        const char* get_info(rs2_camera_info info) const
        {
            rs2_error* e = nullptr;
            auto result = rs2_get_device_info(_dev.get(), info, &e);
            error::handle(e);
            return result;
        }

        /**
        * send hardware reset request to the device
        */
        void hardware_reset()
        {
            rs2_error* e = nullptr;

            rs2_hardware_reset(_dev.get(), &e);
            error::handle(e);
        }

        device& operator=(const std::shared_ptr<rs2_device> dev)
        {
            _dev.reset();
            _dev = dev;
            return *this;
        }
        device& operator=(const device& dev)
        {
            *this = nullptr;
            _dev = dev._dev;
            return *this;
        }
        device() : _dev(nullptr) {}

        operator bool() const
        {
            return _dev != nullptr;
        }
        const std::shared_ptr<rs2_device>& get() const
        {
            return _dev;
        }

        template<class T>
        bool is() const
        {
            T extension(*this);
            return extension;
        }

        template<class T>
        T as() const
        {
            T extension(*this);
            return extension;
        }
        virtual ~device()
        {
        }
    protected:
        friend class rs2::context;
        friend class rs2::device_list;
        friend class rs2::pipeline_profile;
        friend class rs2::device_hub;

        std::shared_ptr<rs2_device> _dev;
        explicit device(std::shared_ptr<rs2_device> dev) : _dev(dev)
        {
        }
    };

    class debug_protocol : public device
    {
    public:
        debug_protocol(device d)
                : device(d.get())
        {
            rs2_error* e = nullptr;
            if(rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_DEBUG, &e) == 0 && !e)
            {
                _dev = nullptr;
            }
            error::handle(e);
        }

        std::vector<uint8_t> send_and_receive_raw_data(const std::vector<uint8_t>& input) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                    rs2_send_and_receive_raw_data(_dev.get(), (void*)input.data(), (uint32_t)input.size(), &e),
                    rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }
    };

    class device_list
    {
    public:
        explicit device_list(std::shared_ptr<rs2_device_list> list)
            : _list(move(list)) {}

        device_list()
            : _list(nullptr) {}

        operator std::vector<device>() const
        {
            std::vector<device> res;
            for (auto&& dev : *this) res.push_back(dev);
            return res;
        }

        bool contains(const device& dev) const
        {
            rs2_error* e = nullptr;
            auto res = !!(rs2_device_list_contains(_list.get(), dev.get().get(), &e));
            error::handle(e);
            return res;
        }

        device_list& operator=(std::shared_ptr<rs2_device_list> list)
        {
            _list = move(list);
            return *this;
        }

        device operator[](uint32_t index) const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_device(_list.get(), index, &e),
                rs2_delete_device);
            error::handle(e);

            return device(dev);
        }

        uint32_t size() const
        {
            rs2_error* e = nullptr;
            auto size = rs2_get_device_count(_list.get(), &e);
            error::handle(e);
            return size;
        }

        device front() const { return std::move((*this)[0]); }
        device back() const
        {
            return std::move((*this)[size() - 1]);
        }

        class device_list_iterator
        {
            device_list_iterator(
                const device_list& device_list,
                uint32_t uint32_t)
                : _list(device_list),
                  _index(uint32_t)
            {
            }

        public:
            device operator*() const
            {
                return _list[_index];
            }
            bool operator!=(const device_list_iterator& other) const
            {
                return other._index != _index || &other._list != &_list;
            }
            bool operator==(const device_list_iterator& other) const
            {
                return !(*this != other);
            }
            device_list_iterator& operator++()
            {
                _index++;
                return *this;
            }
        private:
            friend device_list;
            const device_list& _list;
            uint32_t _index;
        };

        device_list_iterator begin() const
        {
            return device_list_iterator(*this, 0);
        }
        device_list_iterator end() const
        {
            return device_list_iterator(*this, size());
        }
        const rs2_device_list* get_list() const
        {
            return _list.get();
        }

    private:
        std::shared_ptr<rs2_device_list> _list;
    };
}
#endif // LIBREALSENSE_RS2_DEVICE_HPP
