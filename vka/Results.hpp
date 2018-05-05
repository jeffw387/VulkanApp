#pragma once

#include "vulkan/vulkan.h"

#include <type_traits>

namespace vka
{
    namespace Results
    {
        template <int N>
        struct Result
        {
            static constexpr int code = N;
        };

        template <int A, int B>
        constexpr bool operator==(Result<A> a, Result<B> b)
        {
            return A == B;
        }

        template <int A, int B>
        constexpr bool operator!=(Result<A> a, Result<B> b)
        {
            return A != B;
        }

        using Success = Result<VK_SUCCESS>;
        using NotReady = Result<VK_NOT_READY>;
        using Timeout = Result<VK_TIMEOUT>;
        using EventSet = Result<VK_EVENT_SET>;
        using EventReset = Result<VK_EVENT_RESET>;
        using Incomplete = Result<VK_INCOMPLETE>;
        using OutOfHostMemory = Result<VK_OUT_OF_HOST_MEMORY>;
        using OutOfDeviceMemory = Result<VK_OUT_OF_DEVICE_MEMORY>;
        using ErrorInitializationFailed = Result<VK_ERROR_INITIALIZATION_FAILED>;
        using ErrorDeviceLost = Result<VK_ERROR_DEVICE_LOST>;
        using ErrorMemoryMapFailed = Result<VK_ERROR_MEMORY_MAP_FAILED>;
        using ErrorLayerNotPresent = Result<VK_ERROR_LAYER_NOT_PRESENT>;
        using ErrorExtensionNotPresent = Result<VK_ERROR_EXTENSION_NOT_PRESENT>;
        using ErrorFeatureNotPresent = Result<VK_ERROR_FEATURE_NOT_PRESENT>;
        using ErrorIncompatibleDriver = Result<VK_ERROR_INCOMPATIBLE_DRIVER>;
        using ErrorTooManyObjects = Result<VK_ERROR_TOO_MANY_OBJECTS>;
        using ErrorFormatNotSupported = Result<VK_ERROR_FORMAT_NOT_SUPPORTED>;
        using ErrorFragmentedPool = Result<VK_ERROR_FRAGMENTED_POOL>;
        using ErrorOutOfPoolMemory = Result<VK_ERROR_OUT_OF_POOL_MEMORY>;
        using ErrorInvalidExternalHandle = Result<VK_ERROR_INVALID_EXTERNAL_HANDLE>;
        using ErrorSurfaceLost = Result<VK_ERROR_SURFACE_LOST_KHR>;
        using ErrorNativeWindowInUse = Result<VK_ERROR_NATIVE_WINDOW_IN_USE_KHR>;
        using Suboptimal = Result<VK_SUBOPTIMAL_KHR>;
        using ErrorOutOfDate = Result<VK_ERROR_OUT_OF_DATE_KHR>;
        using ErrorIncompatibleDisplay = Result<VK_ERROR_INCOMPATIBLE_DISPLAY_KHR>;
        using ErrorValidationFailed = Result<VK_ERROR_VALIDATION_FAILED_EXT>;
    }
}