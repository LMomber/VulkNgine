#include "vkMemory.h"

#include "vkDevice.h"
#include "vkPhysicalDevice.h"

Chunk::Chunk(std::shared_ptr<Device> device, VkDeviceSize size, int memoryTypeIndex) :
	m_pDevice(device), m_size(size), m_memoryTypeIndex(memoryTypeIndex)
{
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	Block block;
	block.free;
	block.offset = 0;
	block.size = size;
	m_memory = block.memory = m_pDevice->AllocateMemory(allocInfo);

	const auto propertyFlags = device->GetPhysicalDevice()->GetMemoryProperties().memoryTypes[memoryTypeIndex].propertyFlags;

	if ((propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		m_ptr = device->MapMemory(m_memory, 0, VK_WHOLE_SIZE);
	}

	m_blocks.push_back(block);
}

bool Chunk::Allocate(VkDeviceSize size, VkDeviceSize alignment, Block& block)
{
    // if chunk is too small
    if (size > m_size)
        return false;

    for (uint32_t i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i].free) {
            // Compute virtual size after taking care about offsetAlignment
            uint32_t newSize = m_blocks[i].size;

            if (m_blocks[i].offset % alignment != 0)
                newSize -= alignment - m_blocks[i].offset % alignment;

            // If match
            if (newSize >= size) {

                // We compute offset and size that care about alignment (for this Block)
                m_blocks[i].size = newSize;
                if (m_blocks[i].offset % alignment != 0)
                    m_blocks[i].offset += alignment - m_blocks[i].offset % alignment;

                // Compute the ptr address
                if (m_ptr != nullptr)
                    m_blocks[i].ptr = (char*)m_ptr + m_blocks[i].offset;

                // if perfect match
                if (m_blocks[i].size == size) {
                    m_blocks[i].free = false;
                    block = m_blocks[i];
                    return true;
                }

                Block nextBlock;
                nextBlock.free = true;
                nextBlock.offset = m_blocks[i].offset + size;
                nextBlock.memory = m_memory;
                nextBlock.size = m_blocks[i].size - size;
                m_blocks.emplace_back(nextBlock); // We add the newBlock

                m_blocks[i].size = size;
                m_blocks[i].free = false;

                block = m_blocks[i];
                return true;
            }
        }
    }

    return false;
}

bool Chunk::IsIn(Block const& block) const
{
    return block.memory == m_memory;
}

void Chunk::Deallocate(Block const& block)
{
	auto blockIt(std::find(m_blocks.begin(), m_blocks.end(), block));
	assert(blockIt != m_blocks.end());

	// Just put the block to free
	blockIt->free = true;
}

int Chunk::GetMemoryTypeIndex() const
{
    return m_memoryTypeIndex;
}

Chunk::~Chunk()
{
    for (auto& block : m_blocks)
    {
        block.free = true;
    }
}
