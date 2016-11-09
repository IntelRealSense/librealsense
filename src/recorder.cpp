// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "recorder.h"
#include <fstream>
#include "sql.h"
#include <algorithm>
#include "types.h"

using namespace std;
using namespace sql;

const char* CONFIG_TABLE = "rs_config";
const char* CONFIG_QUERY = "SELECT value FROM rs_config WHERE key = ?";
const char* CONFIG_CREATE = "CREATE TABLE rs_config(key TEXT PRIMARY KEY, value TEXT)";
const char* CONFIG_INSERT = "INSERT OR REPLACE INTO rs_config(key, value) VALUES(?, ?)";
const char* API_VERSION_KEY = "api_version";

const char* CALLS_CREATE = "CREATE TABLE rs_calls(type NUMBER, timestamp NUMBER, entity_id NUMBER, txt TEXT, param1 NUMBER, param2 NUMBER, param3 NUMBER, param4 NUMBER)";
const char* CALLS_INSERT = "INSERT INTO rs_calls(type, timestamp, entity_id, txt, param1, param2, param3, param4) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
const char* CALLS_SELECT_ALL = "SELECT * FROM rs_calls";

const char* DEVICE_INFO_CREATE = "CREATE TABLE rs_device_info(type NUMBER, id TEXT, uid TEXT, pid NUMBER, vid NUMBER, mi NUMBER)";
const char* DEVICE_INFO_SELECT_ALL = "SELECT * FROM rs_device_info";
const char* DEVICE_INFO_INSERT = "INSERT INTO rs_device_info(type, id, uid, pid, vid, mi) VALUES(?, ?, ?, ?, ?, ?)";

const char* BLOBS_CREATE = "CREATE TABLE rs_blobs(data BLOB)";
const char* BLOBS_INSERT = "INSERT INTO rs_blobs(data) VALUES(?)";
const char* BLOBS_SELECT_ALL = "SELECT * FROM rs_blobs";

