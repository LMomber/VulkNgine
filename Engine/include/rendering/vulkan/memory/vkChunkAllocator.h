#pragma once

#include "vkMemory.h"

class Device;
class ChunkAllocator
{
public:
    ChunkAllocator(VkDeviceSize size);

    // if size > mSize, allocate to the next power of 2
    std::unique_ptr<Chunk> Allocate(VkDeviceSize size, int memoryTypeIndex) const;

    ChunkAllocator(const ChunkAllocator&) = delete;
    ChunkAllocator& operator=(const ChunkAllocator&) = delete;

private:
    VkDeviceSize NextPowerOfTwo(VkDeviceSize size) const;
    bool IsPowerOfTwo(VkDeviceSize size) const;

    VkDeviceSize m_size;
};