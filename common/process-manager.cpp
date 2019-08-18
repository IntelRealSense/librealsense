// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "process-manager.h"

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <model-views.h>

#include "os.h"

namespace rs2
{
    void process_manager::log(std::string line)
    {
        std::lock_guard<std::mutex> lock(_log_lock);
        _log += line + "\n";
    }

    void process_manager::reset()
    {
        _progress = 0;
        _started = false;
        _done = false;
        _failed = false;
        _last_error = "";
    }

    void process_manager::fail(std::string error)
    {
        _last_error = error;
        _progress = 0;
        log("\nERROR: " + error);
        _failed = true;
    }

    void process_manager::start(std::shared_ptr<notification_model> n)
    {
        auto cleanup = [n]() {
            //n->dismiss(false);
        };

        log(to_string() << "Started " << _process_name << " process");

        auto me = shared_from_this();
        std::weak_ptr<process_manager> ptr(me);
        
        std::thread t([ptr, cleanup]() {
            auto self = ptr.lock();
            if (!self) return;

            try
            {
                self->process_flow(cleanup);
            }
            catch (const error& e)
            {
                self->fail(error_to_string(e));
                cleanup();
            }
            catch (const std::exception& ex)
            {
                self->fail(ex.what());
                cleanup();
            }
            catch (...)
            {
                self->fail(to_string() << "Unknown error during " << self->_process_name << " process!");
                cleanup();
            }
        });
        t.detach();

        _started = true;
    }
}
