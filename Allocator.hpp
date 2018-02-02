#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <map>

namespace vka
{

    struct Allocation
    {
        vk::DeviceMemory memory;
        vk::DeviceSize size;
        vk::DeviceSize offsetInDeviceMemory;
        uint32_t typeID;
    };

    namespace internal
    {
        struct SubAllocation
        {
            vk::DeviceSize size = 0U;
            bool allocated = false;
        };
        
        bool CanSuballocate(
            const SubAllocation& subAllocation,
            const vk::MemoryRequirements& requirements);

        struct MemoryBlock
        {
            MemoryBlock(vk::UniqueDeviceMemory&& memory, const vk::MemoryAllocateInfo& allocateInfo);
            vk::DeviceSize DivideSubAllocation(
                const vk::DeviceSize allocationOffset,
                const vk::MemoryRequirements& requirements);
            Allocation CreateExternalAllocationFromSuballocation(vk::DeviceSize allocationOffset);
            void DeallocateMemory(Allocation allocation);

            vk::UniqueDeviceMemory m_DeviceMemory;
            vk::MemoryAllocateInfo m_AllocateInfo;
            std::map<vk::DeviceSize, SubAllocation> m_Suballocations;
        };
    }// namespace internal

    struct AllocationDeleter
    {
        using pointer = Allocation;
        void operator()(Allocation allocation)
        {
            block->DeallocateMemory(allocation);
        }
        internal::MemoryBlock* block;
    };
    using UniqueAllocation = std::unique_ptr<Allocation, AllocationDeleter>;

    class Allocator
    {
    public:
        static const vk::DeviceSize DefaultMemoryBlockSize = 1000000U;
        Allocator(vk::PhysicalDevice physicalDevice, 
            vk::Device device, 
            vk::DeviceSize defaultBlockSize = DefaultMemoryBlockSize);
        std::optional<uint32_t> Allocator::ChooseMemoryType(vk::MemoryPropertyFlags memoryFlags);
        UniqueAllocation AllocateMemory(bool DedicatedAllocation, vk::MemoryRequirements requirements);
        UniqueAllocation AllocateForImage(
            bool DedicatedAllocation, 
            vk::Image image, 
            vk::MemoryPropertyFlags memoryFlags);
        UniqueAllocation AllocateForBuffer(
            bool DedicatedAllocation, 
            vk::Buffer buffer, 
            vk::MemoryPropertyFlags memoryFlags);

    private:
        internal::MemoryBlock&& AllocateNewBlock(const vk::MemoryAllocateInfo& allocateInfo);

        std::vector<internal::MemoryBlock> m_MemoryBlocks;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_Device;
        vk::DeviceSize m_DefaultBlockSize;
        vk::PhysicalDeviceMemoryProperties m_MemoryProperties;
    };
}// namespace vka