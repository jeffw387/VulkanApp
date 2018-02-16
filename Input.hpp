#pragma once

namespace Input
{
    enum class KeyStates
    {
        Pressed,
        Released
    };

    enum class ButtonStates
    {
        Pressed,
        Released
    };

    enum class Axes
    {
        LeftX,
        LeftY,
        RightX,
        RightY
    };

    // an abstract notion of something that must be fired only once when given a particular input
    using ActionFunc = void(*)();
    struct Action
    {
        ActionFunc func = nullptr;
    };

    // an abstract notion of something that becomes true or false based on input
    struct State
    {
        bool active = false;
    };

    // an abstract notion of a nonbinary state based on nonbinary input
    using RangeTranslator = float(*)(float);
    struct Range
    {
        RangeTranslator translator;
        float value;
    };
}