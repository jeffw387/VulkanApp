#include <chrono>
#include <map>

namespace profiler
{
    using CodeID = size_t;
    using Description = char*;

    template <typename... Types>
    struct TypeList
    {};

    template <CodeID id, Description desc>
    struct Descriptor
    {

    };

    std::chrono::high_resolution_clock profileClock;

    std::map<CodeID, std::string> codeDescriptions;
    std::multimap<CodeID, std::chrono::duration> durationMap;

    template <CodeID id>
    void describeCode(std::string description)
    {
        codeDescriptions[id] = description;
    }

    template <CodeID id>
    void startTimer()
    {

    }
}