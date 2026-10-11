// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#ifdef ENABLE_AI_ASSISTANT
#include "../http/curl-wrapper.h"
#include <thread>
#endif

#include "assistant-image-cache.h"
#include "assistant-image-decoder.h"
#include "rendering.h"

namespace rs2
{
    namespace assistant
    {
#ifndef ENABLE_AI_ASSISTANT
        // Dummy implementation - never starts a fetch or calls invoke (which would deadlock the UI
        // thread if invoked synchronously from it); the entry just stays permanently "failed".
        cached_image* assistant_image_cache::get_or_load(const std::string& url, invoke_fn)
        {
            auto& entry = _entries[url];
            entry.state = image_load_state::failed;
            return &entry;
        }
        void assistant_image_cache::fetch(const std::string&, invoke_fn) {}

#else

        static const long IMAGE_FETCH_TIMEOUT_SEC = 20L; // overall cap for fetching a markdown image/gif
        static const size_t MAX_IMAGE_BYTES = 20 * 1024 * 1024; // guards a huge/misbehaving image URL

        namespace
        {
            // Transfer + decode only, no cache/UI knowledge - an empty result (frames.empty())
            // covers every failure (transfer error, size cap exceeded, decode failure) uniformly.
            assistant_detail::decoded_image fetch_and_decode(const std::string& url)
            {
                std::vector<uint8_t> bytes;
                http::curl_wrapper curl;
                bool ok = curl.get(url, [&bytes](const char* data, size_t len) {
                    if (bytes.size() + len > MAX_IMAGE_BYTES)
                        return false; // abort the transfer: response grew past the size cap
                    bytes.insert(bytes.end(), data, data + len);
                    return true;
                }, {}, false, IMAGE_FETCH_TIMEOUT_SEC);

                return ok ? assistant_detail::decode_image_bytes(bytes.data(), bytes.size())
                          : assistant_detail::decoded_image();
            }
        }

        // On invoke() timing out (UI thread didn't drain in time), one best-effort retry to mark
        // the entry failed instead of leaving it "loading" forever with no retry/fallback.
        void assistant_image_cache::apply_fetch_result(std::shared_ptr<assistant_image_cache> me,
            const std::string& url, invoke_fn invoke, assistant_detail::decoded_image decoded)
        {
            try
            {
                invoke([me, url, decoded = std::move(decoded)]() {
                    auto found = me->_entries.find(url);
                    if (found == me->_entries.end())
                        return; // shouldn't happen; defensive
                    auto& entry = found->second;

                    if (decoded.frames.empty())
                    {
                        entry.state = image_load_state::failed;
                        return;
                    }

                    entry.width = decoded.width;
                    entry.height = decoded.height;
                    for (auto&& frame : decoded.frames)
                    {
                        auto tex = std::unique_ptr<texture_buffer>(new texture_buffer());
                        tex->upload_image(decoded.width, decoded.height, (void*)frame.rgba.data());
                        entry.frame_textures.push_back(std::move(tex));
                        entry.frame_delays_ms.push_back(frame.delay_ms);
                    }
                    entry.state = image_load_state::loaded;
                });
            }
            catch (const std::exception&)
            {
                safe_invoke(invoke, [me, url]() {
                    auto found = me->_entries.find(url);
                    if (found != me->_entries.end() && found->second.state == image_load_state::loading)
                        found->second.state = image_load_state::failed;
                });
            }
        }

        cached_image* assistant_image_cache::get_or_load(const std::string& url, invoke_fn invoke)
        {
            auto it = _entries.find(url);
            if (it != _entries.end())
                return &it->second;

            auto& entry = _entries[url]; // default-constructed: state == loading
            fetch(url, invoke);
            return &entry;
        }

        void assistant_image_cache::fetch(const std::string& url, invoke_fn invoke)
        {
            auto me = shared_from_this();
            std::thread t([me, url, invoke]() {
                try
                {
                    auto decoded = fetch_and_decode(url);
                    apply_fetch_result(me, url, invoke, std::move(decoded));
                }
                catch (...)
                {
                    // Entry stays "loading" forever in this edge case, same as any other fetch
                    // that never completes - not a new failure mode, just not a crash either.
                }
            });
            t.detach();
        }
#endif
    }
}
