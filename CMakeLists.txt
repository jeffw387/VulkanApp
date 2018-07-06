cmake_minimum_required(VERSION 3.11)

project(VulkanApp)

add_executable(VulkanApp 
    main.cpp 
    vka/VulkanApp.cpp 
    vka/VulkanFunctions.cpp 
    vka/Allocator.cpp)


set(contentSourceDir "${CMAKE_SOURCE_DIR}/Content")
set(contentDestinationDir "${CMAKE_CURRENT_BINARY_DIR}/Content")
add_custom_target(ContentCopyTarget ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${contentSourceDir}" "${contentDestinationDir}"
	COMMENT "Copy content dir to build dir."
	VERBATIM)

set(shaderSourceDir "${CMAKE_SOURCE_DIR}/Shaders")
set(shaderDestinationDir "${CMAKE_CURRENT_BINARY_DIR}/Shaders")
add_custom_target(ShaderCopyTarget ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${shaderSourceDir}" "${shaderDestinationDir}"
	COMMENT "Copy shader dir to build dir."
	VERBATIM)

#set(configFiles 
#	config/fragmentDescriptorSetLayout2D.json 
#	config/fragmentShader2D.json
#	config/pipeline2D.json
#	config/pipelineLayout2D.json
#	config/renderPass.json
#	config/sampler2D.json
#	config/swapchainConfig.json
#	config/vertexShader2D.json
#	config/vk_layer_settings.txt
#	config/VulkanInitInfo.json)
set(configSourceDir "${CMAKE_SOURCE_DIR}/config")
set(configDestinationDir "${CMAKE_CURRENT_BINARY_DIR}/config")
add_custom_target(ConfigCopyTarget ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${configSourceDir}" "${configDestinationDir}"
	COMMENT "Copy config dir to build dir."
	VERBATIM)

add_dependencies(VulkanApp ContentCopyTarget ShaderCopyTarget ConfigCopyTarget)

target_include_directories(VulkanApp PUBLIC vka .)
target_compile_definitions(VulkanApp PRIVATE VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR)
target_compile_features(VulkanApp PRIVATE cxx_std_17)

add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE subprojects/vulkan/include)
target_link_libraries(VulkanApp PUBLIC vulkan)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE subprojects/glm)
target_link_libraries(VulkanApp PUBLIC glm)

set(GLFW_BUILD_TESTS off)
set(GLFW_BUILD_EXAMPLES off)
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

set(bullet_DIR subprojects/bullet)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
add_subdirectory(${bullet_DIR})
find_package(bullet CONFIG REQUIRED COMPONENTS BulletDynamics BulletCollision)
target_link_libraries(VulkanApp PRIVATE BULLET_PHYSICS::BulletDynamics BULLET_PHYSICS::BulletCollision)
