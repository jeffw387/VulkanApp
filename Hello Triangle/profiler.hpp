#pragma once
#include <chrono>
#include <map>
#include "MPL.hpp"
#include <algorithm>
#include "circularBuffer.hpp"

namespace profiler
{
    using CodeID = size_t;
    using Description = char*;
    using profileClock = std::chrono::high_resolution_clock;
    using HiResTimePoint = profileClock::time_point;
    using HiResDuration = profileClock::duration;

    const auto maximumSamples = 100U;
    struct TimePointPair
    {
        HiResTimePoint start, end;
        HiResDuration duration;
        bool valid = false;
        bool durationCalculated = false;
    };

    std::map<CodeID, CircularBuffer<TimePointPair, maximumSamples>> profilingMap;
    std::map<CodeID, std::string> profileDescriptions;
    auto nextSampleID = 0U;

    void calcDuration(TimePointPair& pair)
    {
        pair.duration = pair.end - pair.start;
    }

    template <CodeID id>
    void startTimer()
    {
        TimePointPair pair = {};
        pair.start = profileClock::now();
        profilingMap[id][nextSampleID] = pair;
    }

    template <CodeID id>
    void endTimer()
    {
        auto& pair = profilingMap[id][nextSampleID];
        pair.end = profileClock::now();
        pair.valid = true;
        calcDuration(pair);
        nextSampleID++;
    }

    template <CodeID id>
    class Describe
    {
    public:
        Describe(std::string description)
        {
            profileDescriptions[id] = description;
        }
    };

    template <CodeID id>
    auto getAverageTime()
    {
        HiResDuration total = {};
        size_t validSamples = 0;
        for (auto& pair : profilingMap[id])
        {
            if (pair.valid)
            {
                validSamples++;
                if (!pair.durationCalculated)
                {
                    calcDuration(pair);
                }
                total += pair.duration;
            }
        }
        return total / validSamples;
    }

    template <CodeID id>
    auto getRollingAverage(size_t sampleCount)
    {
        auto max = nextSampleID;
        auto min = max - sampleCount;
        HiResDuration total = {};
        
        for (auto i = min; i < max; i++)
        {
            auto& pair = profilingMap[id][i];
            if (pair.valid)
            {
                if (!pair.durationCalculated)
                {
                    total += pair.duration;
                }
            }
        }
        return total / sampleCount;
    }

    template <CodeID id>
    auto getDescription()
    {
        return profileDescriptions[id];
    }

    auto toMicroseconds(HiResDuration duration)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(duration);
    }
}