cmake_minimum_required(VERSION 3.11)

project(VulkanApp)

add_executable(VulkanApp 
    main.cpp 
    vka/VulkanApp.cpp 
    vka/VulkanFunctions.cpp 
    vka/Allocator.cpp)

set(contentSourceDir "${CMAKE_SOURCE_DIR}/Content")
set(contentDestinationDir "${CMAKE_CURRENT_BINARY_DIR}")
file(INSTALL ${contentSourceDir} DESTINATION ${contentDestinationDir})

set(shaderSourceDir "${CMAKE_SOURCE_DIR}/Shaders")
set(shaderDestinationDir "${CMAKE_CURRENT_BINARY_DIR}")
file(INSTALL ${shaderSourceDir} DESTINATION ${shaderDestinationDir})

file(INSTALL ${CMAKE_SOURCE_DIR}/VulkanInitInfo.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(VulkanApp PUBLIC vka .)
target_compile_definitions(VulkanApp PRIVATE VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR)
target_compile_features(VulkanApp PRIVATE cxx_std_17)

add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE subprojects/vulkan/include)
target_link_libraries(VulkanApp PUBLIC vulkan)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE subprojects/glm)
target_link_libraries(VulkanApp PUBLIC glm)

add_subdirectory(subprojects/glfw)
target_link_libraries(VulkanApp PRIVATE glfw ${GLFW_LIBRARIES})

add_library(entt INTERFACE)
target_include_directories(entt INTERFACE subprojects/entt)
target_link_libraries(VulkanApp PRIVATE entt)

add_library(gsl INTERFACE)
target_include_directories(gsl INTERFACE subprojects/gsl-lite)
target_link_libraries(VulkanApp PRIVATE gsl)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE subprojects/stb)
target_link_libraries(VulkanApp PRIVATE stb)

add_library(freetype INTERFACE)
target_include_directories(freetype INTERFACE subprojects/freetype subprojects/freetype/include)
target_link_libraries(VulkanApp PRIVATE freetype)

add_library(json INTERFACE)
target_include_directories(json INTERFACE subprojects/json)
target_link_libraries(VulkanApp PRIVATE json)

set(bullet_DIR subprojects/bullet3)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
add_subdirectory(${bullet_DIR})
find_package(bullet CONFIG REQUIRED COMPONENTS BulletDynamics BulletCollision)
target_link_libraries(VulkanApp PRIVATE BULLET_PHYSICS::BulletDynamics BULLET_PHYSICS::BulletCollision)