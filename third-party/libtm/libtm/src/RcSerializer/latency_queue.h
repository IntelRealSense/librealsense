#include <ctype.h>
#include <queue>
#include <iostream>
#include <mutex>

#include "Packet.h"

using namespace perc;

class latency_queue
{
public:
    latency_queue(uint64_t latency, std::function<bool(uint64_t, std::shared_ptr<Packet>)> pop_func) :
        m_latency(latency), m_pop_func(pop_func), m_last_time(0) {}

    void push(uint64_t time, std::shared_ptr<Packet> buf)
    {
        std::lock_guard<std::mutex> lk(m_push_mutex);
        // backward comaptibility - pass-through buffers with 0 timestamp
        if (time == 0) {
            m_pop_func(0, buf);
        }
        m_queue.push(queue_element_t(time, buf));
        if (time >= m_last_time) m_last_time = time;
        while (!m_queue.empty()) {
            queue_element_t top = m_queue.top();
            if (m_last_time - top.first < m_latency) {
                break;
            }
            else {
                m_queue.pop();
                m_pop_func(top.first, top.second);
            }
        }
    }

    void drain()
    {
        std::lock_guard<std::mutex> lk(m_push_mutex);
        while (!m_queue.empty()) {
            queue_element_t top = m_queue.top();
            m_queue.pop();
            m_pop_func(top.first, top.second);
        }
    }

private:
    uint64_t m_latency;
    uint64_t m_last_time;
    std::function<bool(uint64_t, std::shared_ptr<Packet>)> m_pop_func;
    std::mutex m_push_mutex;
    typedef std::pair<uint64_t, std::shared_ptr<Packet>> queue_element_t;
    struct element_cmp {
        bool operator () (const queue_element_t &a, const queue_element_t &b) 
        { 
            return a.first > b.first; 
        }
    };
    std::priority_queue<queue_element_t,
        std::vector<queue_element_t>,
        element_cmp> m_queue;
};