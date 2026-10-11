// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#ifdef ENABLE_AI_ASSISTANT
#include <thread>
#endif

#include "assistant-chat-client.h"
#include <rsutils/string/from.h>

namespace rs2
{
    namespace assistant
    {
#ifndef ENABLE_AI_ASSISTANT
        // Dummy implementation - the assistant was not built into this copy of the viewer.

        // Unlike the real implementation these run synchronously on the caller's (UI) thread, so
        // callbacks are called directly - routing through `invoke` would enqueue onto
        // assistant_model's dispatch_queue and deadlock waiting for the UI thread to drain itself.
        void assistant_chat_client::send(const std::string&, const std::string&,
            invoke_fn, event_callback, error_callback on_error)
        {
            on_error("The AI Assistant was not built into this copy of RealSense Viewer.");
        }
        void assistant_chat_client::cancel() {}
        void assistant_chat_client::run(std::string, std::string, invoke_fn, event_callback, error_callback) {}
        // Deliberately never calls on_result: "unhealthy" means a real check against a real URL
        // failed, which isn't true here - the status dot should stay hidden, not show red.
        void assistant_chat_client::check_health(invoke_fn, std::function<void(bool)>) {}
        void assistant_chat_client::send_reaction(const std::string&, int, invoke_fn, error_callback) {}

#else

        static const char* BASE_URL = "https://rs-chat-hnd6gchgesc9fre6.a02.azurefd.net";
        static const long REQUEST_TIMEOUT_SEC = 120L; // overall cap; SSE answers can stream for a while
        static const long ONE_SHOT_TIMEOUT_SEC = 10L; // overall cap for check_health()/send_reaction()

        namespace
        {
            std::string build_chat_request_body(const std::string& message, const std::string& conversation_id)
            {
                rsutils::json body_json;
                body_json["message"] = message;
                if (!conversation_id.empty())
                    body_json["conversationId"] = conversation_id;
                return body_json.dump();
            }
        }

        void assistant_chat_client::send(const std::string& message, const std::string& conversation_id,
            invoke_fn invoke, event_callback on_event, error_callback on_error)
        {
            if (_busy)
            {
                safe_invoke(invoke, [on_error]() { on_error("The assistant is still answering the previous message."); });
                return;
            }
            _busy = true;
            _cancel_requested = false;

            auto me = shared_from_this();
            std::thread t([me, message, conversation_id, invoke, on_event, on_error]() mutable {
                try
                {
                    me->run(message, conversation_id, invoke, on_event, on_error);
                }
                catch (const std::exception& ex)
                {
                    auto what = std::string(ex.what());
                    safe_invoke(invoke, [on_error, what]() { on_error(what); });
                }
                catch (...)
                {
                    safe_invoke(invoke, [on_error]() { on_error("Unknown error while talking to the assistant."); });
                }
                me->_busy = false;
            });
            t.detach();
        }

        void assistant_chat_client::cancel()
        {
            _cancel_requested = true;
        }

        void assistant_chat_client::run(std::string message, std::string conversation_id,
            invoke_fn invoke, event_callback on_event, error_callback on_error)
        {
            if (!_curl.valid())
            {
                safe_invoke(invoke, [on_error]() { on_error("Could not initialize the HTTP client."); });
                return;
            }

            _sse_parser.reset();
            std::string body = build_chat_request_body(message, conversation_id);
            std::string chat_url = std::string(BASE_URL) + "/api/chat/stream";

            std::function<void(const sse_event&)> forward_event = [on_event, invoke](const sse_event& event) {
                safe_invoke(invoke, [on_event, event]() { on_event(event); });
            };
            // A false return (parser overflow, or a user-requested cancel) aborts the transfer -
            // reported below via _sse_parser.overflowed()/_cancel_requested.
            std::function<bool(const char*, size_t)> on_data = [this, &forward_event](const char* data, size_t len) {
                if (_cancel_requested)
                    return false;
                return _sse_parser.feed(data, len, forward_event);
            };

            long http_status = 0;
            std::string error_detail;
            bool ok = _curl.post_stream(chat_url, body, on_data,
                "X-RS-Integration: viewer", REQUEST_TIMEOUT_SEC, &http_status, &error_detail);

            if (_cancel_requested)
                return; // silent user cancel, nothing to report

            if (_sse_parser.overflowed())
            {
                safe_invoke(invoke, [on_error]() { on_error("The assistant's response was too large to process."); });
                return;
            }

            // Maps a finished transfer's result to on_error, if it wasn't a clean 2xx (that case,
            // including a 2xx with no explicit SSE 'done'/'error' event, has nothing to report).
            if (!ok)
            {
                std::string message_text = rsutils::string::from() << "Couldn't reach the assistant: " << error_detail;
                safe_invoke(invoke, [on_error, message_text]() { on_error(message_text); });
            }
            else if (http_status == 429)
            {
                safe_invoke(invoke, [on_error]() { on_error("Too many requests - please wait a moment and try again."); });
            }
            else if (http_status >= 400)
            {
                std::string message_text = rsutils::string::from() << "The assistant returned an error (HTTP " << http_status << ").";
                safe_invoke(invoke, [on_error, message_text]() { on_error(message_text); });
            }
        }

        void assistant_chat_client::check_health(invoke_fn invoke, std::function<void(bool)> on_result)
        {
            auto me = shared_from_this();
            std::thread t([me, invoke, on_result]() {
                try
                {
                    http::curl_wrapper curl;
                    bool healthy = curl.get(std::string(BASE_URL) + "/api/health",
                        [](const char*, size_t) { return true; }, {}, false, ONE_SHOT_TIMEOUT_SEC);
                    safe_invoke(invoke, [on_result, healthy]() { on_result(healthy); });
                }
                catch (...)
                {
                    safe_invoke(invoke, [on_result]() { on_result(false); }); // an exception is a failed check
                }
            });
            t.detach();
        }

        void assistant_chat_client::send_reaction(const std::string& conversation_id, int value,
            invoke_fn invoke, error_callback on_error)
        {
            auto me = shared_from_this();
            std::thread t([me, conversation_id, value, invoke, on_error]() {
                try
                {
                    rsutils::json body_json;
                    body_json["conversationId"] = conversation_id;
                    body_json["value"] = value;

                    http::curl_wrapper curl;
                    long status = 0;
                    bool ok = curl.post_json(std::string(BASE_URL) + "/api/reactions",
                        body_json.dump(), "X-RS-Integration: viewer", &status);
                    if (!ok)
                    {
                        std::string message_text = rsutils::string::from()
                            << "Couldn't send feedback (HTTP " << status << ").";
                        safe_invoke(invoke, [on_error, message_text]() { on_error(message_text); });
                    }
                }
                catch (const std::exception& ex)
                {
                    std::string what = ex.what();
                    safe_invoke(invoke, [on_error, what]() { on_error(what); });
                }
                catch (...)
                {
                    safe_invoke(invoke, [on_error]() { on_error("Unknown error while sending feedback."); });
                }
            });
            t.detach();
        }
#endif
    }
}
