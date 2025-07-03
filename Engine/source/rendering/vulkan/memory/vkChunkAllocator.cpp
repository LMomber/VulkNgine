#include "vkChunkAllocator.h"

ChunkAllocator::ChunkAllocator(VkDeviceSize size) : 
	m_size(size)
{
	assert(IsPowerOfTwo(size));
}

std::unique_ptr<Chunk> ChunkAllocator::Allocate(VkDeviceSize size, int memoryTypeIndex) const
{
	size = (size > m_size) ? NextPowerOfTwo(size) : m_size;

	return std::make_unique<Chunk>(size, memoryTypeIndex);
}

VkDeviceSize ChunkAllocator::NextPowerOfTwo(VkDeviceSize size) const
{
	VkDeviceSize power = static_cast<VkDeviceSize>(std::log2l(static_cast<long double>(size)) + 1);
	return static_cast<VkDeviceSize>(static_cast<uint64_t>(1) << power);
}

bool ChunkAllocator::IsPowerOfTwo(VkDeviceSize size) const
{
	VkDeviceSize mask = 0;
	VkDeviceSize power = static_cast<VkDeviceSize>(std::log2l(static_cast<long double>(size)));

	for (VkDeviceSize i = 0; i < power; ++i)
		mask += static_cast<VkDeviceSize>(static_cast<uint64_t>(1) << i);

	return !(size & mask);
}