const char* PROFILES_CREATE = "CREATE TABLE rs_profile(width NUMBER, height NUMBER, fps NUMBER, fourcc NUMBER)";
const char* PROFILES_INSERT = "INSERT INTO rs_profile(width, height, fps, fourcc) VALUES(?,?,?,?)";
const char* PROFILES_SELECT_ALL = "SELECT * FROM rs_profile";

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

        int compression_algorithm::dist(uint32_t x, uint32_t y) const
        {
            union {
                uint32_t block;
                uint8_t bytes[4];
            } xu;
            union {
                uint32_t block;
                uint8_t bytes[4];
            } yu;
            xu.block = x;
            yu.block = y;
            auto diff_sum = 0;
            for (auto i = 0; i < 4; i++)
            {
                auto dist = abs(xu.bytes[i] - yu.bytes[i]);
                diff_sum += dist * dist;
            }
            return diff_sum;

            //short dist = 0;
            //char val = x^y;
            //while (val)
            //{
            //    ++dist;
            //    val &= val - 1;
            //}
            //return dist;
        }

        vector<uint8_t> compression_algorithm::decode(const vector<uint8_t>& input) const
        {
            vector<uint8_t> results;
            for (auto i = 0; i < input.size() - 5; i += 5)
            {
                union {
                    uint32_t block;
                    uint8_t bytes[4];
                } curr_block;

                curr_block.block = *reinterpret_cast<const uint32_t*>(input.data() + i);
                auto len = input[i + 4];
                for (auto j = 0; j < len * 4; j++)
                {
                    results.push_back(curr_block.bytes[j % 4]);
                }
            }
            return results;
        }

        vector<uint8_t> compression_algorithm::encode(const vector<uint8_t>& input) const
        {
            vector<uint8_t> results;
            union {
                uint32_t block;
                uint8_t bytes[4];
            } curr_block;
            curr_block.block = *reinterpret_cast<const uint32_t*>(input.data());
            uint8_t length = 0;
            for (auto i = 0; i < input.size(); i += 4)
            {
                auto block = *reinterpret_cast<const uint32_t*>(input.data() + i);
                if (dist(block, curr_block.block) < min_dist && length < max_length)
                {
                    length++;
                }
                else
                {
                    for (auto j = 0; j < 4; j++)
                        results.push_back(curr_block.bytes[j]);
                    results.push_back(length);
                    curr_block.block = block;
                    length = 1;
                }
            }
            if (length)
            {
                for (auto j = 0; j < 4; j++)
                    results.push_back(curr_block.bytes[j]);
                results.push_back(length);
            }
            return results;
        }

        recording::recording()
        {
            start_time = chrono::high_resolution_clock::now();
        }

        void recording::save(const char* filename, bool append) const
        {
            if (!append && file_exists(filename))
                throw runtime_error("Can't save over existing recording!");

            connection c(filename);
            LOG_WARNING("Saving recording to file, don't close the application");

            if (!append)
            {
                c.execute(CONFIG_CREATE);
                c.execute(CALLS_CREATE);
                c.execute(DEVICE_INFO_CREATE);
                c.execute(BLOBS_CREATE);
                c.execute(PROFILES_CREATE);

                {
                    statement insert(c, CONFIG_INSERT);
                    insert.bind(1, API_VERSION_KEY);
                    insert.bind(2, RS_API_VERSION_STR);
                    insert();
                }
            }

            for (auto&& cl : calls)
            {
                statement insert(c, CALLS_INSERT);
                insert.bind(1, static_cast<int>(cl.type));
                insert.bind(2, cl.timestamp);
                insert.bind(3, cl.entity_id);
                insert.bind(4, cl.inline_string.c_str());
                insert.bind(5, cl.param1);
                insert.bind(6, cl.param2);
                insert.bind(7, cl.param3);
                insert.bind(8, cl.param4);
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

            for (auto&& profile : stream_profiles)
            {
                statement insert(c, PROFILES_INSERT);
                insert.bind(1, profile.width);
                insert.bind(2, profile.height);
                insert.bind(3, profile.fps);
                insert.bind(4, profile.format);
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
                cl.inline_string = row[3].get_string();
                cl.param1 = row[4].get_int();
                cl.param2 = row[5].get_int();
                cl.param3 = row[6].get_int();
                cl.param4 = row[7].get_int();
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

            statement select_profiles(c, PROFILES_SELECT_ALL);
            for (auto&& row : select_profiles)
            {
                stream_profile p;
                p.width = row[0].get_int();
                p.height = row[1].get_int();
                p.fps = row[2].get_int();
                p.format = row[3].get_int();
                result->stream_profiles.push_back(p);
            }

            statement select_blobs(c, BLOBS_SELECT_ALL);
            for (auto&& row : select_blobs)
            {
                result->blobs.push_back(row[0].get_blob());
            }

            return result;
        }

        int recording::save_blob(const void* ptr, unsigned size)
        {
            lock_guard<mutex> lock(_mutex);
            vector<uint8_t> holder;
            holder.resize(size);
            memcpy(holder.data(), ptr, size);
            auto id = static_cast<int>(blobs.size());
            blobs.push_back(holder);
            return id + blobs_baseline;
        }

        int recording::get_timestamp() const
        {
            using namespace chrono;
            auto diff = high_resolution_clock::now() - start_time;
            return static_cast<int>(duration_cast<milliseconds>(diff).count());
        }

        call& recording::find_call(call_type t, int entity_id)
        {
            lock_guard<mutex> lock(_mutex);
            for (auto i = 1; i <= calls.size(); i++)
            {
                const auto idx = (_cursors[entity_id] + i) % static_cast<int>(calls.size());
                if (calls[idx].type == t && calls[idx].entity_id == entity_id)
                {
                    _cursors[entity_id] = _cycles[entity_id] = idx;
                    return calls[idx];
                }
            }
            throw runtime_error("The recording is missing the part you are trying to playback!");
        }

        call* recording::cycle_calls(call_type t, int id)
        {
            lock_guard<mutex> lock(_mutex);
            for (auto i = 1; i <= calls.size(); i++)
            {
                const auto idx = (_cycles[id] + i) % static_cast<int>(calls.size());
                if (calls[idx].type == t && calls[idx].entity_id == id)
                {
                    _cycles[id] = idx;
                    return &calls[idx];
                }
                if (calls[idx].type != t && calls[idx].entity_id == id)
                {
                    _cycles[id] = _cursors[id];
                    return nullptr;
                }
            }
            return nullptr;
        }

        void record_uvc_device::play(stream_profile profile, frame_callback callback)
        {
            _source->play(profile, [this, callback](stream_profile p, frame_object f)
            {
                _owner->try_record([this, callback, p, &f](recording* rec, lookup_key k)
                {
                    auto&& c = rec->add_call(k);
                    c.param1 = rec->save_blob(&p, sizeof(p));
                    vector<uint8_t> frame((uint8_t*)f.pixels,
                        (uint8_t*)f.pixels + f.size);
                    auto compressed = _compression->encode(frame);
                    c.param2 = rec->save_blob(compressed.data(), static_cast<int>(compressed.size()));
                    c.param4 = static_cast<int>(compressed.size());
                    c.param3 = _compression->save_frames ? 0 : 1;
                    //auto dec = _compression->decode(compressed);
                    _compression->effect = (100.0f * compressed.size()) / frame.size();
                    callback(p, f);
                }, _entity_id, call_type::uvc_frame);
            });
            vector<stream_profile> ps{ profile };
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                rec->save_stream_profiles(ps, k);
            }, _entity_id, call_type::uvc_play);
        }

        void record_uvc_device::stop(stream_profile profile)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->stop(profile);
                vector<stream_profile> ps{ profile };
                rec->save_stream_profiles(ps, k);
            }, _entity_id, call_type::uvc_stop);
        }

        void record_uvc_device::set_power_state(power_state state)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->set_power_state(state);

                auto&& c = rec->add_call(k);
                c.param1 = state;
            }, _entity_id, call_type::uvc_set_power_state);
        }

        power_state record_uvc_device::get_power_state() const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_power_state();

                auto&& c = rec->add_call(k);
                c.param1 = res;

                return res;
            }, _entity_id, call_type::uvc_get_power_state);
        }

        void record_uvc_device::init_xu(const extension_unit& xu)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->init_xu(xu);
                rec->add_call(k);
            }, _entity_id, call_type::uvc_init_xu);
        }

        void record_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->set_xu(xu, ctrl, data, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(data, len);
            }, _entity_id, call_type::uvc_set_xu);
        }

        void record_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->get_xu(xu, ctrl, data, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(data, len);
            }, _entity_id, call_type::uvc_get_xu);
        }

        control_range record_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_xu_range(xu, ctrl, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(&res, sizeof(res));

                return res;
            }, _entity_id, call_type::uvc_get_xu_range);
        }

        int record_uvc_device::get_pu(rs_option opt) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_pu(opt);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = res;

                return res;
            }, _entity_id, call_type::uvc_get_pu);
        }

        void record_uvc_device::set_pu(rs_option opt, int value)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->set_pu(opt, value);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = value;
            }, _entity_id, call_type::uvc_set_pu);
        }

        control_range record_uvc_device::get_pu_range(rs_option opt) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_pu_range(opt);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = rec->save_blob(&res, sizeof(res));

                return res;
            }, _entity_id, call_type::uvc_get_pu_range);
        }

        vector<stream_profile> record_uvc_device::get_profiles() const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto devices = _source->get_profiles();
                rec->save_stream_profiles(devices, k);
                return devices;
            }, _entity_id, call_type::uvc_stream_profiles);
        }

        void record_uvc_device::lock() const
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->lock();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_lock);
        }

        void record_uvc_device::unlock() const
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->lock();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_unlock);
        }

        string record_uvc_device::get_device_location() const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto result = _source->get_device_location();

                auto&& c = rec->add_call(k);
                c.inline_string = result;

                return result;
            }, _entity_id, call_type::uvc_get_location);
        }

        vector<uint8_t> record_usb_device::send_receive(const vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto result = _source->send_receive(data, timeout_ms, require_response);

                auto&& c = rec->add_call(k);
                c.param1 = rec->save_blob((void*)data.data(), static_cast<int>(data.size()));
                c.param2 = rec->save_blob((void*)result.data(), static_cast<int>(result.size()));
                c.param3 = timeout_ms;
                c.param4 = require_response;

                return result;
            }, _entity_id, call_type::send_command);
        }

        shared_ptr<uvc_device> record_backend::create_uvc_device(uvc_device_info info) const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto dev = _source->create_uvc_device(info);

                auto id = _entity_count.fetch_add(1);
                auto&& c = rec->add_call(k);
                c.param1 = id;

                return make_shared<record_uvc_device>(dev, _compression, id, this);
            }, 0, call_type::create_uvc_device);
        }

        vector<uvc_device_info> record_backend::query_uvc_devices() const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto devices = _source->query_uvc_devices();
                rec->save_device_info_list(devices, k);
                return devices;
            }, 0, call_type::query_uvc_devices);
        }

        shared_ptr<usb_device> record_backend::create_usb_device(usb_device_info info) const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto dev = _source->create_usb_device(info);

                auto id = _entity_count.fetch_add(1);
                auto&& c = rec->add_call(k);
                c.param1 = id;

                return make_shared<record_usb_device>(dev, id, this);
            }, 0, call_type::create_usb_device);
        }

        vector<usb_device_info> record_backend::query_usb_devices() const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto devices = _source->query_usb_devices();
                rec->save_device_info_list(devices, k);
                return devices;
            }, 0, call_type::query_usb_devices);
        }

        record_backend::record_backend(shared_ptr<backend> source, const char* filename)
            : _source(source), _rec(make_shared<recording>()), _entity_count(1),
            _compression(make_shared<compression_algorithm>()), _filename(filename),
            _alive(true), _write_to_file([this]() { write_to_file(); })
        {
        }

        record_backend::~record_backend()
        {
            _alive = false;
            _write_to_file.join();
        }

        shared_ptr<uvc_device> playback_backend::create_uvc_device(uvc_device_info info) const
        {
            auto&& c = _rec->find_call(call_type::create_uvc_device, 0);

            return make_shared<playback_uvc_device>(_rec, c.param1);
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

        playback_uvc_device::~playback_uvc_device()
        {
            _alive = false;
            _callback_thread.join();
        }

        void record_backend::write_to_file()
        {
            bool append = false;

            while (_alive)
            {
                std::shared_ptr<recording> current;
                {
                    std::lock_guard<std::mutex> lock(_rec_mutex);
                    current = _rec;
                    _rec = std::make_shared<recording>();
                    _rec->set_baselines(*current);
                }
                if (current->size() > 0)
                {
                    current->save(_filename.c_str(), append);
                    append = true;
                    LOG(INFO) << "Finished writing " << current->size() << " calls...";
                }
            }

            if (_rec->size() > 0)
            {
                _rec->save(_filename.c_str(), append);
                LOG(INFO) << "Finished writing " << _rec->size() << " calls...";
            }
        }

        void playback_uvc_device::play(stream_profile profile, frame_callback callback)
        {
            lock_guard<mutex> lock(_callback_mutex);
            auto stored = _rec->load_stream_profiles(_entity_id, call_type::uvc_play);
            vector<stream_profile> input{ profile };
            if (input != stored)
                throw runtime_error("Recording history mismatch!");

            auto it = std::remove_if(begin(_callbacks), end(_callbacks),
                [&profile](const pair<stream_profile, frame_callback>& pair)
            {
                return pair.first == profile;
            });
            _callbacks.erase(it, end(_callbacks));

            _callbacks.push_back({ profile, callback });
        }

        void playback_uvc_device::stop(stream_profile profile)
        {
            lock_guard<mutex> lock(_callback_mutex);
            auto stored = _rec->load_stream_profiles(_entity_id, call_type::uvc_stop);
            vector<stream_profile> input{ profile };
            if (input != stored)
                throw runtime_error("Recording history mismatch!");

            auto it = std::remove_if(begin(_callbacks), end(_callbacks),
                [&profile](const pair<stream_profile, frame_callback>& pair)
            {
                return pair.first == profile;
            });
            _callbacks.erase(it, end(_callbacks));
        }

        void playback_uvc_device::set_power_state(power_state state)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_power_state, _entity_id);
            if (c.param1 != state)
                throw runtime_error("Recording history mismatch!");
        }

        power_state playback_uvc_device::get_power_state() const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_power_state, _entity_id);
            return static_cast<power_state>(c.param1);
        }

        void playback_uvc_device::init_xu(const extension_unit& xu)
        {
            _rec->find_call(call_type::uvc_init_xu, _entity_id);
        }

        void playback_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_xu, _entity_id);
            if (c.param1 != ctrl)
                throw runtime_error("Recording history mismatch!");
            auto stored_data = _rec->load_blob(c.param2);
            vector<uint8_t> in_data(data, data + len);
            if (stored_data != in_data)
                throw runtime_error("Recording history mismatch!");
        }

        void playback_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_xu, _entity_id);
            if (c.param1 != ctrl)
                throw runtime_error("Recording history mismatch!");
            auto stored_data = _rec->load_blob(c.param2);
            if (stored_data.size() != len)
                throw runtime_error("Recording history mismatch!");
            memcpy(data, stored_data.data(), len);
        }

        control_range playback_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const
        {
            control_range res;
            auto&& c = _rec->find_call(call_type::uvc_get_xu_range, _entity_id);

            if (c.param1 != ctrl)
                throw runtime_error("Recording history mismatch!");

            auto range = _rec->load_blob(c.param2);
            memcpy(&res, range.data(), range.size());
            return res;
        }

        int playback_uvc_device::get_pu(rs_option opt) const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_pu, _entity_id);
            if (c.param1 != opt)
                throw runtime_error("Recording history mismatch!");
            return c.param2;
        }

        void playback_uvc_device::set_pu(rs_option opt, int value)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_pu, _entity_id);
            if (c.param1 != opt && c.param2 != value)
                throw runtime_error("Recording history mismatch!");
        }

        control_range playback_uvc_device::get_pu_range(rs_option opt) const
        {
            control_range res;
            auto&& c = _rec->find_call(call_type::uvc_get_pu_range, _entity_id);
            if (c.param1 != opt)
                throw runtime_error("Recording history mismatch!");
            auto range = _rec->load_blob(c.param2);
            memcpy(&res, range.data(), range.size());
            return res;
        }

        vector<stream_profile> playback_uvc_device::get_profiles() const
        {
            return _rec->load_stream_profiles(_entity_id, call_type::uvc_stream_profiles);
        }

        void playback_uvc_device::lock() const
        {
            _rec->find_call(call_type::uvc_lock, _entity_id);
        }

        void playback_uvc_device::unlock() const
        {
            _rec->find_call(call_type::uvc_unlock, _entity_id);
        }

        string playback_uvc_device::get_device_location() const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_location, _entity_id);
            return c.inline_string;
        }

        playback_uvc_device::playback_uvc_device(shared_ptr<recording> rec, int id)
            : _rec(rec), _entity_id(id),
            _callback_thread([this]() { callback_thread(); }),
            _alive(true)
        {

        }

        vector<uint8_t> playback_usb_device::send_receive(const vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            auto&& c = _rec->find_call(call_type::send_command, _entity_id);
            if (c.param3 != timeout_ms || (c.param4 > 0) != require_response || _rec->load_blob(c.param1) != data)
                throw runtime_error("Recording history mismatch!");
            return _rec->load_blob(c.param2);
        }

        void playback_uvc_device::callback_thread()
        {
            while (_alive)
            {
                auto c_ptr = _rec->cycle_calls(call_type::uvc_frame, _entity_id);
                if (c_ptr)
                {
                    auto profile_blob = _rec->load_blob(c_ptr->param1);
                    stream_profile p;
                    memcpy(&p, profile_blob.data(), sizeof(p));
                    lock_guard<mutex> lock(_callback_mutex);
                    for (auto&& pair : _callbacks)
                    {
                        if (p == pair.first)
                        {
                            vector<uint8_t> frame_blob;
                            if (c_ptr->param3 == 1) // frame was not saved
                            {
                                frame_blob = vector<uint8_t>(c_ptr->param4, 0);
                            }
                            else // frame was saved
                            {
                                frame_blob = _compression.decode(_rec->load_blob(c_ptr->param2));
                            }
                            frame_object fo{ static_cast<int>(frame_blob.size()), frame_blob.data() };
                            pair.second(p, fo);
                            break;
                        }
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(50));
            }
        }
    }
}

