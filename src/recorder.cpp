// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "recorder.h"
#include <fstream>
#include "sql.h"

using namespace std;
using namespace sql;

const char* CONFIG_TABLE = "rs_config";
const char* CONFIG_QUERY = "SELECT value FROM rs_config WHERE key = ?";
const char* CONFIG_CREATE = "CREATE TABLE rs_config(key TEXT PRIMARY KEY, value TEXT)";
const char* CONFIG_INSERT = "INSERT OR REPLACE INTO rs_config(key, value) VALUES(?, ?)";
const char* API_VERSION_KEY = "api_version";

const char* CALLS_CREATE = "CREATE TABLE rs_calls(type NUMBER, timestamp NUMBER, entity_id NUMBER, param1 NUMBER, param2 NUMBER, param3 NUMBER, param4 NUMBER)";
const char* CALLS_INSERT = "INSERT INTO rs_calls(type, timestamp, entity_id, param1, param2, param3, param4) VALUES(?, ?, ?, ?, ?, ?, ?)";
const char* CALLS_SELECT_ALL = "SELECT * FROM rs_calls";

const char* DEVICE_INFO_CREATE = "CREATE TABLE rs_device_info(type NUMBER, id TEXT, uid TEXT, pid NUMBER, vid NUMBER, mi NUMBER)";
const char* DEVICE_INFO_SELECT_ALL = "SELECT * FROM rs_device_info";
const char* DEVICE_INFO_INSERT = "INSERT INTO rs_device_info(type, id, uid, pid, vid, mi) VALUES(?, ?, ?, ?, ?, ?)";

const char* BLOBS_CREATE = "CREATE TABLE rs_blobs(data BLOB)";
const char* BLOBS_INSERT = "INSERT INTO rs_blobs(data) VALUES(?)";
const char* BLOBS_SELECT_ALL = "SELECT * FROM rs_blobs";

namespace rsimpl
{
    namespace uvc
    {
        enum device_type
        {
            uvc,
            usb
        };

        inline bool file_exists(const char* filename)
        {
            ifstream f(filename);
            return f.good();
        }


        recording::recording(): cursor(0)
        {
            start_time = chrono::high_resolution_clock::now();
        }

        void recording::save(const char* filename) const
        {
            if (file_exists(filename)) 
                throw runtime_error("Can't save over existing recording!");
            connection c(filename);

            c.execute(CONFIG_CREATE);
            c.execute(CALLS_CREATE);
            c.execute(DEVICE_INFO_CREATE);
            c.execute(BLOBS_CREATE);

            {
                statement insert(c, CONFIG_INSERT);
                insert.bind(1, API_VERSION_KEY);
                insert.bind(2, RS_API_VERSION_STR);
                insert();
            }

            for (auto&& cl : calls)
            {
                statement insert(c, CALLS_INSERT);
                insert.bind(1, static_cast<int>(cl.type));
                insert.bind(2, cl.timestamp);
                insert.bind(3, cl.entity_id);
                insert.bind(4, cl.param1);
                insert.bind(5, cl.param2);
                insert.bind(6, cl.param3);
                insert.bind(7, cl.param4);
                insert();
            }

            for (auto&& uvc_info : uvc_device_infos)
            {
                statement insert(c, DEVICE_INFO_INSERT);
                insert.bind(1, uvc);
                insert.bind(2, "");
                insert.bind(3, uvc_info.unique_id.c_str());
                insert.bind(4, uvc_info.pid);
                insert.bind(5, uvc_info.vid);
                insert.bind(6, uvc_info.mi);
                insert();
            }

            for (auto&& usb_info : usb_device_infos)
            {
                statement insert(c, DEVICE_INFO_INSERT);
                insert.bind(1, usb);
                string id(usb_info.id.begin(), usb_info.id.end());
                insert.bind(2, id.c_str());
                insert.bind(3, usb_info.unique_id.c_str());
                insert.bind(4, usb_info.pid);
                insert.bind(5, usb_info.vid);
                insert.bind(6, usb_info.mi);
                insert();
            }

            for (auto&& blob : blobs)
            {
                statement insert(c, BLOBS_INSERT);
                insert.bind(1, blob);
                insert();
            }
        }

