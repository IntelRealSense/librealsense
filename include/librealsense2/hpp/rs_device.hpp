// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_DEVICE_HPP
#define LIBREALSENSE_RS2_DEVICE_HPP

#include "rs_types.hpp"
#include "rs_sensor.hpp"
#include <array>

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
        T first() const
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

        explicit operator std::shared_ptr<rs2_device>() { return _dev; };
        explicit device(std::shared_ptr<rs2_device> dev) : _dev(dev) {}
    protected:
        friend class rs2::context;
        friend class rs2::device_list;
        friend class rs2::pipeline_profile;
        friend class rs2::device_hub;

        std::shared_ptr<rs2_device> _dev;

    };

    template<class T>
    class update_progress_callback : public rs2_update_progress_callback
    {
        T _callback;

    public:
        explicit update_progress_callback(T callback) : _callback(callback) {}

        void on_update_progress(const float progress) override
        {
            _callback(progress);
        }

        void release() override { delete this; }
    };

    class updatable : public device
    {
    public:
        updatable() : device() {}
        updatable(device d)
            : device(d.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_UPDATABLE, &e) == 0 && !e)
            {
                _dev.reset();
            }
            error::handle(e);
        }

        // Move the device to update state, this will cause the updatable device to disconnect and reconnect as an update device.
        void enter_update_state() const
        {
            rs2_error* e = nullptr;
            rs2_enter_update_state(_dev.get(), &e);
            error::handle(e);
        }

        // Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be
        // loaded back to the device, but it does contain all calibration and device information."
        std::vector<uint8_t> create_flash_backup() const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_create_flash_backup_cpp(_dev.get(), nullptr, &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        template<class T>
        std::vector<uint8_t> create_flash_backup(T callback) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_create_flash_backup_cpp(_dev.get(), new update_progress_callback<T>(std::move(callback)), &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        // Update an updatable device to the provided unsigned firmware. This call is executed on the caller's thread.
        void update_unsigned(const std::vector<uint8_t>& image, int update_mode = RS2_UNSIGNED_UPDATE_MODE_UPDATE) const
        {
            rs2_error* e = nullptr;
            rs2_update_firmware_unsigned_cpp(_dev.get(), image.data(), (int)image.size(), nullptr, update_mode, &e);
            error::handle(e);
        }

        // Update an updatable device to the provided unsigned firmware. This call is executed on the caller's thread and it supports progress notifications via the callback.
        template<class T>
        void update_unsigned(const std::vector<uint8_t>& image, T callback, int update_mode = RS2_UNSIGNED_UPDATE_MODE_UPDATE) const
        {
            rs2_error* e = nullptr;
            rs2_update_firmware_unsigned_cpp(_dev.get(), image.data(), int(image.size()), new update_progress_callback<T>(std::move(callback)), update_mode, &e);
            error::handle(e);
        }
    };

    class update_device : public device
    {
    public:
        update_device() : device() {}
        update_device(device d)
            : device(d.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_UPDATE_DEVICE, &e) == 0 && !e)
            {
                _dev.reset();
            }
            error::handle(e);
        }

        // Update an updatable device to the provided firmware.
        // This call is executed on the caller's thread.
        void update(const std::vector<uint8_t>& fw_image) const
        {
            rs2_error* e = nullptr;
            rs2_update_firmware_cpp(_dev.get(), fw_image.data(), (int)fw_image.size(), NULL, &e);
            error::handle(e);
        }

        // Update an updatable device to the provided firmware.
        // This call is executed on the caller's thread and it supports progress notifications via the callback.
        template<class T>
        void update(const std::vector<uint8_t>& fw_image, T callback) const
        {
            rs2_error* e = nullptr;
            rs2_update_firmware_cpp(_dev.get(), fw_image.data(), int(fw_image.size()), new update_progress_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }
    };

    typedef std::vector<uint8_t> calibration_table;

    class calibrated_device : public device
    {
    public:
        calibrated_device(device d)
            : device(d.get())
        {}

        /**
        * Write calibration that was set by set_calibration_table to device's EEPROM.
        */
        void write_calibration() const
        {
            rs2_error* e = nullptr;
            rs2_write_calibration(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * Reset device to factory calibration
        */
        void reset_to_factory_calibration()
        {
            rs2_error* e = nullptr;
            rs2_reset_to_factory_calibration(_dev.get(), &e);
            error::handle(e);
        }
    };

    class auto_calibrated_device : public calibrated_device
    {
    public:
        auto_calibrated_device(device d)
            : calibrated_device(d)
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_AUTO_CALIBRATED_DEVICE, &e) == 0 && !e)
            {
                _dev.reset();
            }
            error::handle(e);
        }

        /**
         * This will improve the depth noise.
         * \param[in] json_content      Json string to configure regular speed on chip calibration parameters:
                                            {
                                              "calib type" : 0,
                                              "speed": 3,
                                              "scan parameter": 0,
                                              "adjust both sides": 0,
                                              "white wall mode": 0
                                            }
                                            calib_type - calibraton type: 0 = regular, 1 = focal length, 2 = both regular and focal length in order
                                            speed - for regular calibration. value can be one of: Very fast = 0, Fast = 1, Medium = 2, Slow = 3, White wall = 4, default is Slow for type 0 and Fast for type 2
                                            scan_parameter - for regular calibration. value can be one of: Py scan (default) = 0, Rx scan = 1
                                            adjust_both_sides - for focal length calibration. value can be one of: 0 = adjust right only, 1 = adjust both sides
                                            white_wall_mode - white wall mode: 0 for normal mode and 1 for white wall mode
                                            if json is nullptr it will be ignored and calibration will use the default parameters
         * \param[out] health           The absolute value of regular calibration Health-Check captures how far camera calibration is from the optimal one
                                            [0, 0.25) - Good
                                            [0.25, 0.75) - Can be Improved
                                            [0.75, ) - Requires Calibration
                                        The absolute value of focal length calibration Health-Check captures how far camera calibration is from the optimal one
                                            [0, 0.15) - Good
                                            [0.15, 0.75) - Can be Improved
                                            [0.75, ) - Requires Calibration
                                        The two health numbers are encoded in one integer as follows for calib_type 2:
                                            Regular health number times 1000 are bits 0 to 11
                                            Regular health number is negative if bit 24 is 1
                                            Focal length health number times 1000 are bits 12 to 23
                                            Focal length health number is negative if bit 25 is 1
         * \param[in] callback          Optional callback to get progress notifications
         * \param[in] timeout_ms        Timeout in ms
         * \return                      New calibration table
        */
        template<class T>
        calibration_table run_on_chip_calibration(std::string json_content, float* health, T callback, int timeout_ms = 5000) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            auto buf = rs2_run_on_chip_calibration_cpp(_dev.get(), json_content.data(), int(json_content.size()), health, new update_progress_callback<T>(std::move(callback)), timeout_ms, &e);
            error::handle(e);

            std::shared_ptr<const rs2_raw_data_buffer> list(buf, rs2_delete_raw_data);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        /**
         * This will improve the depth noise.
         * \param[in] json_content      Json string to configure regular speed on chip calibration parameters:
                                            {
                                              "focal length" : 0,
                                              "speed": 3,
                                              "scan parameter": 0,
                                              "adjust both sides": 0,
                                              "white wall mode": 0
                                            }
                                            focal_length - calibraton type: 0 = regular, 1 = focal length, 2 = both regular and focal length in order
                                            speed - for regular calibration. value can be one of: Very fast = 0, Fast = 1, Medium = 2, Slow = 3, White wall = 4, default is Slow for type 0 and Fast for type 2
                                            scan_parameter - for regular calibration. value can be one of: Py scan (default) = 0, Rx scan = 1
                                            adjust_both_sides - for focal length calibration. value can be one of: 0 = adjust right only, 1 = adjust both sides
                                            white_wall_mode - white wall mode: 0 for normal mode and 1 for white wall mode
                                            if json is nullptr it will be ignored and calibration will use the default parameters
         * \param[out] health           The absolute value of regular calibration Health-Check captures how far camera calibration is from the optimal one
                                            [0, 0.25) - Good
                                            [0.25, 0.75) - Can be Improved
                                            [0.75, ) - Requires Calibration
                                        The absolute value of focal length calibration Health-Check captures how far camera calibration is from the optimal one
                                            [0, 0.15) - Good
                                            [0.15, 0.75) - Can be Improved
                                            [0.75, ) - Requires Calibration
                                        The two health numbers are encoded in one integer as follows for calib_type 2:
                                            Regular health number times 1000 are bits 0 to 11
                                            Regular health number is negative if bit 24 is 1
                                            Focal length health number times 1000 are bits 12 to 23
                                            Focal length health number is negative if bit 25 is 1
         * \param[in] timeout_ms        Timeout in ms
         * \return                      New calibration table
         */
        calibration_table run_on_chip_calibration(std::string json_content, float* health, int timeout_ms = 5000) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_run_on_chip_calibration_cpp(_dev.get(), json_content.data(), static_cast< int >( json_content.size() ), health, nullptr, timeout_ms, &e),
                rs2_delete_raw_data);
            error::handle(e);
            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        /**
        * This will adjust camera absolute distance to flat target. User needs to enter the known ground truth.
        * \param[in] ground_truth_mm     Ground truth in mm must be between 60 and 10000
        * \param[in] json_content        Json string to configure tare calibration parameters:
                                            {
                                              "average step count": 20,
                                              "step count": 20,
                                              "accuracy": 2,
                                              "scan parameter": 0,
                                              "data sampling": 0
                                            }
                                            average step count - number of frames to average, must be between 1 - 30, default = 20
                                            step count - max iteration steps, must be between 5 - 30, default = 10
                                            accuracy - Subpixel accuracy level, value can be one of: Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%), default is very high (0.025%)
                                            scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                            data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                            if json is nullptr it will be ignored and calibration will use the default parameters
        * \param[in]  content_size        Json string size if its 0 the json will be ignored and calibration will use the default parameters
        * \param[in]  callback            Optional callback to get progress notifications
        * \param[in] timeout_ms           Timeout in ms
        * \return                         New calibration table
        */
        template<class T>
        calibration_table run_tare_calibration(float ground_truth_mm, std::string json_content, T callback, int timeout_ms = 5000) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_run_tare_calibration_cpp(_dev.get(), ground_truth_mm, json_content.data(), int(json_content.size()), new update_progress_callback<T>(std::move(callback)), timeout_ms, &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        /**
         * This will adjust camera absolute distance to flat target. User needs to enter the known ground truth.
         * \param[in] ground_truth_mm     Ground truth in mm must be between 60 and 10000
         * \param[in] json_content        Json string to configure tare calibration parameters:
                                             {
                                               "average step count": 20,
                                               "step count": 20,
                                               "accuracy": 2,
                                               "scan parameter": 0,
                                               "data sampling": 0
                                             }
                                             average step count - number of frames to average, must be between 1 - 30, default = 20
                                             step count - max iteration steps, must be between 5 - 30, default = 10
                                             accuracy - Subpixel accuracy level, value can be one of: Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%), default is very high (0.025%)
                                             scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                             data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                             if json is nullptr it will be ignored and calibration will use the default parameters
         * \param[in]  content_size        Json string size if its 0 the json will be ignored and calibration will use the default parameters
         * \param[in] timeout_ms           Timeout in ms
         * \return                         New calibration table
         */
        calibration_table run_tare_calibration(float ground_truth_mm, std::string json_content, int timeout_ms = 5000) const
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_run_tare_calibration_cpp(_dev.get(), ground_truth_mm, json_content.data(), static_cast< int >( json_content.size() ), nullptr, timeout_ms, &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        /**
        *  Read current calibration table from flash.
        * \return    Calibration table
        */
        calibration_table get_calibration_table()
        {
            std::vector<uint8_t> results;

            rs2_error* e = nullptr;
            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_get_calibration_table(_dev.get(), &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            results.insert(results.begin(), start, start + size);

            return results;
        }

        /**
        *  Set current table to dynamic area.
        * \param[in]     Calibration table
        */
        void set_calibration_table(const calibration_table& calibration)
        {
            rs2_error* e = nullptr;
            rs2_set_calibration_table(_dev.get(), calibration.data(), static_cast< int >( calibration.size() ), &e);
            error::handle(e);
        }
    };

    /*
        Wrapper around any callback function that is given to calibration_change_callback.
    */
    template< class callback >
    class calibration_change_callback : public rs2_calibration_change_callback
    {
        //using callback = std::function< void( rs2_calibration_status ) >;
        callback _callback;
    public:
        calibration_change_callback( callback cb ) : _callback( cb ) {}

        void on_calibration_change( rs2_calibration_status status ) noexcept override
        {
            _callback( status );
        }
        void release() override { delete this; }
    };

    class device_calibration : public device
    {
    public:
        device_calibration( device d )
            : device( d.get() )
        {
            rs2_error* e = nullptr;
            if( rs2_is_device_extendable_to( _dev.get(), RS2_EXTENSION_DEVICE_CALIBRATION, &e ) == 0 && !e )
            {
                _dev.reset();
            }
            error::handle( e );
        }

        /*
        Your callback should look like this, for example:
            sensor.register_calibration_change_callback(
                []( rs2_calibration_status ) noexcept
                {
                    ...
                })
        */
        template< typename T >
        void register_calibration_change_callback( T callback )
        {
            // We wrap the callback with an interface and pass it to librealsense, who will
            // now manage its lifetime. Rather than deleting it, though, it will call its
            // release() function, where (back in our context) it can be safely deleted:
            rs2_error* e = nullptr;
            rs2_register_calibration_change_callback_cpp(
                _dev.get(),
                new calibration_change_callback< T >( std::move( callback )),
                &e );
            error::handle( e );
        }

        /**
        * This will trigger the given calibration, if available
        */
        void trigger_device_calibration( rs2_calibration_type type )
        {
            rs2_error* e = nullptr;
            rs2_trigger_device_calibration( _dev.get(), type, &e );
            error::handle( e );
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
                _dev.reset();
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

        operator std::shared_ptr<rs2_device_list>() { return _list; };

    private:
        std::shared_ptr<rs2_device_list> _list;
    };

    /**
     * The tm2 class is an interface for T2XX devices, such as T265.
     *
     * For T265, it provides RS2_STREAM_FISHEYE (2), RS2_STREAM_GYRO, RS2_STREAM_ACCEL, and RS2_STREAM_POSE streams,
     * and contains the following sensors:
     *
     * - pose_sensor: map and relocalization functions.
     * - wheel_odometer: input for odometry data.
     */
    class tm2 : public calibrated_device // TODO: add to wrappers [Python done]
    {
    public:
        tm2(device d)
            : calibrated_device(d)
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_TM2, &e) == 0 && !e)
            {
                _dev.reset();
            }
            error::handle(e);
        }

        /**
        * Enter the given device into loopback operation mode that uses the given file as input for raw data
        * \param[in]  from_file  Path to bag file with raw data for loopback
        */
        void enable_loopback(const std::string& from_file)
        {
            rs2_error* e = nullptr;
            rs2_loopback_enable(_dev.get(), from_file.c_str(), &e);
            error::handle(e);
        }

        /**
        * Restores the given device into normal operation mode
        */
        void disable_loopback()
        {
            rs2_error* e = nullptr;
            rs2_loopback_disable(_dev.get(), &e);
            error::handle(e);
        }

        /**
        * Checks if the device is in loopback mode or not
        * \return true if the device is in loopback operation mode
        */
        bool is_loopback_enabled() const
        {
            rs2_error* e = nullptr;
            int is_enabled = rs2_loopback_is_enabled(_dev.get(), &e);
            error::handle(e);
            return is_enabled != 0;
        }

        /**
        * Connects to a given tm2 controller
        * \param[in]  mac_addr   The MAC address of the desired controller
        */
        void connect_controller(const std::array<uint8_t, 6>& mac_addr)
        {
            rs2_error* e = nullptr;
            rs2_connect_tm2_controller(_dev.get(), mac_addr.data(), &e);
            error::handle(e);
        }

        /**
        * Disconnects a given tm2 controller
        * \param[in]  id         The ID of the desired controller
        */
        void disconnect_controller(int id)
        {
            rs2_error* e = nullptr;
            rs2_disconnect_tm2_controller(_dev.get(), id, &e);
            error::handle(e);
        }

        /**
        * Set tm2 camera intrinsics
        * \param[in] fisheye_senor_id The ID of the fisheye sensor
        * \param[in] intrinsics       value to be written to the device
        */
        void set_intrinsics(int fisheye_sensor_id, const rs2_intrinsics& intrinsics)
        {
            rs2_error* e = nullptr;
            auto fisheye_sensor = get_sensor_profile(RS2_STREAM_FISHEYE, fisheye_sensor_id);
            rs2_set_intrinsics(fisheye_sensor.first.get().get(), fisheye_sensor.second.get(), &intrinsics, &e);
            error::handle(e);
        }

        /**
        * Set tm2 camera extrinsics
        * \param[in] from_stream     only support RS2_STREAM_FISHEYE
        * \param[in] from_id         only support left fisheye = 1
        * \param[in] to_stream       only support RS2_STREAM_FISHEYE
        * \param[in] to_id           only support right fisheye = 2
        * \param[in] extrinsics      extrinsics value to be written to the device
        */
        void set_extrinsics(rs2_stream from_stream, int from_id, rs2_stream to_stream, int to_id, rs2_extrinsics& extrinsics)
        {
            rs2_error* e = nullptr;
            auto from_sensor = get_sensor_profile(from_stream, from_id);
            auto to_sensor   = get_sensor_profile(to_stream, to_id);
            rs2_set_extrinsics(from_sensor.first.get().get(), from_sensor.second.get(), to_sensor.first.get().get(), to_sensor.second.get(), &extrinsics, &e);
            error::handle(e);
        }

        /** 
        * Set tm2 motion device intrinsics
        * \param[in] stream_type       stream type of the motion device
        * \param[in] motion_intriniscs intrinsics value to be written to the device
        */
        void set_motion_device_intrinsics(rs2_stream stream_type, const rs2_motion_device_intrinsic& motion_intriniscs)
        {
            rs2_error* e = nullptr;
            auto motion_sensor = get_sensor_profile(stream_type, 0);
            rs2_set_motion_device_intrinsics(motion_sensor.first.get().get(), motion_sensor.second.get(), &motion_intriniscs, &e);
            error::handle(e);
        }

    private:

        std::pair<sensor, stream_profile> get_sensor_profile(rs2_stream stream_type, int stream_index) {
            for (auto s : query_sensors()) {
                for (auto p : s.get_stream_profiles()) {
                    if (p.stream_type() == stream_type && p.stream_index() == stream_index)
                        return std::pair<sensor, stream_profile>(s, p);
                }
            }
            return std::pair<sensor, stream_profile>();
         }
    };
}
#endif // LIBREALSENSE_RS2_DEVICE_HPP
