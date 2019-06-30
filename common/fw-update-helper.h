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

    int parse_product_line(std::string id);
    std::string get_available_firmware_version(int product_line);
    std::map<int, std::vector<uint8_t>> create_default_fw_table();
    std::vector<int> parse_fw_version(const std::string& fw);
    bool is_upgradeable(const std::string& curr, const std::string& available);

    class firmware_update_manager : public std::enable_shared_from_this<firmware_update_manager>
    {
    public:
        firmware_update_manager(device_model& model, device dev, context ctx, std::vector<uint8_t> fw) 
            : _dev(dev), _fw(fw), _model(model), _ctx(ctx) {}

        void start();
        int get_progress() const { return _progress; }
        bool done() const { return _done; }
        bool started() const { return _started; }
        bool failed() const { return _failed; }
        const std::string& get_log() const { return _log; }

        void check_error(std::string& error) { if (_failed) error = _last_error; }

        void log(std::string line);
        void fail(std::string error);

    private:
        void do_update(std::function<void()> cleanup);
        bool check_for(
            std::function<bool()> action, std::function<void()> cleanup,
            std::chrono::system_clock::duration delta);

        std::string _log;
        bool _started = false;
        bool _done = false;
        bool _failed = false;
        int _progress = 0;
        device _dev;
        std::vector<uint8_t> _fw;
        device_model& _model;
        std::mutex _log_lock;
        std::string _last_error;

        context _ctx;
    };
}