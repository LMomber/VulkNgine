#pragma once

#include "vkCommon.h"

struct Block {
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    bool free;
    void* ptr = nullptr; // Useless if it is a GPU allocation

    bool operator==(Block const& block)
    {
        if (memory == block.memory &&
            offset == block.offset &&
            size == block.size &&
            free == block.free &&
            ptr == block.ptr)
            return true;
        return false;
    }
};

class Device;
class Chunk 
{
public:
    Chunk(std::shared_ptr<Device> device, VkDeviceSize size, int memoryTypeIndex);

    bool Allocate(VkDeviceSize size, VkDeviceSize alignment, Block& block);
    bool IsIn(Block const& block) const;
    void Deallocate(Block const& block);
    int GetMemoryTypeIndex() const;

    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

protected:
    std::shared_ptr<Device> m_pDevice;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size;
    int m_memoryTypeIndex;
    std::vector<Block> m_blocks;
    void* m_ptr = nullptr;
};