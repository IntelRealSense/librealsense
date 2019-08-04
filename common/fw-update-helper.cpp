// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#include "fw-update-helper.h"

#include <map>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <model-views.h>

#include "os.h"

#ifdef INTERNAL_FW
#include "common/fw/D4XX_FW_Image.h"
#include "common/fw/SR3XX_FW_Image.h"
#else
#define FW_D4XX_FW_IMAGE_VERSION ""
#define FW_SR3XX_FW_IMAGE_VERSION ""
#endif // INTERNAL_FW

namespace rs2
{
    static std::map<int, std::string> product_line_to_fw =
    {
        {RS2_PRODUCT_LINE_D400, FW_D4XX_FW_IMAGE_VERSION},
        {RS2_PRODUCT_LINE_SR300, FW_SR3XX_FW_IMAGE_VERSION},
    };

    int parse_product_line(std::string id)
    {
        if (id == "D400") return RS2_PRODUCT_LINE_D400;
        else if (id == "SR300") return RS2_PRODUCT_LINE_SR300;
        else return -1;
    }

    std::string get_available_firmware_version(int product_line)
    {
        auto it = product_line_to_fw.find(product_line);
        if (it != product_line_to_fw.end())
            return it->second;
        return "";
    }

    std::map<int, std::vector<uint8_t>> create_default_fw_table()
    {
        std::map<int, std::vector<uint8_t>> rv;

        if ("" != FW_D4XX_FW_IMAGE_VERSION)
        {
            int size = 0;
            auto hex = fw_get_D4XX_FW_Image(size);
            auto vec = std::vector<uint8_t>(hex, hex + size);
            rv[RS2_PRODUCT_LINE_D400] = vec;
        }

        if ("" != FW_SR3XX_FW_IMAGE_VERSION)
        {
            int size = 0;
            auto hex = fw_get_SR3XX_FW_Image(size);
            auto vec = std::vector<uint8_t>(hex, hex + size);
            rv[RS2_PRODUCT_LINE_SR300] = vec;
        }

        return rv;
    }

