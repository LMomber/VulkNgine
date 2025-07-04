#include "vkMemory.h"

#include "vkDevice.h"
#include "vkPhysicalDevice.h"

#include "engine.h"

static inline VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

Chunk::Chunk(VkDeviceSize size, int memoryTypeIndex) :
	m_size(size), m_memoryTypeIndex(memoryTypeIndex)
{
	auto& device = Core::engine.GetDevice();

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	Block block;
	block.free;
	block.offset = 0;
	block.size = size;
	m_memory = block.memory = device.AllocateMemory(allocInfo);

	const auto propertyFlags = device.GetPhysicalDevice()->GetMemoryProperties().memoryTypes[memoryTypeIndex].propertyFlags;

	if ((propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		m_ptr = device.MapMemory(m_memory, 0, VK_WHOLE_SIZE);
	}

	m_blocks.push_back(block);
}

bool Chunk::Allocate(VkDeviceSize size, VkDeviceSize alignment, Block& block)
{
	// if chunk is too small
	if (size > m_size)
	{
		return false;
	}

	for (uint32_t i = 0; i < m_blocks.size(); ++i)
	{
		Block& current = m_blocks[i];
		if (!current.free)
		{
			continue;
		}

		VkDeviceSize alignedOffset = AlignUp(current.offset, alignment);
		VkDeviceSize padding = alignedOffset - current.offset;

		if (padding + size > current.size)
		{
			continue;
		}

		VkDeviceSize remainingSize = current.size - (padding + size);

		// Split the padding that this allocation requires into it's own memory block and inject in between blocks.
		if (padding > 0)
		{
			Block paddingBlock;
			paddingBlock.free = true;
			paddingBlock.offset = current.offset;
			paddingBlock.size = padding;
			paddingBlock.memory = current.memory;
			paddingBlock.ptr = m_ptr ? static_cast<char*>(m_ptr) + current.offset : nullptr;

			m_blocks.insert(m_blocks.begin() + i, paddingBlock);

			++i;
		}

		current.offset = alignedOffset;
		current.size = size;
		current.free = false;
		current.ptr = m_ptr ? static_cast<char*>(m_ptr) + current.offset : nullptr;

		block = current;

		// Add a block for the remaining free memory
		if (remainingSize > 0)
		{
			Block newBlock;
			newBlock.free = true;
			newBlock.offset = alignedOffset + size;
			newBlock.size = remainingSize;
			newBlock.memory = current.memory;
			newBlock.ptr = m_ptr ? static_cast<char*>(m_ptr) + newBlock.offset : nullptr;

			m_blocks.insert(m_blocks.begin() + i + 1, newBlock);
		}

		return true;
	}

	return false;
}

bool Chunk::IsIn(Block const& block) const
{
	return block.memory == m_memory;
}

void Chunk::Deallocate(const Block& block)
{
	auto blockIt(std::find(m_blocks.begin(), m_blocks.end(), block));
	assert(blockIt != m_blocks.end());

	blockIt->free = true;

	// Coalescing next block
	auto nextBlock = std::next(blockIt);
	if (nextBlock != m_blocks.end())
	{
		if (nextBlock->free)
		{
			blockIt->size += nextBlock->size;
			m_blocks.erase(nextBlock);
		}
	}

	// Coalescing previous block
	if (blockIt != m_blocks.begin())
	{
		auto prevBlock = std::prev(blockIt);
		if (prevBlock->free)
		{
			prevBlock->size += blockIt->size;
			m_blocks.erase(blockIt);
		}
	}
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