        shared_ptr<recording> recording::load(const char* filename)
        {
            if (!file_exists(filename))
            {
                throw runtime_error("Recording file not found!");
            }

            auto result = make_shared<recording>();

            connection c(filename);

            if (!c.table_exists(CONFIG_TABLE))
            {
                throw runtime_error("Invalid recording, missing config section!");
            }

            statement select_magic(c, CONFIG_QUERY);

            statement select_api_version(c, CONFIG_QUERY);
            select_api_version.bind(1, API_VERSION_KEY);
            auto api_version = select_api_version()[0].get_string();

            statement select_calls(c, CALLS_SELECT_ALL);
            for (auto&& row : select_calls)
            {
                call cl;
                cl.type = static_cast<call_type>(row[0].get_int());
                cl.timestamp = row[1].get_int();
                cl.entity_id = row[2].get_int();
                cl.param1 = row[3].get_int();
                cl.param2 = row[4].get_int();
                cl.param3 = row[5].get_int();
                cl.param4 = row[6].get_int();
                result->calls.push_back(cl);
            }

            statement select_devices(c, DEVICE_INFO_SELECT_ALL);
            for (auto&& row : select_devices)
            {
                if (row[0].get_int() == usb)
                {
                    usb_device_info info;
                    auto id = row[1].get_string();
                    info.id = wstring(id.begin(), id.end());
                    info.unique_id = row[2].get_string();
                    info.pid = row[3].get_int();
                    info.vid = row[4].get_int();
                    info.mi = row[5].get_int();
                    result->usb_device_infos.push_back(info);
                }
                else
                {
                    uvc_device_info info;
                    info.unique_id = row[2].get_string();
                    info.pid = row[3].get_int();
                    info.vid = row[4].get_int();
                    info.mi = row[5].get_int();
                    result->uvc_device_infos.push_back(info);
                }
            }

            statement select_blobs(c, BLOBS_SELECT_ALL);
            for (auto&& row : select_blobs)
            {
                result->blobs.push_back(row[0].get_blob());
            }

            return result;
        }

        int recording::save_blob(void* ptr, unsigned size)
        {
            lock_guard<mutex> lock(_mutex);
            vector<uint8_t> holder;
            holder.resize(size);
            memcpy(holder.data(), ptr, size);
            int id = blobs.size();
            blobs.push_back(holder);
            return id;
        }

        int recording::get_timestamp() const
        {
            using namespace chrono;
            auto diff = high_resolution_clock::now() - start_time;
            return duration_cast<milliseconds>(diff).count();
        }

        call& recording::find_call(call_type t, int entity_id)
        {
            lock_guard<mutex> lock(_mutex);
            for (auto i = 1; i <= calls.size(); i++)
            {
                const auto idx = (cursor + i) % calls.size();
                if (calls[idx].type == t && calls[idx].entity_id == entity_id)
                {
                    cursor = idx;
                    return calls[cursor];
                }
            }
            throw runtime_error("The recording is missing the part you are trying to playback!");
        }

        vector<uint8_t> record_usb_device::send_receive(const vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            auto result = _source->send_receive(data, timeout_ms, require_response);

            auto&& c = _rec->add_call(_entity_id, call_type::send_command);
            c.param1 = _rec->save_blob((void*)data.data(), data.size());
            c.param2 = _rec->save_blob((void*)result.data(), result.size());
            c.param3 = timeout_ms;
            c.param4 = require_response;

            return result;
        }

        shared_ptr<uvc_device> record_backend::create_uvc_device(uvc_device_info info) const
        {
            return nullptr;
        }

        vector<uvc_device_info> record_backend::query_uvc_devices() const
        {
            auto devices = _source->query_uvc_devices();
            _rec->save_uvc_device_info_list(devices);
            return devices;
        }

        shared_ptr<usb_device> record_backend::create_usb_device(usb_device_info info) const
        {
            auto dev = _source->create_usb_device(info);

            auto id = _entity_count.fetch_add(1);
            auto&& c = _rec->add_call(0, call_type::create_usb_device);
            c.param1 = id;

            return make_shared<record_usb_device>(_rec, dev, id);
        }

        vector<usb_device_info> record_backend::query_usb_devices() const
        {
            auto devices = _source->query_usb_devices();
            _rec->save_usb_device_info_list(devices);
            return devices;
        }

        record_backend::record_backend(shared_ptr<backend> source)
            : _source(source), _rec(make_shared<recording>())
        {
        }

        shared_ptr<uvc_device> playback_backend::create_uvc_device(uvc_device_info info) const
        {
            return nullptr;
        }

        vector<uvc_device_info> playback_backend::query_uvc_devices() const
        {
            return _rec->load_uvc_device_info_list();
        }

        shared_ptr<usb_device> playback_backend::create_usb_device(usb_device_info info) const
        {
            auto&& c = _rec->find_call(call_type::create_usb_device, 0);

            return make_shared<playback_usb_device>(_rec, c.param1);
        }

        vector<usb_device_info> playback_backend::query_usb_devices() const
        {
            return _rec->load_usb_device_info_list();
        }

        playback_backend::playback_backend(const char* filename)
            : _rec(recording::load(filename))
        {
        }

        void record_backend::save_to_file(const char* filename) const
        {
            _rec->save(filename);
        }

        std::vector<uint8_t> playback_usb_device::send_receive(const std::vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            auto&& c = _rec->find_call(call_type::send_command, _entity_id);
            if (c.param3 != timeout_ms || c.param4 != require_response || _rec->load_blob(c.param1) != data)
                throw runtime_error("Recording history mismatch!");
            return _rec->load_blob(c.param2);
        }
    }
}

