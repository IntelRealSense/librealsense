// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

namespace rs2
{
    class device_model;
    class notification_model;

    class process_manager : public std::enable_shared_from_this<process_manager>
    {
    public:
        process_manager(std::string name, device_model& model)
            : _process_name(name), _model(model) {}

        void start(std::shared_ptr<notification_model> n);
        int get_progress() const { return _progress; }
        bool done() const { return _done; }
        bool started() const { return _started; }
        bool failed() const { return _failed; }
        const std::string& get_log() const { return _log; }
        void reset();

        void check_error(std::string& error) { if (_failed) error = _last_error; }

        void log(std::string line);
        void fail(std::string error);

    protected:
        virtual void process_flow(std::function<void()> cleanup) = 0;

        std::string _log;
        bool _started = false;
        bool _done = false;
        bool _failed = false;
        int _progress = 0;
  
        std::mutex _log_lock;
        std::string _last_error;
        std::string _process_name;

        device_model& _model;
    };
}