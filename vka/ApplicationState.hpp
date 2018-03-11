#pragma once

#include "GLFW/glfw3.h"
#include "TimeHelper.hpp"

namespace vka
{
    struct ApplicationState
    {
        GLFWwindow* window;
        TimePoint_ms startupTimePoint;
        TimePoint_ms currentSimulationTime;
        bool gameLoop = false;
    };
}