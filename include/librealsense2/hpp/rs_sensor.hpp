// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SENSOR_HPP
#define LIBREALSENSE_RS2_SENSOR_HPP

#include "rs_types.hpp"

namespace rs2
{
    class stream_profile
    {
    public:
        stream_profile() : _profile(nullptr) {}

        int stream_index() const { return _index; }
        rs2_stream stream_type() const { return _type; }
        rs2_format format() const { return _format; }

        int fps() const { return _framerate; }

        int unique_id() const { return _uid; }

        stream_profile clone(rs2_stream type, int index, rs2_format format) const
        {
            rs2_error* e = nullptr;
            auto ref = rs2_clone_stream_profile(_profile, type, index, format, &e);
            error::handle(e);
            stream_profile res(ref);
            res._clone = std::shared_ptr<rs2_stream_profile>(ref, [](rs2_stream_profile* r) { rs2_delete_stream_profile(r); });

            return res;
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

        std::string stream_name() const
        {
            std::stringstream ss;
            ss << rs2_stream_to_string(stream_type());
            if (stream_index() != 0) ss << " " << stream_index();
            return ss.str();
        }

        bool is_default() const { return _default; }
        std::size_t size() const { return _size; }

        operator bool() const { return _profile != nullptr; }

        const rs2_stream_profile* get() const { return _profile; }

        rs2_extrinsics get_extrinsics_to(const stream_profile& to) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics res;
            rs2_get_extrinsics(get(), to.get(), &res, &e);
            error::handle(e);
            return res;
        }

    protected:
        friend class sensor;
        friend class frame;
        friend class pipeline;

        explicit stream_profile(const rs2_stream_profile* profile) : _profile(profile)
        {
            rs2_error* e = nullptr;
            rs2_get_stream_profile_data(_profile, &_type, &_format, &_index, &_uid, &_framerate, &e);
            error::handle(e);

            _default = !!(rs2_is_stream_profile_default(_profile, &e));
            error::handle(e);

            _size = rs2_get_stream_profile_size(_profile, &e);
            error::handle(e);
        }

        const rs2_stream_profile* _profile;
        std::shared_ptr<rs2_stream_profile> _clone;

        int _index = 0;
        int _uid = 0;
        int _framerate = 0;
        rs2_format _format = RS2_FORMAT_ANY;
        rs2_stream _type = RS2_STREAM_ANY;

        bool _default = false;
        size_t _size = 0;
    };

    class video_stream_profile : public stream_profile
    {
    public:
        explicit video_stream_profile(const stream_profile& sp)
            : stream_profile(sp)
        {
            rs2_error* e = nullptr;
            if ((rs2_stream_profile_is(sp.get(), RS2_EXTENSION_VIDEO_PROFILE, &e) == 0 && !e))
            {
                _profile = nullptr;
            }
            error::handle(e);

            if (_profile)
            {
                rs2_get_video_stream_resolution(_profile, &_width, &_height, &e);
                error::handle(e);
            }
        }

        int width() const
        {
            return _width;
        }

        int height() const
        {
            return _height;
        }

        rs2_intrinsics get_intrinsics() const
        {
            rs2_error* e = nullptr;
            rs2_intrinsics intr;
            rs2_get_video_stream_intrinsics(_profile, &intr, &e);
            error::handle(e);
            return intr;
        }

    private:
        int _width = 0;
        int _height = 0;
    };

    class notification
    {
    public:
        notification(rs2_notification* notification)
        {
            rs2_error* e = nullptr;
            _description = rs2_get_notification_description(notification, &e);
            error::handle(e);
            _timestamp = rs2_get_notification_timestamp(notification, &e);
            error::handle(e);
            _severity = rs2_get_notification_severity(notification, &e);
            error::handle(e);
            _category = rs2_get_notification_category(notification, &e);
            error::handle(e);
        }

        notification()
            : _description(""),
              _timestamp(-1),
              _severity(RS2_LOG_SEVERITY_COUNT),
              _category(RS2_NOTIFICATION_CATEGORY_COUNT)
        {}

        /**
        * retrieve the notification category
        * \return            the notification category
        */
        rs2_notification_category get_category() const
        {
            return _category;
        }
        /**
        * retrieve the notification description
        * \return            the notification description
        */
        std::string get_description() const
        {
            return _description;
        }

        /**
        * retrieve the notification arrival timestamp
        * \return            the arrival timestamp
        */
        double get_timestamp() const
        {
            return _timestamp;
        }

