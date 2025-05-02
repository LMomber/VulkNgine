#pragma once

#include "vkMemory.h"

class Device;
class ChunkAllocator
{
public:
    ChunkAllocator(std::shared_ptr<Device> device, VkDeviceSize size);

    // if size > mSize, allocate to the next power of 2
    std::unique_ptr<Chunk> Allocate(VkDeviceSize size, int memoryTypeIndex);

    ChunkAllocator(const ChunkAllocator&) = delete;
    ChunkAllocator& operator=(const ChunkAllocator&) = delete;

private:
    VkDeviceSize NextPowerOfTwo(VkDeviceSize size);
    bool IsPowerOfTwo(VkDeviceSize size);

    std::shared_ptr<Device> m_pDevice;
    VkDeviceSize m_size;
};