// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "recorder.h"
#include <fstream>
#include "sql.h"
#include <algorithm>
#include "types.h"
#include <iostream>

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
const char* SECTIONS_COUNT_ALL = "SELECT COUNT(*) FROM rs_sections";
const char* SECTIONS_FIND_BY_NAME = "SELECT key FROM rs_sections WHERE name = ?";

const char* CALLS_CREATE = "CREATE TABLE rs_calls(section NUMBER, type NUMBER, timestamp NUMBER, entity_id NUMBER, txt TEXT, param1 NUMBER, param2 NUMBER, param3 NUMBER, param4 NUMBER, param5 NUMBER, param6 NUMBER, had_errors NUMBER, param7 NUMBER, param8 NUMBER, param9 NUMBER, param10 NUMBER, param11 NUMBER, param12 NUMBER)";
const char* CALLS_INSERT = "INSERT INTO rs_calls(section, type, timestamp, entity_id, txt, param1, param2, param3, param4, param5, param6, had_errors, param7, param8, param9, param10, param11, param12) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?, ?, ?, ?, ?, ?,?)";
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

namespace librealsense
{
    namespace platform
    {
        class playback_backend_exception : public backend_exception
        {
        public:
            playback_backend_exception(const std::string& msg, call_type t, int entity_id) noexcept
                : backend_exception(generate_message(msg, t, entity_id), RS2_EXCEPTION_TYPE_BACKEND)
            {}
        private:
            std::string generate_message(const std::string& msg, call_type t, int entity_id) const
            {
                std::stringstream s;
                s << msg << " call type: " << int(t) << " entity " << entity_id;
                return s.str();
            }
        };

        enum class device_type
        {
            uvc,
            usb,
            hid,
            hid_sensor,
            hid_input
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

        vector<uint8_t> compression_algorithm::encode(uint8_t* data, size_t size) const
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

        recording::recording(std::shared_ptr<time_service> ts, std::shared_ptr<playback_device_watcher> watcher)
            :_ts(ts), _watcher(watcher)
        {
        }

        void recording::invoke_device_changed_event()
        {
            call* next;
            do
            {
                backend_device_group old, curr;
                lookup_key k{ 0, call_type::device_watcher_event };
                load_device_changed_data(old, curr, k);
                _watcher->raise_callback(old, curr);
                next = pick_next_call();
            } while (next && next->type == call_type::device_watcher_event);
        }

        rs2_time_t recording::get_time()
        {
            return _curr_time;
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
                {
                    statement check_section_unique(c, SECTIONS_COUNT_BY_NAME);
                    check_section_unique.bind(1, section);
                    auto result = check_section_unique();
                    if (result[0].get_int() > 0)
                    {
                        throw runtime_error(to_string() << "Append record - can't save over existing section in file " << filename << "!");
                    }
                }

                {
                    statement max_section_id(c, SECTIONS_COUNT_ALL);
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
                        throw runtime_error(to_string() << "Append record - Could not find section " << section << " in file " << filename << "!");
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
                    insert.bind(3, RS2_API_VERSION_STR);
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
                    insert.bind(10, cl.param5);
                    insert.bind(11, cl.param6);
                    insert.bind(12, cl.had_error ? 1 : 0);
                    insert.bind(13, cl.param7);
                    insert.bind(14, cl.param8);
                    insert.bind(15, cl.param9);
                    insert.bind(16, cl.param10);
                    insert.bind(17, cl.param11);
                    insert.bind(18, cl.param12);

                    insert();
                }

                for (auto&& uvc_info : uvc_device_infos)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)device_type::uvc);
                    insert.bind(3, "");
                    insert.bind(4, uvc_info.unique_id.c_str());
                    insert.bind(5, (int)uvc_info.pid);
                    insert.bind(6, (int)uvc_info.vid);
                    insert.bind(7, (int)uvc_info.mi);
                    insert();
                }

