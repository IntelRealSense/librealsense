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
const char* CONFIG_QUERY = "SELECT value FROM rs_config WHERE section = ? AND key = ?";
const char* CONFIG_CREATE = "CREATE TABLE rs_config(section NUMBER, key TEXT, value TEXT)";
const char* CONFIG_INSERT = "INSERT OR REPLACE INTO rs_config(section, key, value) VALUES(?, ?, ?)";
const char* API_VERSION_KEY = "api_version";
const char* CREATED_AT_KEY = "created_at";

const char* SECTIONS_TABLE = "rs_sections";
const char* SECTIONS_SELECT_MAX_ID = "SELECT max(key) from rs_sections";
const char* SECTIONS_CREATE = "CREATE TABLE rs_sections(key NUMBER PRIMARY KEY, name TEXT)";
const char* SECTIONS_INSERT = "INSERT INTO rs_sections(key, name) VALUES (?, ?)";
const char* SECTIONS_COUNT_BY_NAME = "SELECT COUNT(*) FROM rs_sections WHERE name = ?";
const char* SECTIONS_FIND_BY_NAME = "SELECT key FROM rs_sections WHERE name = ?";

const char* CALLS_CREATE = "CREATE TABLE rs_calls(section NUMBER, type NUMBER, timestamp NUMBER, entity_id NUMBER, txt TEXT, param1 NUMBER, param2 NUMBER, param3 NUMBER, param4 NUMBER, had_errors NUMBER)";
const char* CALLS_INSERT = "INSERT INTO rs_calls(section, type, timestamp, entity_id, txt, param1, param2, param3, param4, had_errors) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
const char* CALLS_SELECT_ALL = "SELECT * FROM rs_calls WHERE section = ?";

const char* DEVICE_INFO_CREATE = "CREATE TABLE rs_device_info(section NUMBER, type NUMBER, id TEXT, uid TEXT, pid NUMBER, vid NUMBER, mi NUMBER)";
const char* DEVICE_INFO_SELECT_ALL = "SELECT * FROM rs_device_info WHERE section = ?";
const char* DEVICE_INFO_INSERT = "INSERT INTO rs_device_info(section, type, id, uid, pid, vid, mi) VALUES(?, ?, ?, ?, ?, ?, ?)";

const char* BLOBS_CREATE = "CREATE TABLE rs_blobs(section NUMBER, data BLOB)";
const char* BLOBS_INSERT = "INSERT INTO rs_blobs(section, data) VALUES(?, ?)";
const char* BLOBS_SELECT_ALL = "SELECT * FROM rs_blobs WHERE section = ?";

const char* PROFILES_CREATE = "CREATE TABLE rs_profile(section NUMBER, width NUMBER, height NUMBER, fps NUMBER, fourcc NUMBER)";
const char* PROFILES_INSERT = "INSERT INTO rs_profile(section, width, height, fps, fourcc) VALUES(?, ? ,? ,? ,?)";
const char* PROFILES_SELECT_ALL = "SELECT * FROM rs_profile WHERE section = ?";

namespace rsimpl
{
    namespace uvc
    {
        enum device_type
        {
            uvc,
            usb
        };

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
        }

