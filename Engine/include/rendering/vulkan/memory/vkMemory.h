#pragma once

#include "vkCommon.h"

struct Block 
{
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    bool free;
    void* ptr = nullptr; // Useless if it is a GPU allocation

    bool operator==(const Block& block)
    {
        if (memory == block.memory &&
            offset == block.offset &&
            size == block.size &&
            free == block.free &&
            ptr == block.ptr)
        {
            return true;
        }
        return false;
    }
};

class Device;
class Chunk 
{
public:
    Chunk(VkDeviceSize size, int memoryTypeIndex);

    bool Allocate(VkDeviceSize size, VkDeviceSize alignment, Block& block);
    bool IsIn(const Block& block) const;
    void Deallocate(const Block& block);
    int GetMemoryTypeIndex() const;

    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

protected:
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size;
    int m_memoryTypeIndex;
    std::vector<Block> m_blocks;
    void* m_ptr = nullptr;
};