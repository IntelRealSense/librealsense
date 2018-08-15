// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <vector>

#include <windows.h>

#define WAIT_FOR_MUTEX_TIME_OUT  (5000)

namespace librealsense
{
    namespace platform
    {
        bool check(const char * call, HRESULT hr, bool to_throw = true);
#define CHECK_HR(x) check(#x, x);
#define LOG_HR(x) check(#x, x, false);

        std::string win_to_utf(const WCHAR * s);

        bool is_win10_redstone2();

        std::vector<std::string> tokenize(std::string string, char separator);

        bool parse_usb_path(uint16_t & vid, uint16_t & pid, uint16_t & mi, std::string & unique_id, std::string & guid, const std::string & path);

        bool get_usb_descriptors(uint16_t device_vid, uint16_t device_pid, const std::string& device_uid,
            std::string& location, usb_spec& spec);

        class event_base
        {
        public:
            virtual ~event_base();
            virtual bool set();
            virtual bool wait(DWORD timeout) const;

            static event_base* wait(const std::vector<event_base*>& events, bool waitAll, int timeout);
            static event_base* wait_any(const std::vector<event_base*>& events, int timeout);
            static event_base* wait_all(const std::vector<event_base*>& events, int timeout);

            HANDLE get_handle() const { return _handle; }

        protected:
            explicit event_base(HANDLE handle);

            HANDLE _handle;

        private:
            event_base() = delete;

            // Disallow copy:
            event_base(const event_base&) = delete;
            event_base& operator=(const event_base&) = delete;
        };

        class auto_reset_event : public event_base
        {
        public:
            auto_reset_event();
        };

        class manual_reset_event : public event_base
        {
        public:
            manual_reset_event();

            bool reset() const;
        };

        enum create_and_open_status
        {
            Mutex_Succeed,
            Mutex_TotalFailure,
            Mutex_AlreadyExist
        };

        class named_mutex
        {
        public:
            named_mutex(const char* id, unsigned timeout);
            ~named_mutex();
            bool try_lock() const;
            void lock() const { acquire(); }
            void unlock() const { release(); }

        private:
            create_and_open_status create_named_mutex(const char* camID);
            create_and_open_status open_named_mutex(const char* camID);
            void update_id(const char* id);
            void acquire() const;
            void release() const;
            void close();

            unsigned _timeout;
            HANDLE _winusb_mutex;
        };

    }
}
