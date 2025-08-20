// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 RealSense, Inc. All Rights Reserved.
#pragma once
#include <mutex>
#include <functional>

namespace rsutils {
    namespace concurrency {

        class shared_data_access
        {
        public:
            shared_data_access(std::mutex& mutex_ref)
                : m_mutex(mutex_ref) {
            }

            void write(const std::function<void()>& action)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                action();
            }

            template<class T>
            T read(const std::function<T()>& action) const
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return action();
            }

        private:
            std::mutex& m_mutex;
        };

    }
}
