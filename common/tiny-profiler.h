#pragma once

#include <unordered_map>
#include <chrono>
#include <iostream>
#include <cstring> // strlen

class scoped_timer
{
public:
    void tocout(long a)
    {
        long c = 1;

        if (a<0) { a *= -1; std::cout << "-"; }
        while ((c *= 1000)<a);
        while (c>1)
        {
            int t = (a%c) / (c / 1000);
            std::cout << (((c>a) || (t>99)) ? "" : ((t>9) ? "0" : "00")) << t;
            std::cout << (((c /= 1000) == 1) ? "" : ",");
        }
    }

    struct profiler
    {
        std::unordered_map<const char*, double> duration;
        std::unordered_map<const char*, int>    counts;
        std::unordered_map<const char*,
            std::chrono::high_resolution_clock::time_point> lasts;
        int scope = 0;

        static profiler& instance()
        {
            static profiler inst;
            return inst;
        }
    };

    scoped_timer(const char* key) : key(key)
    {
        _started = std::chrono::high_resolution_clock::now();

        auto& lasts = profiler::instance().lasts;

        if (lasts.find(key) == lasts.end())
            lasts[key] = std::chrono::high_resolution_clock::now();

        auto since_last = _started - lasts[key];
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(since_last).count();

        if (sec >= 2)
        {
            if (!profiler::instance().scope)
            {
                for (int i = 0; i < 50; i++) std::cout << "=";
                std::cout << "\n";
            }
            lasts[key] = _started;
            for (int i = 0; i < profiler::instance().scope; i++)
                std::cout << "  ";
            auto l = strlen(key);
            std::cout << key;
            std::cout << " ";
            for (int i = 0; i < 50 - int(l) - profiler::instance().scope * 2; i++)
                std::cout << ".";
            auto avg = (profiler::instance().duration[key]
                / profiler::instance().counts[key]);
            std::cout << " ";
            tocout(long(avg));
            std::cout << " usec,\t" << (profiler::instance().counts[key] / 2) << " Hz\n";
            profiler::instance().duration[key] = 0;
            profiler::instance().counts[key] = 1;
        }

        profiler::instance().scope++;
    }

    ~scoped_timer()
    {
        auto ended = std::chrono::high_resolution_clock::now();
        auto duration = ended - _started;
        auto usec = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        profiler::instance().duration[key] += usec;
        profiler::instance().counts[key]++;
        profiler::instance().scope--;
    }

private:
    std::chrono::high_resolution_clock::time_point _started;
    const char* key;
};
