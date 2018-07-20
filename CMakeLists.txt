cmake_minimum_required(VERSION 3.11)

project(VulkanApp)

add_executable(VulkanApp 
    main.cpp 
    vka/VulkanFunctions.cpp 
    vka/Allocator.cpp)

set(VulkanSDKPath C:/VulkanSDK/1.1.70.1)
set(ShaderOutput ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${ShaderOutput})

set(shaderGroup 2D 3D)
set(shaderStages vert frag)
foreach(stage ${shaderStages})
	foreach(group ${ShaderGroup})
		add_custom_target(${stage}Shader${group}
			COMMAND ${VulkanSDKPath}/Bin/glslangValidator.exe -V 
				shader.${stage} -o ${ShaderOutput}/${group}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/shaders/${group})
		add_dependencies(VulkanApp ${stage}Shader${group})
	endforeach(group)
endforeach(stage)

set(contentSourceDir "${CMAKE_SOURCE_DIR}/content")
set(contentDestinationDir "${CMAKE_CURRENT_BINARY_DIR}/content")
add_custom_target(contentCopyTarget ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${contentSourceDir}" "${contentDestinationDir}"
	COMMENT "Copy content dir to build dir."
	VERBATIM)

set(configSourceDir "${CMAKE_SOURCE_DIR}/config")
set(configDestinationDir "${CMAKE_CURRENT_BINARY_DIR}/config")
add_custom_target(ConfigCopyTarget ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${configSourceDir}" "${configDestinationDir}"
	COMMENT "Copy config dir to build dir."
	VERBATIM)

add_dependencies(VulkanApp contentCopyTarget ConfigCopyTarget)

target_include_directories(VulkanApp PUBLIC vka .)
target_compile_definitions(VulkanApp PRIVATE VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR)
target_compile_features(VulkanApp PRIVATE cxx_std_17)

add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE "subprojects/vulkan/include")
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