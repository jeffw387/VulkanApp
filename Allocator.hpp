#include <vulkan/vulkan.hpp>
#include <memory>
#include <optional>
#include <map>
#include <list>

namespace vka
{
    struct Allocation
    {
        vk::DeviceMemory memory;
        vk::DeviceSize size;
        vk::DeviceSize offsetInDeviceMemory;
        uint32_t typeID;
    };

    struct MemoryBlock;
    struct AllocationDeleter
    {
        void operator()(Allocation* allocation);
        AllocationDeleter(const AllocationDeleter&) noexcept = default;
        AllocationDeleter() noexcept = default;
        MemoryBlock* block;
    };
    using UniqueAllocation = std::unique_ptr<Allocation, AllocationDeleter>;

        struct SubAllocation
        {
            vk::DeviceSize size = 0U;
            bool allocated = false;
        };
        using SubAllocationMap = std::map<vk::DeviceSize, SubAllocation>;

        struct MemoryBlock
        {
            MemoryBlock(vk::UniqueDeviceMemory&& memory, const vk::MemoryAllocateInfo& allocateInfo);
            std::optional<vk::DeviceSize> CanSuballocate(const vk::MemoryRequirements& requirements);
            vk::DeviceSize DivideSubAllocation(
                const vk::DeviceSize allocationOffset,
                const vk::MemoryRequirements& requirements);
            UniqueAllocation CreateExternalAllocationFromSuballocation(vk::DeviceSize allocationOffset);
            void DeallocateMemory(Allocation allocation);

            vk::UniqueDeviceMemory m_DeviceMemory;
            vk::MemoryAllocateInfo m_AllocateInfo;
            SubAllocationMap m_Suballocations;
        };

    class Allocator
    {
    public:
        static const vk::DeviceSize DefaultMemoryBlockSize = 1000000U;
        Allocator() = default;
        Allocator(vk::PhysicalDevice physicalDevice, 
            vk::Device device, 
            vk::DeviceSize defaultBlockSize = DefaultMemoryBlockSize);
        std::optional<uint32_t> Allocator::ChooseMemoryType(vk::MemoryPropertyFlags memoryFlags, 
        const vk::MemoryRequirements& requirements);
        UniqueAllocation AllocateMemory(const bool DedicatedAllocation, 
        const vk::MemoryRequirements& requirements, 
        const vk::MemoryPropertyFlags memoryFlags);
        UniqueAllocation AllocateForImage(
            const bool DedicatedAllocation, 
            const vk::Image image, 
            const vk::MemoryPropertyFlags memoryFlags);
        UniqueAllocation AllocateForBuffer(
            const bool DedicatedAllocation, 
            const vk::Buffer buffer, 
            const vk::MemoryPropertyFlags memoryFlags);

    private:
        MemoryBlock& AllocateNewBlock(const vk::MemoryAllocateInfo& allocateInfo);

        std::list<MemoryBlock> m_MemoryBlocks;
        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_Device;
        vk::DeviceSize m_DefaultBlockSize;
        vk::PhysicalDeviceMemoryProperties m_MemoryProperties;
    };
}// namespace vka