        vector<uint8_t> compression_algorithm::decode(const vector<uint8_t>& input) const
        {
            vector<uint8_t> results;
            for (size_t i = 0; i < input.size() - 5; i += 5)
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

        vector<uint8_t> compression_algorithm::encode(uint8_t* data, uint32_t size) const
        {
            vector<uint8_t> results;
            union {
                uint32_t block;
                uint8_t bytes[4];
            } curr_block;
            curr_block.block = *reinterpret_cast<const uint32_t*>(data);
            uint8_t length = 0;
            for (size_t i = 0; i < size; i += 4)
            {
                auto block = *reinterpret_cast<const uint32_t*>(data + i);
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

        void recording::save(const char* filename, const char* section, bool append) const
        {
            connection c(filename);
            LOG_WARNING("Saving recording to file, don't close the application");

            if (!c.table_exists(CONFIG_TABLE))
            {
                c.execute(SECTIONS_CREATE);
                c.execute(CONFIG_CREATE);
                c.execute(CALLS_CREATE);
                c.execute(DEVICE_INFO_CREATE);
                c.execute(BLOBS_CREATE);
                c.execute(PROFILES_CREATE);
            }

            auto section_id = 0;

            if (!append)
            {
                statement check_section_unique(c, SECTIONS_COUNT_BY_NAME);
                check_section_unique.bind(1, section);
                auto result = check_section_unique();
                if (result[0].get_int() > 0)
                {
                    throw runtime_error("Can't save over existing section!");
                }

                {
                    statement max_section_id(c, SECTIONS_SELECT_MAX_ID);
                    auto result = max_section_id();
                    section_id = result[0].get_int() + 1;
                }

                {
                    statement create_section(c, SECTIONS_INSERT);
                    create_section.bind(1, section_id);
                    create_section.bind(2, section);
                    create_section();
                }
            }
            else
            {
                {
                    statement check_section_exists(c, SECTIONS_COUNT_BY_NAME);
                    check_section_exists.bind(1, section);
                    auto result = check_section_exists();
                    if (result[0].get_int() == 0)
                    {
                        throw runtime_error(to_string() << "Could not find section " << section << "!");
                    }
                }
                {
                    statement find_section_id(c, SECTIONS_FIND_BY_NAME);
                    find_section_id.bind(1, section);
                    auto result = find_section_id();
                    section_id = result[0].get_int();
                }
            }

            if (!append)
            {
                {
                    statement insert(c, CONFIG_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, API_VERSION_KEY);
                    insert.bind(3, RS_API_VERSION_STR);
                    insert();
                }

                {
                    statement insert(c, CONFIG_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, CREATED_AT_KEY);
                    auto datetime = datetime_string();
                    insert.bind(3, datetime.c_str());
                    insert();
                }
            }

            c.transaction([&]()
            {
                for (auto&& cl : calls)
                {
                    statement insert(c, CALLS_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, static_cast<int>(cl.type));
                    insert.bind(3, cl.timestamp);
                    insert.bind(4, cl.entity_id);
                    insert.bind(5, cl.inline_string.c_str());
                    insert.bind(6, cl.param1);
                    insert.bind(7, cl.param2);
                    insert.bind(8, cl.param3);
                    insert.bind(9, cl.param4);
                    insert.bind(10, cl.had_error ? 1 : 0);
                    insert();
                }

                for (auto&& uvc_info : uvc_device_infos)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, device_type::uvc);
                    insert.bind(3, "");
                    insert.bind(4, uvc_info.unique_id.c_str());
                    insert.bind(5, uvc_info.pid);
                    insert.bind(6, uvc_info.vid);
                    insert.bind(7, uvc_info.mi);
                    insert();
                }

                for (auto&& usb_info : usb_device_infos)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, device_type::usb);
                    string id(usb_info.id.begin(), usb_info.id.end());
                    insert.bind(3, id.c_str());
                    insert.bind(4, usb_info.unique_id.c_str());
                    insert.bind(5, usb_info.pid);
                    insert.bind(6, usb_info.vid);
                    insert.bind(7, usb_info.mi);
                    insert();
                }

                for (auto&& profile : stream_profiles)
                {
                    statement insert(c, PROFILES_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, profile.width);
                    insert.bind(3, profile.height);
                    insert.bind(4, profile.fps);
                    insert.bind(5, profile.format);
                    insert();
                }

                for (auto&& blob : blobs)
                {
                    statement insert(c, BLOBS_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, blob);
                    insert();
                }
            });
        }

        shared_ptr<recording> recording::load(const char* filename, const char* section)
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

            int section_id = 0;
            {
                statement check_section_exists(c, SECTIONS_COUNT_BY_NAME);
                check_section_exists.bind(1, section);
                auto result = check_section_exists();
                if (result[0].get_int() == 0)
                {
                    throw runtime_error(to_string() << "Could not find section " << section << "!");
                }
            }
            {
                statement find_section_id(c, SECTIONS_FIND_BY_NAME);
                find_section_id.bind(1, section);
                auto result = find_section_id();
                section_id = result[0].get_int();
            }

            {
                statement select_api_version(c, CONFIG_QUERY);
                select_api_version.bind(1, section_id);
                select_api_version.bind(2, API_VERSION_KEY);
                auto api_version = select_api_version()[0].get_string();
                LOG_WARNING("Loaded recording from API version " << api_version);
            }

            statement select_calls(c, CALLS_SELECT_ALL);
            select_calls.bind(1, section_id);
            for (auto&& row : select_calls)
            {
                call cl;
                cl.type = static_cast<call_type>(row[1].get_int());
                cl.timestamp = row[2].get_int();
                cl.entity_id = row[3].get_int();
                cl.inline_string = row[4].get_string();
                cl.param1 = row[5].get_int();
                cl.param2 = row[6].get_int();
                cl.param3 = row[7].get_int();
                cl.param4 = row[8].get_int();
                cl.had_error = row[9].get_int() > 0;
                result->calls.push_back(cl);
            }

            statement select_devices(c, DEVICE_INFO_SELECT_ALL);
            select_devices.bind(1, section_id);
            for (auto&& row : select_devices)
            {
                if (row[1].get_int() == usb)
                {
                    usb_device_info info;
                    auto id = row[2].get_string();
                    info.id = wstring(id.begin(), id.end());
                    info.unique_id = row[3].get_string();
                    info.pid = row[4].get_int();
                    info.vid = row[5].get_int();
                    info.mi = row[6].get_int();
                    result->usb_device_infos.push_back(info);
                }
                else
                {
                    uvc_device_info info;
                    info.unique_id = row[3].get_string();
                    info.pid = row[4].get_int();
                    info.vid = row[5].get_int();
                    info.mi = row[6].get_int();
                    result->uvc_device_infos.push_back(info);
                }
            }

            statement select_profiles(c, PROFILES_SELECT_ALL);
            select_profiles.bind(1, section_id);
            for (auto&& row : select_profiles)
            {
                stream_profile p;
                p.width = row[1].get_int();
                p.height = row[2].get_int();
                p.fps = row[3].get_int();
                p.format = row[4].get_int();
                result->stream_profiles.push_back(p);
            }

            statement select_blobs(c, BLOBS_SELECT_ALL);
            select_blobs.bind(1, section_id);
            for (auto&& row : select_blobs)
            {
                result->blobs.push_back(row[1].get_blob());
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
            for (size_t i = 1; i <= calls.size(); i++)
            {
                const auto idx = (_cursors[entity_id] + i) % static_cast<int>(calls.size());
                if (calls[idx].type == t && calls[idx].entity_id == entity_id)
                {
                    _cursors[entity_id] = _cycles[entity_id] = idx;

                    if (calls[idx].had_error)
                    {
                        throw std::runtime_error(calls[idx].inline_string);
                    }

                    return calls[idx];
                }
            }
            throw runtime_error("The recording is missing the part you are trying to playback!");
        }

        call* recording::cycle_calls(call_type t, int id)
        {
            lock_guard<mutex> lock(_mutex);
            for (size_t i = 1; i <= calls.size(); i++)
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

        void record_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback)
        {
            _owner->try_record([this, callback, profile](recording* rec, lookup_key k)
            {
                _source->probe_and_commit(profile, [this, callback](stream_profile p, frame_object f)
                {
                    _owner->try_record([this, callback, p, &f](recording* rec1, lookup_key key1)
                    {
                        auto&& c = rec1->add_call(key1);
                        c.param1 = rec1->save_blob(&p, sizeof(p));

                        if (_owner->get_mode() == RS_RECORDING_MODE_BEST_QUALITY)
                        {
                            c.param2 = rec1->save_blob(f.pixels, static_cast<int>(f.size));
                            c.param4 = static_cast<int>(f.size);
                            c.param3 = 1;
                        }
                        else if (_owner->get_mode() == RS_RECORDING_MODE_BLANK_FRAMES)
                        {
                            c.param2 = -1;
                            c.param4 = static_cast<int>(f.size);
                            c.param3 = 0;
                        }
                        else
                        {
                            auto compressed = _compression->encode((uint8_t*)f.pixels, f.size);
                            c.param2 = rec1->save_blob(compressed.data(), static_cast<int>(compressed.size()));
                            c.param4 = static_cast<int>(compressed.size());
                            c.param3 = 1;
                        }

                        callback(p, f);
                    }, _entity_id, call_type::uvc_frame);
                });

                vector<stream_profile> ps{ profile };
                rec->save_stream_profiles(ps, k);

            }, _entity_id, call_type::uvc_probe_commit);
        }

        void record_uvc_device::play()
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->play();
                rec->add_call(k);
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

        record_backend::record_backend(shared_ptr<backend> source,
            const char* filename, const char* section,
            rs_recording_mode mode)
            : _source(source), _rec(make_shared<recording>()), _entity_count(1),
            _filename(filename),
            _section(section), _compression(make_shared<compression_algorithm>()), _mode(mode)
        {
            write_to_file();
        }

        record_backend::~record_backend()
        {
            write_to_file();
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

        playback_backend::playback_backend(const char* filename, const char* section)
            : _rec(recording::load(filename, section))
        {
        }

        playback_uvc_device::~playback_uvc_device()
        {
            _alive = false;
            _callback_thread.join();
        }

        void record_backend::write_to_file()
        {
            auto append = false;

            shared_ptr<recording> current;
            {
                current = _rec;
                _rec = make_shared<recording>();
                _rec->set_baselines(*current);
            }
            if (current->size() > 0)
            {
                current->save(_filename.c_str(), _section.c_str(), append);
                append = true;
                LOG(INFO) << "Finished writing " << current->size() << " calls...";
            }

            if (_rec->size() > 0)
            {
                _rec->save(_filename.c_str(), _section.c_str(), append);
                LOG(INFO) << "Finished writing " << _rec->size() << " calls...";
            }
        }

        void playback_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback)
        {
            lock_guard<mutex> lock(_callback_mutex);
            auto stored = _rec->load_stream_profiles(_entity_id, call_type::uvc_probe_commit);
            vector<stream_profile> input{ profile };
            if (input != stored)
                throw runtime_error("Recording history mismatch!");

            auto it = std::remove_if(begin(_callbacks), end(_callbacks),
                [&profile](const pair<stream_profile, frame_callback>& pair)
            {
                return pair.first == profile;
            });
            _callbacks.erase(it, end(_callbacks));
            _commitments.push_back({ profile, callback });
        }

        void playback_uvc_device::play()
        {
            lock_guard<mutex> lock(_callback_mutex);

            auto&& c = _rec->find_call(call_type::uvc_play, _entity_id);

            for (auto&& pair : _commitments)
                _callbacks.push_back(pair);
            _commitments.clear();
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
                            if (c_ptr->param3 == 0) // frame was not saved
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

