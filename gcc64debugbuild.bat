g++ ^
-std=c++17 ^
-g ^
-IC:/VulkanSDK/1.0.65.1/Include ^
-IC:/Projects/WindowsPlatform/ ^
-IC:/Libraries/Source/fmt-master ^
-IC:/Libraries/Source/stb-master ^
-IC:/Libraries/Source/glm ^
-IC:/Libraries/Source/stx-btree-0.9/include ^
-IC:/Libraries/Source/freetype-2.8.1/include ^
-IC:/Libraries/Source/gsl/include ^
-IC:/Libraries/Source/VulkanMemoryAllocator/src/ ^
-DVK_USE_PLATFORM_WIN32_KHR ^
main2.cpp ^
VulkanApp2.cpp ^
-LC:/VulkanSDK/1.0.65.1/Lib ^
-LC:/Libraries/Compiled/freetype/Debug ^
-L"C:/Libraries/Source/freetype/2.8.1/freetype-2.8.1/builds/windows/vc2010/Bin/x64/Debug Multithreaded" ^
-lvulkan-1 ^
-lfreetype281MTd