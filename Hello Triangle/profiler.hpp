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
        bool durationCalculated = false;
    };

    std::map<CodeID, CircularBuffer<TimePointPair, maximumSamples>> profilingMap;
    std::map<CodeID, std::string> profileDescriptions;
    auto nextSampleID = 0U;

    template <CodeID id>
    void startTimer()
    {
        TimePointPair pair;
        pair.start = profileClock::now();
        profilingMap[id][nextSampleID] = pair;
    }

    template <CodeID id>
    void endTimer()
    {
        profilingMap[id][nextSampleID].end = profileClock::now();
        nextSampleID = (++nextSampleID) % maximumSamples;
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

    auto getDuration(TimePointPair& pair)
    {
        if (pair.durationCalculated)
        {
            return pair.duration;
        }
        pair.duration = pair.end - pair.start;
        pair.durationCalculated = true;
        return pair.duration;
    }

    template <CodeID id>
    auto getAverageTime()
    {
        HiResDuration total = {};
        for (auto& pair : profilingMap[id])
        {
            total += getDuration(pair);
        }
        return total / profilingMap[id].size();
    }

    template <CodeID id>
    auto getRollingAverage(size_t sampleCount)
    {
        auto max = nextSampleID;
        auto min = std::clamp(max - sampleCount, (size_t)0, max);
        HiResDuration total = {};
        
        for (auto i = min; i < max; i++)
        {
            total += getDuration(profilingMap[id][i]);
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