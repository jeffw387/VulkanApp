#pragma once

namespace Input
{
    enum class BooleanInputEvents
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
        BooleanInputEvents triggerEvent = BooleanInputEvents::Pressed;
        ActionFunc func = nullptr;
        void operator()()
        {
            func();
        }
    };

    // boolean that toggles between true and false on a toggle event
    struct ToggleState
    {
        BooleanInputEvents toggleEvent = BooleanInputEvents::Pressed;
        bool active = false;
    };

    // boolean that becomes true on one event, and becomes false on another event
    struct MaintainState
    {
        BooleanInputEvents trueEvent = BooleanInputEvents::Pressed;
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