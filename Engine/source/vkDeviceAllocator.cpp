#include "vkDeviceAllocator.h"

DeviceAllocator::DeviceAllocator(std::shared_ptr<Device> device, VkDeviceSize size) :
	AbstractAllocator(device), m_chunkAllocator(device, size)
{
}

Block DeviceAllocator::Allocate(VkDeviceSize size, VkDeviceSize alignment, int memoryTypeIndex)
{
    Block block;

    // We search a "good" chunk
    for (auto& chunk : m_chunks)
        if (chunk->GetMemoryTypeIndex() == memoryTypeIndex)
            if (chunk->Allocate(size, alignment, block))
                return block;

    m_chunks.emplace_back(m_chunkAllocator.Allocate(size, memoryTypeIndex));
    assert(m_chunks.back()->Allocate(size, alignment, block));
    return block;
}

void DeviceAllocator::Deallocate(Block& block)
{
    for (auto& chunk : m_chunks) {
        if (chunk->IsIn(block)) {
            chunk->Deallocate(block);
            return;
        }
    }
    assert(!"unable to deallocate the block");
}