                for (auto&& usb_info : usb_device_infos)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)device_type::usb);
                    string id(usb_info.id.begin(), usb_info.id.end());
                    insert.bind(3, id.c_str());
                    insert.bind(4, usb_info.unique_id.c_str());
                    insert.bind(5, (int)usb_info.pid);
                    insert.bind(6, (int)usb_info.vid);
                    insert.bind(7, (int)usb_info.mi);
                    insert();
                }

                for (auto&& hid_info : hid_device_infos)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)device_type::hid);
                    insert.bind(3, hid_info.id.c_str());
                    insert.bind(4, hid_info.unique_id.c_str());

                    stringstream ss_vid(hid_info.vid);
                    stringstream ss_pid(hid_info.pid);
                    uint32_t vid, pid;
                    ss_vid >> hex >> vid;
                    ss_pid >> hex >> pid;

                    insert.bind(5, (int)pid);
                    insert.bind(6, (int)vid);
                    insert.bind(7, hid_info.device_path.c_str());
                    insert();
                }

                for (auto&& hid_info : hid_sensors)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)device_type::hid_sensor);
                    insert.bind(3, hid_info.name.c_str());
                    insert.bind(4, "");
                    insert();
                }

                for (auto&& hid_info : hid_sensor_inputs)
                {
                    statement insert(c, DEVICE_INFO_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)device_type::hid_input);
                    insert.bind(3, hid_info.name.c_str());
                    insert.bind(4, "");
                    insert();
                }

                for (auto&& profile : this->stream_profiles)
                {
                    statement insert(c, PROFILES_INSERT);
                    insert.bind(1, section_id);
                    insert.bind(2, (int)profile.width);
                    insert.bind(3, (int)profile.height);
                    insert.bind(4, (int)profile.fps);
                    insert.bind(5, (int)profile.format);
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

        static bool is_heighr_or_equel_to_min_version(std::string api_version, std::string min_api_version)
        {
            const int ver_size = 3;
            int section_version[ver_size] = { -1, -1, -1 };
            int min_version[ver_size] = { -1 ,-1 , -1 };
            std::sscanf(api_version.c_str(), "%d.%d.%d", &section_version[0], &section_version[1], &section_version[2]);
            std::sscanf(min_api_version.c_str(), "%d.%d.%d", &min_version[0], &min_version[1], &min_version[2]);
            for (int i = 0; i < ver_size; i++)
            {
                if(min_version[i] < 0)
                    throw runtime_error(to_string() << "Minimum provided version is in wrong format, expexted format: 0.0.0, actual string: " << min_api_version);
                if (section_version[i] == min_version[i]) continue;
                if (section_version[i] > min_version[i]) continue;
                if (section_version[i] < min_version[i]) return false;
            }
            return true;
        }

        shared_ptr<recording> recording::load(const char* filename, const char* section, std::shared_ptr<playback_device_watcher> watcher, std::string min_api_version)
        {
            if (!file_exists(filename))
            {
                throw runtime_error("Recording file not found!");
            }

            auto result = make_shared<recording>(nullptr, watcher);

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
                if(is_heighr_or_equel_to_min_version(api_version, min_api_version) == false)
                    throw runtime_error(to_string() << "File version is lower than the minimum required version that was defind by the test, file version: " <<
                        api_version << " min version: " << min_api_version);
                LOG_WARNING("Loaded recording from API version " << api_version);
            }

            statement select_calls(c, CALLS_SELECT_ALL);
            select_calls.bind(1, section_id);

            result->calls.push_back(call());

            for (auto&& row : select_calls)
            {
                call cl;
                cl.type = static_cast<call_type>(row[1].get_int());
                cl.timestamp = row[2].get_double();
                cl.entity_id = row[3].get_int();
                cl.inline_string = row[4].get_string();
                cl.param1 = row[5].get_int();
                cl.param2 = row[6].get_int();
                cl.param3 = row[7].get_int();
                cl.param4 = row[8].get_int();
                cl.param5 = row[9].get_int();
                cl.param6 = row[10].get_int();
                cl.had_error = row[11].get_int() > 0;
                cl.param7 = row[12].get_int();
                cl.param8 = row[13].get_int();
                cl.param9 = row[14].get_int();
                cl.param10 = row[15].get_int();
                cl.param11 = row[16].get_int();
                cl.param12 = row[17].get_int();


                result->calls.push_back(cl);
                result->_curr_time = cl.timestamp;
            }

            statement select_devices(c, DEVICE_INFO_SELECT_ALL);
            select_devices.bind(1, section_id);
            for (auto&& row : select_devices)
            {
                if (row[1].get_int() == (int)device_type::usb)
                {
                    usb_device_info info;
                    info.id = row[2].get_string();
                    info.unique_id = row[3].get_string();
                    info.pid = row[4].get_int();
                    info.vid = row[5].get_int();
                    info.mi = row[6].get_int();
                    result->usb_device_infos.push_back(info);
                }
                else if (row[1].get_int() == (int)device_type::uvc)
                {
                    uvc_device_info info;
                    info.unique_id = row[3].get_string();
                    info.pid = row[4].get_int();
                    info.vid = row[5].get_int();
                    info.mi = row[6].get_int();
                    result->uvc_device_infos.push_back(info);
                }
                else if (row[1].get_int() == (int)device_type::hid)
                {
                    hid_device_info info;
                    info.id = row[2].get_string();
                    info.unique_id = row[3].get_string();
                    info.pid = to_string() << row[4].get_int();
                    info.vid = to_string() << row[5].get_int();
                    info.device_path = row[6].get_string();

                    result->hid_device_infos.push_back(info);
                }
                else if (row[1].get_int() == (int)device_type::hid_sensor)
                {
                    hid_sensor info;
                    info.name = row[2].get_string();

                    result->hid_sensors.push_back(info);
                }
                else if (row[1].get_int() == (int)device_type::hid_input)
                {
                    hid_sensor_input info;
                    info.name = row[2].get_string();
                    info.index = row[4].get_int();

                    result->hid_sensor_inputs.push_back(info);
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

        int recording::save_blob(const void* ptr, size_t size)
        {
            lock_guard<recursive_mutex> lock(_mutex);
            vector<uint8_t> holder;
            holder.resize(size);
            librealsense::copy(holder.data(), ptr, size);
            auto id = static_cast<int>(blobs.size());
            blobs.push_back(holder);
            return id;
        }

        double recording::get_current_time()
        {
            return _ts->get_time();
        }

        call& recording::find_call(call_type t, int entity_id, std::function<bool(const call& c)> history_match_validation)
        {
            lock_guard<recursive_mutex> lock(_mutex);


            for (size_t i = 1; i <= calls.size(); i++)
            {
                const auto idx = (_cursors[entity_id] + i) % static_cast<int>(calls.size());
                if (calls[idx].type == t && calls[idx].entity_id == entity_id)
                {
                    if (calls[idx].had_error)
                    {
                        throw runtime_error(calls[idx].inline_string);
                    }
                    _curr_time = calls[idx].timestamp;

                    if (!history_match_validation(calls[idx]))
                    {
                        throw playback_backend_exception("Recording history mismatch!", t, entity_id);
                    }

                    _cursors[entity_id] = _cycles[entity_id] = idx;

                    auto next = pick_next_call();
                    if (next && t != call_type::device_watcher_event && next->type == call_type::device_watcher_event)
                    {
                        invoke_device_changed_event();
                    }
                    return calls[idx];
                }
            }
            throw runtime_error("The recording is missing the part you are trying to playback!");
        }

        call* recording::pick_next_call(int id)
        {
            lock_guard<recursive_mutex> lock(_mutex);
            const auto idx = (_cycles[id] + 1) % static_cast<int>(calls.size());
            return &calls[idx];
        }

        call* recording::cycle_calls(call_type t, int id)
        {

            lock_guard<recursive_mutex> lock(_mutex);
            auto&& next = pick_next_call();
            if (next && next->type == call_type::device_watcher_event)
            {
                invoke_device_changed_event();
            }

            for (size_t i = 1; i <= calls.size(); i++)
            {
                const auto idx = (_cycles[id] + i);
                 if (idx >= calls.size())
                 {
                     _cycles[id] = _cursors[id];
                     return nullptr;
                 }

                if (calls[idx].type == t && calls[idx].entity_id == id)
                {
                    _cycles[id] = idx;
                    _curr_time = calls[idx].timestamp;
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

        void record_device_watcher::start(device_changed_callback callback)
        {
            _owner->try_record([=](recording* rec1, lookup_key key1)
            {
                _source_watcher->start([=](backend_device_group old, backend_device_group curr)
                {
                    _owner->try_record([=](recording* rec1, lookup_key key1)
                    {
                        rec1->save_device_changed_data(old, curr, key1);
                        callback(old, curr);

                    }, _entity_id, call_type::device_watcher_event);
                });

            }, _entity_id, call_type::device_watcher_start);
        }

        void record_device_watcher::stop()
        {
            _owner->try_record([&](recording* rec1, lookup_key key1)
            {
                _source_watcher->stop();

            }, _entity_id, call_type::device_watcher_stop);
        }


        void record_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback, int buffers)
        {
            _owner->try_record([this, callback, profile](recording* rec, lookup_key k)
            {
                _source->probe_and_commit(profile, [this, callback](stream_profile p, frame_object f, std::function<void()> continuation)
                {
                    _owner->try_record([this, callback, p, &f, continuation](recording* rec1, lookup_key key1)
                    {
                        auto&& c = rec1->add_call(key1);
                        c.param1 = rec1->save_blob(&p, sizeof(p));

                        if (_owner->get_mode() == RS2_RECORDING_MODE_BEST_QUALITY)
                        {
                            c.param2 = rec1->save_blob(f.pixels, static_cast<int>(f.frame_size));
                            c.param4 = static_cast<int>(f.frame_size);
                            c.param3 = 1;
                        }
                        else if (_owner->get_mode() == RS2_RECORDING_MODE_BLANK_FRAMES)
                        {
                            c.param2 = -1;
                            c.param4 = static_cast<int>(f.frame_size);
                            c.param3 = 0;
                        }
                        else
                        {
                            auto compressed = _compression->encode((uint8_t*)f.pixels, f.frame_size);
                            c.param2 = rec1->save_blob(compressed.data(), static_cast<int>(compressed.size()));
                            c.param4 = static_cast<int>(compressed.size());
                            c.param3 = 2;
                        }

                        c.param5 = rec1->save_blob(f.metadata, static_cast<int>(f.metadata_size));
                        c.param6 = static_cast<int>(f.metadata_size);
                        callback(p, f, continuation);
                    }, _entity_id, call_type::uvc_frame);
                });

                vector<stream_profile> ps{ profile };
                rec->save_stream_profiles(ps, k);

            }, _entity_id, call_type::uvc_probe_commit);
        }

        void record_uvc_device::stream_on(std::function<void(const notification& n)> error_handler)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->stream_on();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_play);
        }

        void record_uvc_device::start_callbacks()
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->start_callbacks();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_start_callbacks);
        }

        void record_uvc_device::stop_callbacks()
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->stop_callbacks();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_stop_callbacks);
        }

        void record_uvc_device::close(stream_profile profile)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->close(profile);
                vector<stream_profile> ps{ profile };
                rec->save_stream_profiles(ps, k);
            }, _entity_id, call_type::uvc_close);
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

        bool record_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len)
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto sts = _source->set_xu(xu, ctrl, data, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(data, len);
                c.param3 = sts;

                return sts;
            }, _entity_id, call_type::uvc_set_xu);
        }

        bool record_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto sts = _source->get_xu(xu, ctrl, data, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(data, len);
                c.param3 = sts;

                return sts;
            }, _entity_id, call_type::uvc_get_xu);
        }

        control_range record_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_xu_range(xu, ctrl, len);

                auto&& c = rec->add_call(k);
                c.param1 = ctrl;
                c.param2 = rec->save_blob(res.def.data(), res.def.size());
                c.param3 = rec->save_blob(res.min.data(), res.min.size());
                c.param4 = rec->save_blob(res.max.data(), res.max.size());
                c.param5 = rec->save_blob(res.step.data(), res.step.size());

                return res;
            }, _entity_id, call_type::uvc_get_xu_range);
        }

        bool record_uvc_device::get_pu(rs2_option opt, int32_t& value) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto sts = _source->get_pu(opt, value);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = value;
                c.param3 = sts;

                return sts;
            }, _entity_id, call_type::uvc_get_pu);
        }

        bool record_uvc_device::set_pu(rs2_option opt, int32_t value)
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto sts = _source->set_pu(opt, value);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = value;
                c.param3 = sts;

                return sts;
            }, _entity_id, call_type::uvc_set_pu);
        }

        control_range record_uvc_device::get_pu_range(rs2_option opt) const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_pu_range(opt);

                auto&& c = rec->add_call(k);
                c.param1 = opt;
                c.param2 = rec->save_blob(res.min.data(), res.min.size());
                c.param3 = rec->save_blob(res.max.data(), res.max.size());
                c.param4 = rec->save_blob(res.step.data(), res.step.size());
                c.param5 = rec->save_blob(res.def.data(), res.def.size());

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
                _source->unlock();
                rec->add_call(k);
            }, _entity_id, call_type::uvc_unlock);
        }

        void record_hid_device::register_profiles(const std::vector<hid_profile>& hid_profiles)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->register_profiles(hid_profiles);
                auto&& c = rec->add_call(k);
                c.param1 = rec->save_blob(hid_profiles.data(), hid_profiles.size() * sizeof(hid_profile));
            }, _entity_id, call_type::hid_register_profiles);
        }

        void record_hid_device::open(const std::vector<hid_profile>& hid_profiles)
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->open(hid_profiles);
                auto&& c =rec->add_call(k);
                c.param1 = rec->save_blob(hid_profiles.data(), hid_profiles.size() * sizeof(hid_profile));
            }, _entity_id, call_type::hid_open);
        }

        void record_hid_device::close()
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->close();
                rec->add_call(k);
            }, _entity_id, call_type::hid_close);
        }

        void record_hid_device::stop_capture()
        {
            _owner->try_record([&](recording* rec, lookup_key k)
            {
                _source->stop_capture();
                rec->add_call(k);
            }, _entity_id, call_type::hid_stop_capture);
        }

        void record_hid_device::start_capture(hid_callback callback)
        {
            _owner->try_record([this, callback](recording* rec, lookup_key k)
            {
                _source->start_capture([this, callback](const sensor_data& sd)
                {
                    _owner->try_record([this, callback, &sd](recording* rec1, lookup_key key1)
                    {
                        auto&& c = rec1->add_call(key1);
                        c.param1 = rec1->save_blob(sd.fo.pixels, sd.fo.frame_size);
                        c.param2 = rec1->save_blob(sd.fo.metadata, sd.fo.metadata_size);


                        c.inline_string = sd.sensor.name;

                        callback(sd);
                    }, _entity_id, call_type::hid_frame);
                });

            }, _entity_id, call_type::hid_start_capture);
        }

        vector<hid_sensor> record_hid_device::get_sensors()
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto res = _source->get_sensors();
                rec->save_hid_sensors(res, k);
                return res;
            }, _entity_id, call_type::hid_get_sensors);
        }

        std::vector<uint8_t> record_hid_device::get_custom_report_data(const std::string& custom_sensor_name,
            const std::string& report_name,
            custom_sensor_report_field report_field)
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto result = _source->get_custom_report_data(custom_sensor_name, report_name, report_field);

                auto&& c = rec->add_call(k);
                c.param1 = rec->save_blob((void*)result.data(), result.size());
                c.param2 = rec->save_blob(custom_sensor_name.c_str(), custom_sensor_name.size() + 1);
                c.param3 = rec->save_blob(report_name.c_str(), report_name.size() + 1);
                c.param4 = report_field;

                return result;
            }, _entity_id, call_type::hid_get_custom_report_data);
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

        usb_spec record_uvc_device::get_usb_specification() const
        {
            return _owner->try_record([&](recording* rec, lookup_key k)
            {
                auto result = _source->get_usb_specification();

                auto&& c = rec->add_call(k);
                c.param1 = result;

                return result;
            }, _entity_id, call_type::uvc_get_usb_specification);
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


        shared_ptr<hid_device> record_backend::create_hid_device(hid_device_info info) const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto dev = _source->create_hid_device(info);

                auto id = _entity_count.fetch_add(1);
                auto&& c = rec->add_call(k);
                c.param1 = id;

                return make_shared<record_hid_device>(dev, id, this);
            }, 0, call_type::create_hid_device);
        }

        vector<hid_device_info> record_backend::query_hid_devices() const
        {
            return try_record([&](recording* rec, lookup_key k)
            {
                auto devices = _source->query_hid_devices();
                rec->save_device_info_list(devices, k);
                return devices;
            }, 0, call_type::query_hid_devices);
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

        shared_ptr<command_transfer> record_backend::create_usb_device(usb_device_info info) const
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

        std::shared_ptr<time_service> record_backend::create_time_service() const
        {
            return _source->create_time_service();
        }

        std::shared_ptr<device_watcher> record_backend::create_device_watcher() const
        {
            return std::make_shared<record_device_watcher>(this, _source->create_device_watcher(), 0);
        }

        record_backend::record_backend(shared_ptr<backend> source,
            const char* filename, const char* section,
            rs2_recording_mode mode)
            : _source(source), _rec(std::make_shared<platform::recording>(create_time_service())), _entity_count(1),
            _filename(filename),
            _section(section), _compression(make_shared<compression_algorithm>()), _mode(mode)
        {}

        record_backend::~record_backend()
        {
            try
            {
                write_to_file();
            }
            catch (const runtime_error& e)
            {
                std::cerr << "Recording failed: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Recording failed: - unresolved error" << std::endl;
            }
        }

        shared_ptr<hid_device> playback_backend::create_hid_device(hid_device_info info) const
        {
            auto&& c = _rec->find_call(call_type::create_hid_device, 0);

            return make_shared<playback_hid_device>(_rec, c.param1);
        }

        vector<hid_device_info> playback_backend::query_hid_devices() const
        {
            return _rec->load_hid_device_info_list();
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

        shared_ptr<command_transfer> playback_backend::create_usb_device(usb_device_info info) const
        {
            auto&& c = _rec->find_call(call_type::create_usb_device, 0);

            return make_shared<playback_usb_device>(_rec, c.param1);
        }

        vector<usb_device_info> playback_backend::query_usb_devices() const
        {
            return _rec->load_usb_device_info_list();
        }

        std::shared_ptr<time_service> playback_backend::create_time_service() const
        {
            return make_shared<recording_time_service>(*_rec);
        }

        std::shared_ptr<device_watcher> playback_backend::create_device_watcher() const
        {
            return _device_watcher;
        }

        playback_backend::playback_backend(const char* filename, const char* section, std::string min_api_version)
            : _device_watcher(new playback_device_watcher(0)),
            _rec(platform::recording::load(filename, section, _device_watcher, min_api_version))
        {
            LOG_DEBUG("Starting section " << section);
        }

        playback_uvc_device::~playback_uvc_device()
        {
            assert(_alive);
            _alive = false;
            _callback_thread.join();
        }

        void record_backend::write_to_file() const
        {
            _rec->save(_filename.c_str(), _section.c_str(), false);
            //LOG(INFO) << "Finished writing " << _rec->size() << " calls...";
        }

        playback_device_watcher::playback_device_watcher(int id)
            : _entity_id(id), _alive(false), _dispatcher(10)
        {}

        playback_device_watcher::~playback_device_watcher()
        {
            stop();
        }

        void playback_device_watcher::start(device_changed_callback callback)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            stop();
            _dispatcher.start();
            _callback = callback;
            _alive = true;
        }

        void playback_device_watcher::stop()
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            if (_alive)
            {
                _alive = false;
                _dispatcher.stop();
            }
        }

        void playback_device_watcher::raise_callback(backend_device_group old, backend_device_group curr)
        {
            _dispatcher.invoke([=](dispatcher::cancellable_timer t) {
                _callback(old, curr);
            });
        }

        void playback_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback, int buffers)
        {
            lock_guard<mutex> lock(_callback_mutex);

            auto it = std::remove_if(begin(_callbacks), end(_callbacks),
                [&profile](const pair<stream_profile, frame_callback>& pair)
            {
                return pair.first == profile;
            });

            _callbacks.erase(it, end(_callbacks));
            _commitments.push_back({ profile, callback });
        }

        void playback_uvc_device::stream_on(std::function<void(const notification& n)> error_handler)
        {
            lock_guard<mutex> lock(_callback_mutex);

            auto&& c = _rec->find_call(call_type::uvc_play, _entity_id);

            for (auto&& pair : _commitments)
                _callbacks.push_back(pair);
            _commitments.clear();
        }

        void playback_uvc_device::start_callbacks()
        {
            _rec->find_call(call_type::uvc_start_callbacks, _entity_id);
        }

        void playback_uvc_device::stop_callbacks()
        {
            _rec->find_call(call_type::uvc_stop_callbacks, _entity_id);
        }

        void playback_uvc_device::close(stream_profile profile)
        {
            auto stored = _rec->load_stream_profiles(_entity_id, call_type::uvc_close);
            vector<stream_profile> input{ profile };
            if (input != stored)
                throw playback_backend_exception("Recording history mismatch!", call_type::uvc_close, _entity_id);

            lock_guard<mutex> lock(_callback_mutex);
            auto it = std::remove_if(begin(_callbacks), end(_callbacks),
                [&profile](const pair<stream_profile, frame_callback>& pair)
            {
                return pair.first == profile;
            });
            _callbacks.erase(it, end(_callbacks));
        }

        void playback_uvc_device::set_power_state(power_state state)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_power_state, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == state;
            });
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

        bool playback_uvc_device::set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_xu, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == ctrl;
            });

            auto stored_data = _rec->load_blob(c.param2);
            vector<uint8_t> in_data(data, data + len);
            if (stored_data != in_data)
            {
                throw playback_backend_exception("Recording history mismatch!", call_type::uvc_set_xu, _entity_id);
            }
            return (c.param3 != 0);
        }

        bool playback_uvc_device::get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_xu, _entity_id);
            if (c.param1 != ctrl)
                throw playback_backend_exception("Recording history mismatch!", call_type::uvc_get_xu, _entity_id);
            auto stored_data = _rec->load_blob(c.param2);
            if (stored_data.size() != len)
                throw playback_backend_exception("Recording history mismatch!", call_type::uvc_get_xu, _entity_id);
            librealsense::copy(data, stored_data.data(), len);
            return (c.param3 != 0);
        }

        control_range playback_uvc_device::get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const
        {
            control_range res;
            auto&& c = _rec->find_call(call_type::uvc_get_xu_range, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == ctrl;
            });

            res.def = _rec->load_blob(c.param2);
            res.min = _rec->load_blob(c.param3);
            res.max = _rec->load_blob(c.param4);
            res.step = _rec->load_blob(c.param5);

            return res;
        }

        bool playback_uvc_device::get_pu(rs2_option opt, int32_t& value) const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_pu, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == opt;
            });
            value = c.param2;
            return (c.param3 != 0);
        }

        bool playback_uvc_device::set_pu(rs2_option opt, int32_t value)
        {
            auto&& c = _rec->find_call(call_type::uvc_set_pu, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == opt && call_found.param2 == value;
            });
            return (c.param3 != 0);
        }

        control_range playback_uvc_device::get_pu_range(rs2_option opt) const
        {

            auto&& c = _rec->find_call(call_type::uvc_get_pu_range, _entity_id, [&](const call& call_found)
            {
                return call_found.param1 == opt;
            });

            control_range res(_rec->load_blob(c.param2), _rec->load_blob(c.param3), _rec->load_blob(c.param4), _rec->load_blob(c.param5));

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

        usb_spec playback_uvc_device::get_usb_specification() const
        {
            auto&& c = _rec->find_call(call_type::uvc_get_usb_specification, _entity_id);
            return static_cast<usb_spec>(c.param1);
        }

        playback_uvc_device::playback_uvc_device(shared_ptr<recording> rec, int id)
            : _rec(rec), _entity_id(id), _alive(true)
        {
            _callback_thread = std::thread([this]() { callback_thread(); });
        }

        void playback_hid_device::register_profiles(const std::vector<hid_profile>& hid_profiles)
        {
            auto stored = _rec->find_call(call_type::hid_register_profiles, _entity_id);
            auto stored_iios = _rec->load_blob(stored.param1);
            // TODO: Verify sensor_iio
        }

        void playback_hid_device::open(const std::vector<hid_profile>& hid_profiles)
        {
            auto stored = _rec->find_call(call_type::hid_open, _entity_id);
            auto stored_iios = _rec->load_blob(stored.param1);
            // TODO: Verify sensor_iio
        }

        void playback_hid_device::close()
        {
            _rec->find_call(call_type::hid_close, _entity_id);

            lock_guard<mutex> lock(_callback_mutex);
            if (_alive)
            {
                _alive = false;
                _callback_thread.join();
            }
        }

        void playback_hid_device::stop_capture()
        {
            _rec->find_call(call_type::hid_stop_capture, _entity_id);

            lock_guard<mutex> lock(_callback_mutex);
            _alive = false;
            _callback_thread.join();
        }

        void playback_hid_device::start_capture(hid_callback callback)
        {
            lock_guard<mutex> lock(_callback_mutex);

            _callback = callback;
            _alive = true;

            _callback_thread = std::thread([this]() { callback_thread(); });
        }

        vector<hid_sensor> playback_hid_device::get_sensors()
        {
            return _rec->load_hid_sensors2_list(_entity_id);
        }

        std::vector<uint8_t> playback_hid_device::get_custom_report_data(const std::string& custom_sensor_name,
            const std::string& report_name,
            custom_sensor_report_field report_field)
        {
            auto&& c = _rec->find_call(call_type::hid_get_custom_report_data, _entity_id, [&](const call& call_found)
            {
                return custom_sensor_name == std::string((const char*)_rec->load_blob(call_found.param2).data()) &&
                    report_name == std::string((const char*)_rec->load_blob(call_found.param3).data()) &&
                    report_field == call_found.param4;
            });

            return _rec->load_blob(c.param1);
        }

        void playback_hid_device::callback_thread()
        {
            while (_alive)
            {
                auto c_ptr = _rec->cycle_calls(call_type::hid_frame, _entity_id);
                if (c_ptr)
                {
                    auto sd_data = _rec->load_blob(c_ptr->param1);
                    auto sensor_name = c_ptr->inline_string;

                    sensor_data sd;
                    sd.fo.pixels = (void*)sd_data.data();
                    sd.fo.frame_size = sd_data.size();

                    auto metadata = _rec->load_blob(c_ptr->param2);
                    sd.fo.metadata = (void*)metadata.data();
                    sd.fo.metadata_size = static_cast<uint8_t>(metadata.size());

                    sd.sensor.name = sensor_name;

                    _callback(sd);
                }
                this_thread::sleep_for(chrono::milliseconds(1));
            }
        }

        playback_hid_device::~playback_hid_device()
        {

        }

        playback_hid_device::playback_hid_device(shared_ptr<recording> rec, int id)
            : _rec(rec), _entity_id(id),
            _alive(false)
        {

        }

        vector<uint8_t> playback_usb_device::send_receive(const vector<uint8_t>& data, int timeout_ms, bool require_response)
        {
            auto&& c = _rec->find_call(call_type::send_command, _entity_id, [&](const call& call_found)
            {
                return call_found.param3 == timeout_ms && (call_found.param4 > 0) == require_response && _rec->load_blob(call_found.param1) == data;
            });

            return _rec->load_blob(c.param2);
        }



        stream_profile playback_uvc_device::get_profile(call* frame) const
        {
            auto profile_blob = _rec->load_blob(frame->param1);

            stream_profile p;
            librealsense::copy(&p, profile_blob.data(), sizeof(p));

            return p;
        }


        void playback_uvc_device::callback_thread()
        {
            int next_timeout_ms = 0;
            double prev_frame_ts = 0;

            while (_alive)
            {
                auto c_ptr = _rec->pick_next_call(_entity_id);

                if (c_ptr && c_ptr->type == call_type::uvc_frame)
                {
                    lock_guard<mutex> lock(_callback_mutex);
                    for (auto&& pair : _callbacks)
                    {
                        if(get_profile(c_ptr) == pair.first)
                        {
                            auto c_ptr = _rec->cycle_calls(call_type::uvc_frame, _entity_id);

                            if (c_ptr)
                            {
                                auto p = get_profile(c_ptr);
                                if(p == pair.first)
                                {
                                    vector<uint8_t> frame_blob;
                                    vector<uint8_t> metadata_blob;

                                    if (prev_frame_ts > 0 &&
                                        c_ptr->timestamp > prev_frame_ts &&
                                        c_ptr->timestamp - prev_frame_ts <= 300)
                                    {
                                        prev_frame_ts = c_ptr->timestamp - prev_frame_ts;
                                    }

                                    prev_frame_ts = c_ptr->timestamp;

                                    if (c_ptr->param3 == 0) // frame was not saved
                                    {
                                        frame_blob = vector<uint8_t>(c_ptr->param4, 0);
                                    }
                                    else if (c_ptr->param3 == 1)// frame was saved
                                    {
                                        frame_blob = _rec->load_blob(c_ptr->param2);
                                    }
                                    else
                                    {
                                        frame_blob = _compression.decode(_rec->load_blob(c_ptr->param2));
                                    }

                                    metadata_blob = _rec->load_blob(c_ptr->param5);
                                    frame_object fo{ frame_blob.size(),
                                                static_cast<uint8_t>(metadata_blob.size()), // Metadata is limited to 0xff bytes by design
                                                frame_blob.data(),metadata_blob.data() };


                                    pair.second(p, fo, []() {});

                                    break;
                                }
                            }
                            else
                            {
                                LOG_WARNING("Could not Cycle frames!");
                            }

                        }

                    }
                }
                else
                {
                    _rec->cycle_calls(call_type::uvc_frame, _entity_id);
                }
                // work around - Let the other threads of playback uvc devices pull their frames
                this_thread::sleep_for(std::chrono::milliseconds(next_timeout_ms));
            }
        }

    }
}

