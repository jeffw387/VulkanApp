#include <vulkan/vulkan.hpp>
#include <glm.hpp>

struct BufferStruct
{
    vk::CommandPool pool;
    vk::Fence drawCommandExecuted;
    vk::CommandBuffer drawCommandBuffer;
	vk::CommandBuffer stagingCommandBuffer;
	vk::Buffer matrixBuffer;
	vk::Buffer matrixStagingBuffer;
	vk::Semaphore matrixBufferStaged;
	vk::Fence stagingCommandExecuted;

};