        /**
        * retrieve the notification severity
        * \return            the severity
        */
        rs2_log_severity get_severity() const
        {
            return _severity;
        }

    private:
        std::string _description;
        double _timestamp;
        rs2_log_severity _severity;
        rs2_notification_category _category;
    };

    template<class T>
    class notifications_callback : public rs2_notifications_callback
    {
        T on_notification_function;
    public:
        explicit notifications_callback(T on_notification) : on_notification_function(on_notification) {}

        void on_notification(rs2_notification* _notification) override
        {
            on_notification_function(notification{ _notification });
        }

        void release() override { delete this; }
    };

    class sensor
    {
    public:
        /**
        * open subdevice for exclusive access, by committing to a configuration
        * \param[in] profile    configuration commited by the device
        */
        void open(const stream_profile& profile) const
        {
            rs2_error* e = nullptr;
            rs2_open(_sensor.get(),
                profile.get(),
                &e);
            error::handle(e);
        }

        /**
        * check if specific camera info is supported
        * \param[in] info    the parameter to check for support
        * \return                true if the parameter both exist and well-defined for the specific device
        */
        bool supports(rs2_camera_info info) const
        {
            rs2_error* e = nullptr;
            auto is_supported = rs2_supports_sensor_info(_sensor.get(), info, &e);
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
            auto result = rs2_get_sensor_info(_sensor.get(), info, &e);
            error::handle(e);
            return result;
        }

        /**
        * open subdevice for exclusive access, by committing to composite configuration, specifying one or more stream profiles
        * this method should be used for interdependent  streams, such as depth and infrared, that have to be configured together
        * \param[in] profiles   vector of configurations to be commited by the device
        */
        void open(const std::vector<stream_profile>& profiles) const
        {
            rs2_error* e = nullptr;

            std::vector<const rs2_stream_profile*> profs;
            profs.reserve(profiles.size());
            for (auto& p : profiles)
            {
                profs.push_back(p.get());
            }

            rs2_open_multiple(_sensor.get(),
                profs.data(),
                static_cast<int>(profiles.size()),
                &e);
            error::handle(e);
        }

        /**
        * close subdevice for exclusive access
        * this method should be used for releasing device resource
        */
        void close() const
        {
            rs2_error* e = nullptr;
            rs2_close(_sensor.get(), &e);
            error::handle(e);
        }

