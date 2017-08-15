// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_RECORD_PLAYBACK_HPP
#define LIBREALSENSE_RS2_RECORD_PLAYBACK_HPP

#include "rs2_types.hpp"
#include "rs2_device.hpp"

namespace rs2
{
    class playback : public device
    {
    public:
        playback(device d) : playback(d.get()) {}
        void pause()
        {
            rs2_error* e = nullptr;
            rs2_playback_device_pause(_dev.get(), &e);
            error::handle(e);
        }
        void resume()
        {
            rs2_error* e = nullptr;
            rs2_playback_device_resume(_dev.get(), &e);
            error::handle(e);
        }

        std::string file_name() const
        {
            return m_file; //retrieved in construction
        }
        uint64_t get_position() const
        {
            rs2_error* e = nullptr;
            uint64_t pos = rs2_playback_get_position(_dev.get(), &e);
            error::handle(e);
            return pos;
        }
        std::chrono::nanoseconds get_duration() const
        {
            rs2_error* e = nullptr;
            std::chrono::nanoseconds duration(rs2_playback_get_duration(_dev.get(), &e));
            error::handle(e);
            return duration;
        }
        void seek(std::chrono::nanoseconds time)
        {
            rs2_error* e = nullptr;
            rs2_playback_seek(_dev.get(), time.count(), &e);
            error::handle(e);
        }

        bool is_real_time() const
        {
            rs2_error* e = nullptr;
            bool real_time = rs2_playback_device_is_real_time(_dev.get(), &e) != 0;
            error::handle(e);
            return real_time;
        }

        void set_real_time(bool real_time) const
        {
            rs2_error* e = nullptr;
            rs2_playback_device_set_real_time(_dev.get(), (real_time ? 1 : 0), &e);
            error::handle(e);
        }
        template <typename T>
        void set_status_changed_callback(T callback)
        {
            rs2_error* e = nullptr;
            rs2_playback_device_set_status_changed_callback(_dev.get(), new status_changed_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        rs2_playback_status current_status() const
        {
            rs2_error* e = nullptr;
            rs2_playback_status sts = rs2_playback_device_get_current_status(_dev.get(), &e);
            error::handle(e);
            return sts;
        }
    protected:
        friend context;
        explicit playback(std::shared_ptr<rs2_device> dev) : device(dev)
        {
            rs2_error* e = nullptr;
            if(rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_PLAYBACK, &e) == 0 && !e)
            {
                _dev = nullptr;
            }
            error::handle(e);

            if(_dev)
            {
                e = nullptr;
                m_file = rs2_playback_device_get_file_path(_dev.get(), &e);
                error::handle(e);
            }
        }
    private:
        std::string m_file;
    };
    class recorder : public device
    {
    public:
        recorder(device d) : recorder(d.get()) {}

        recorder(const std::string& file, rs2::device device)
        {
            rs2_error* e = nullptr;
            _dev = std::shared_ptr<rs2_device>(
                rs2_create_record_device(device.get().get(), file.c_str(), &e),
                rs2_delete_device);
            rs2::error::handle(e);
        }

        void pause()
        {
            rs2_error* e = nullptr;
            rs2_record_device_pause(_dev.get(), &e);
            error::handle(e);
        }
        void resume()
        {
            rs2_error* e = nullptr;
            rs2_record_device_resume(_dev.get(), &e);
            error::handle(e);
        }
    protected:
        explicit recorder(std::shared_ptr<rs2_device> dev) : device(dev)
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_RECORD, &e) == 0 && !e)
            {
                _dev = nullptr;
            }
            error::handle(e);
        }
    };
}
#endif // LIBREALSENSE_RS2_RECORD_PLAYBACK_HPP
