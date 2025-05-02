#pragma once

#include "vkCommon.h"
#include "vkAbstractAllocator.h"
#include "vkChunkAllocator.h"

class Device;
class DeviceAllocator : public AbstractAllocator
{
public:
    DeviceAllocator(std::shared_ptr<Device> device, VkDeviceSize  size);

    Block Allocate(VkDeviceSize size, VkDeviceSize  alignment, int memoryTypeIndex);
    void Deallocate(Block& block);

private:
    ChunkAllocator m_chunkAllocator;
    std::vector<std::shared_ptr<Chunk>> m_chunks;
};