    std::vector<int> parse_fw_version(const std::string& fw)
    {
        std::vector<int> rv;
        size_t pos = 0;
        std::string delimiter = ".";
        auto str = fw + delimiter;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            auto s = str.substr(0, pos);
            int val = std::stoi(s);
            rv.push_back(val);
            str.erase(0, pos + delimiter.length());
        }
        return rv;
    }

    bool is_upgradeable(const std::string& curr, const std::string& available)
    {
        if (curr == "" || available == "") return false;

        size_t fw_string_size = 4;
        auto c = parse_fw_version(curr);
        auto a = parse_fw_version(available);
        if (a.size() != fw_string_size || c.size() != fw_string_size)
            return false;

        for (int i = 0; i < fw_string_size; i++) {
            if (c[i] > a[i])
                return false;
            if (c[i] < a[i])
                return true;
        }
        return false; //equle
    }

    void firmware_update_manager::log(std::string line)
    {
        std::lock_guard<std::mutex> lock(_log_lock);
        _log += line + "\n";
    }

    void firmware_update_manager::fail(std::string error)
    {
        _last_error = error;
        _progress = 0;
        log("\nERROR: " + error);
        log("\nFirmware Update process is safe.\nSimply reconnect the device to get it working again");
        _failed = true;
    }

    bool firmware_update_manager::check_for(
        std::function<bool()> action, std::function<void()> cleanup,
        std::chrono::system_clock::duration delta)
    {
        using namespace std;
        using namespace std::chrono;

        auto start = system_clock::now();
        auto now = system_clock::now();
        do
        {
            try
            {

                if (action()) return true;
            }
            catch (const error& e)
            {
                fail(error_to_string(e));
                cleanup();
                return false;
            }
            catch (const std::exception& ex)
            {
                fail(ex.what());
                cleanup();
                return false;
            }
            catch (...)
            {
                fail("Unknown error during update.\nPlease reconnect the camera to exit recovery mode");
                cleanup();
                return false;
            }

            now = system_clock::now();
            this_thread::sleep_for(milliseconds(100));
        } while (now - start < delta);
        return false;
    }

    void firmware_update_manager::do_update(std::function<void()> cleanup)
    {
        try
        {
            std::string serial = "";
            if (_dev.supports(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER))
                serial = _dev.get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER);
            else
                serial = _dev.query_sensors().front().get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER);

            _progress = 5;

            int next_progress = 10;

            update_device dfu{};

            if (auto upd = _dev.as<updatable>())
            {
                log("Backing-up camera flash memory");

                auto flash = upd.create_flash_backup([&](const float progress)
                {
                    _progress = ((ceil(progress * 5) / 5) * (30 - next_progress)) + next_progress;
                });

                auto temp = get_folder_path(special_folder::app_data);
                temp += serial + "." + get_timestamped_file_name() + ".bin";

                {
                    std::ofstream file(temp.c_str(), std::ios::binary);
                    file.write((const char*)flash.data(), flash.size());
                }

                std::string log_line = "Backup completed and saved as '";
                log_line += temp + "'";
                log(log_line);

                next_progress = 40;

                if (_is_signed)
                {
                    log("Requesting to switch to recovery mode");
                    upd.enter_update_state();

                    if (!check_for([this, serial, &dfu]() {
                        auto devs = _ctx.query_devices();

                        for (int j = 0; j < devs.size(); j++)
                        {
                            try
                            {
                                auto d = devs[j];
                                if (d.is<update_device>())
                                {
                                    if (d.supports(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER))
                                    {
                                        if (serial == d.get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER))
                                        {
                                            dfu = d;
                                            return true;
                                        }
                                    }
                                }
                            }
                            catch (...) {}
                        }

                        return false;
                    }, cleanup, std::chrono::seconds(60)))
                    {
                        fail("Recovery device did not connect in time!");
                        return;
                    }
                }
            }
            else
            {
                dfu = _dev.as<update_device>();
            }

            if (dfu)
            {
                _progress = next_progress;

                log("Recovery device connected, starting update");

                dfu.update(_fw, [&](const float progress)
                {
                    _progress = (ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress;
                });

                log("Firmware Download completed, await DFU transition event");
                std::this_thread::sleep_for(std::chrono::seconds(3));
                log("Firmware Update completed, waiting for device to reconnect");
            }
            else
            {
                auto upd = _dev.as<updatable>();
                upd.update_unsigned(_fw, [&](const float progress)
                {
                    _progress = (ceil(progress * 10) / 10 * (90 - next_progress)) + next_progress;
                });
                log("Firmware Update completed, waiting for device to reconnect");
            }

            if (!check_for([this, serial, &dfu]() {
                auto devs = _ctx.query_devices();

                for (int j = 0; j < devs.size(); j++)
                {
                    try
                    {
                        auto d = devs[j];

                        if (d.query_sensors().size() && d.query_sensors().front().supports(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER))
                        {
                            auto s = d.query_sensors().front().get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER);
                            if (s == serial)
                            {
                                log("Discovered connection of the original device");
                                return true;
                            }
                        }
                    }
                    catch (...) {}
                }

                return false;
            }, cleanup, std::chrono::seconds(60)))
            {
                fail("Original device did not reconnect in time!");
                return;
            }

            log("Device reconnected succesfully!");

            _progress = 100;

            _done = true;
        }
        catch (const error& e)
        {
            fail(error_to_string(e));
            cleanup();
        }
        catch (const std::exception& ex)
        {
            fail(ex.what());
            cleanup();
        }
        catch (...)
        {
            fail("Unknown error during update.\nPlease reconnect the camera to exit recovery mode");
            cleanup();
        }
    }

    void firmware_update_manager::start()
    {
        auto cleanup = _model.cleanup;
        _model.cleanup = [] {};

        log("Started update process");

        auto me = shared_from_this();
        std::weak_ptr<firmware_update_manager> ptr(me);
        
        std::thread t([ptr, cleanup]() {
            auto self = ptr.lock();
            if (!self) return;

            self->do_update(cleanup);
        });
        t.detach();

        _started = true;
    }
}
