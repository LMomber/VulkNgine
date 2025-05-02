#include "vkChunkAllocator.h"

ChunkAllocator::ChunkAllocator(std::shared_ptr<Device> device, VkDeviceSize size) : 
	m_pDevice(device), m_size(size)
{
	assert(IsPowerOfTwo(size));
}

std::unique_ptr<Chunk> ChunkAllocator::Allocate(VkDeviceSize size, int memoryTypeIndex)
{
	size = (size > m_size) ? NextPowerOfTwo(size) : m_size;

	return std::make_unique<Chunk>(m_pDevice, size, memoryTypeIndex);
}

VkDeviceSize ChunkAllocator::NextPowerOfTwo(VkDeviceSize size)
{
	VkDeviceSize power = static_cast<VkDeviceSize>(std::log2l(size) + 1);
	return static_cast<VkDeviceSize>(1 << power);
}

bool ChunkAllocator::IsPowerOfTwo(VkDeviceSize size)
{
	VkDeviceSize mask = 0;
	VkDeviceSize power = static_cast<VkDeviceSize>(std::log2l(size));

	for (VkDeviceSize i = 0; i < power; ++i)
		mask += static_cast<VkDeviceSize>(1 << i);

	return !(size & mask);
}
