#include "vkDeviceAllocator.h"

DeviceAllocator::DeviceAllocator(VkDeviceSize size) :
	m_chunkAllocator(size)
{
}

Block DeviceAllocator::Allocate(VkDeviceSize size, VkDeviceSize alignment, int memoryTypeIndex)
{
    Block block;

    // Search a chunk of the right memory type
    for (auto& chunk : m_chunks)
    {
        if (chunk->GetMemoryTypeIndex() == memoryTypeIndex)
        {
            if (chunk->Allocate(size, alignment, block))
            {
                return block;
            }
        }
    }

    m_chunks.emplace_back(m_chunkAllocator.Allocate(size, memoryTypeIndex));
    assert(m_chunks.back()->Allocate(size, alignment, block));

    return block;
}

void DeviceAllocator::Deallocate(const Block& block)
{
    for (auto& chunk : m_chunks) 
    {
        if (chunk->IsIn(block)) 
        {
            chunk->Deallocate(block);
            return;
        }
    }
    assert(!"unable to deallocate the block");
}
