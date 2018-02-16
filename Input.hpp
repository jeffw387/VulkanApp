#pragma once
#include <functional>

namespace Input
{
    // an abstract notion of something that must be fired only once when given a particular input
    struct Action
    {
        std::function<void()> func;
    };

    // an abstract notion of something that becomes true or false based on input
    struct State
    {
        bool active = false;
    };

    // an abstract notion of a nonbinary state based on nonbinary input
    struct Range
    {
        std::function<float(float)> translator;
        float value;
    };
}