

cl /EHsc /MT /O2 ^
/std:c++17 ^
/D RELEASE ^
/I C:/VulkanSDK/1.0.54.0/Include ^
/I C:/Libraries/Compiled/glfw-3.2.1.bin.WIN64/include ^
/I C:/Libraries/Source/fmt-master /I C:/Libraries/Source/stb-master ^
/I C:/Libraries/Source/glm ^
main2.cpp ^
glfw3dll.lib ^
vulkan-1.lib ^
/link ^
/LIBPATH:C:/Libraries/Compiled/glfw-3.2.1.bin.WIN64/lib-vc2015 ^
/LIBPATH:C:/VulkanSDK/1.0.54.0/Lib