        /**
        * Start passing frames into user provided callback
        * \param[in] callback   Stream callback, can be any callable object accepting rs2::frame
        */
        template<class T>
        void start(T callback) const
        {
            rs2_error* e = nullptr;
            rs2_start_cpp(_sensor.get(), new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * stop streaming
        */
        void stop() const
        {
            rs2_error* e = nullptr;
            rs2_stop(_sensor.get(), &e);
            error::handle(e);
        }

        /**
        * check if particular option is read-only
        * \param[in] option     option id to be checked
        * \return true if option is read-only
        */
        bool is_option_read_only(rs2_option option)
        {
            rs2_error* e = nullptr;
            auto res = rs2_is_option_read_only(_sensor.get(), option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * register notifications callback
        * \param[in] callback   notifications callback
        */
        template<class T>
        void set_notifications_callback(T callback) const
        {
            rs2_error* e = nullptr;
            rs2_set_notifications_callback_cpp(_sensor.get(),
                new notifications_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * read option value from the device
        * \param[in] option   option id to be queried
        * \return value of the option
        */
        float get_option(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option(_sensor.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * retrieve the available range of values of a supported option
        * \return option  range containing minimum and maximum values, step and default value
        */
        option_range get_option_range(rs2_option option) const
        {
            option_range result;
            rs2_error* e = nullptr;
            rs2_get_option_range(_sensor.get(), option,
                &result.min, &result.max, &result.step, &result.def, &e);
            error::handle(e);
            return result;
        }

        /**
        * write new value to device option
        * \param[in] option     option id to be queried
        * \param[in] value      new value for the option
        */
        void set_option(rs2_option option, float value) const
        {
            rs2_error* e = nullptr;
            rs2_set_option(_sensor.get(), option, value, &e);
            error::handle(e);
        }

        /**
        * check if particular option is supported by a subdevice
        * \param[in] option     option id to be checked
        * \return true if option is supported
        */
        bool supports(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_supports_option(_sensor.get(), option, &e);
            error::handle(e);
            return res > 0;
        }

        /**
        * get option description
        * \param[in] option     option id to be checked
        * \return human-readable option description
        */
        const char* get_option_description(rs2_option option) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_description(_sensor.get(), option, &e);
            error::handle(e);
            return res;
        }

        /**
        * get option value description (in case specific option value hold special meaning)
        * \param[in] option     option id to be checked
        * \param[in] value      value of the option
        * \return human-readable description of a specific value of an option or null if no special meaning
        */
        const char* get_option_value_description(rs2_option option, float val) const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_option_value_description(_sensor.get(), option, val, &e);
            error::handle(e);
            return res;
        }

        /**
        * check if physical subdevice is supported
        * \return   list of stream profiles that given subdevice can provide, should be released by rs2_delete_profiles_list
        */
        std::vector<stream_profile> get_stream_profiles() const
        {
            std::vector<stream_profile> results;

            rs2_error* e = nullptr;
            std::shared_ptr<rs2_stream_profile_list> list(
                rs2_get_stream_profiles(_sensor.get(), &e),
                rs2_delete_stream_profiles_list);
            error::handle(e);

            auto size = rs2_get_stream_profiles_count(list.get(), &e);
            error::handle(e);

            for (auto i = 0; i < size; i++)
            {
                stream_profile profile(rs2_get_stream_profile(list.get(), i, &e));
                error::handle(e);
                results.push_back(profile);
            }

            return results;
        }

        /**
         * returns scale and bias of a motion stream
         * \param stream    Motion stream type (Gyro / Accel / ...)
         */
        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) {
            rs2_error *e = nullptr;
            rs2_motion_device_intrinsic intrin;
            rs2_get_motion_intrinsics(_sensor.get(), stream, &intrin, &e);
            error::handle(e);
            return intrin;
        }

        sensor& operator=(const std::shared_ptr<rs2_sensor> dev)
        {
            _sensor.reset();
            _sensor = dev;
            return *this;
        }
        sensor& operator=(const sensor& dev)
        {
            *this = nullptr;
            _sensor = dev._sensor;
            return *this;
        }
        sensor() : _sensor(nullptr) {}

        operator bool() const
        {
            return _sensor != nullptr;
        }

        const std::shared_ptr<rs2_sensor>& get() const
        {
            return _sensor;
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

    protected:
        friend context;
        friend device_list;
        friend device;
        friend device_base;
        friend roi_sensor;

        std::shared_ptr<rs2_sensor> _sensor;
        explicit sensor(std::shared_ptr<rs2_sensor> dev)
            : _sensor(dev)
        {
        }
    };

    inline bool operator==(const sensor& lhs, const sensor& rhs)
    {
        if (!(lhs.supports(RS2_CAMERA_INFO_NAME) && lhs.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)
            && rhs.supports(RS2_CAMERA_INFO_NAME) && rhs.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)))
            return false;

        return std::string(lhs.get_info(RS2_CAMERA_INFO_NAME)) == rhs.get_info(RS2_CAMERA_INFO_NAME)
            && std::string(lhs.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)) == rhs.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    }

    class roi_sensor : public sensor
    {
    public:
        roi_sensor(sensor s)
            : sensor(s.get())
        {
            rs2_error* e = nullptr;
            if(rs2_is_sensor_extendable_to(_sensor.get(), RS2_EXTENSION_ROI, &e) == 0 && !e)
            {
                _sensor = nullptr;
            }
            error::handle(e);
        }

        void set_region_of_interest(const region_of_interest& roi)
        {
            rs2_error* e = nullptr;
            rs2_set_region_of_interest(_sensor.get(), roi.min_x, roi.min_y, roi.max_x, roi.max_y, &e);
            error::handle(e);
        }

        region_of_interest get_region_of_interest() const
        {
            region_of_interest roi {};
            rs2_error* e = nullptr;
            rs2_get_region_of_interest(_sensor.get(), &roi.min_x, &roi.min_y, &roi.max_x, &roi.max_y, &e);
            error::handle(e);
            return roi;
        }

        operator bool() const { return _sensor.get() != nullptr; }
    };

    class depth_sensor : public sensor
    {
    public:
        depth_sensor(sensor s)
            : sensor(s.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_sensor_extendable_to(_sensor.get(), RS2_EXTENSION_DEPTH_SENSOR, &e) == 0 && !e)
            {
                _sensor = nullptr;
            }
            error::handle(e);
        }

        /** Retrieves mapping between the units of the depth image and meters
        * \return depth in meters corresponding to a depth value of 1
        */
        float get_depth_scale() const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_depth_scale(_sensor.get(), &e);
            error::handle(e);
            return res;
        }

        operator bool() const { return _sensor.get() != nullptr; }
    };
}
#endif // LIBREALSENSE_RS2_SENSOR_HPP
