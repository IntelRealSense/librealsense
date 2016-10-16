// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace rsimpl
{
    namespace uvc
    {
        void check(const char * call, HRESULT hr, bool to_throw = true);
#define CHECK_HR(x) check(#x, x);
#define LOG_HR(x) check(#x, x, false);

        std::string win_to_utf(const WCHAR * s);

        std::vector<std::string> tokenize(std::string string, char separator);

        bool parse_usb_path(int & vid, int & pid, int & mi, std::string & unique_id, const std::string & path);

        std::string get_usb_port_id(int device_vid, int device_pid, const std::string& device_uid);

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
    }
}
