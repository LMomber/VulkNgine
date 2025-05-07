#include "vkDescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(VkDevice device) :
	m_device(device)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
	for (auto pool : m_freePools)
	{
		vkDestroyDescriptorPool(m_device, pool, nullptr);
	}
	for (auto pool : m_usedPools)
	{
		vkDestroyDescriptorPool(m_device, pool, nullptr);
	}
}

VkDescriptorPool DescriptorAllocator::GrabPool()
{
	if (m_freePools.size() > 0)
	{
		VkDescriptorPool descriptorPool = m_freePools.back();
		m_freePools.pop_back();
		return descriptorPool;
	}
	else
	{
		return CreatePool(m_descriptorSizes, 1000, 0);
	}
}

VkDescriptorPool DescriptorAllocator::CreatePool(const PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags) const
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.sizes.size());
	for (const auto& sizePair : poolSizes.sizes)
	{
		sizes.push_back({ sizePair.first, static_cast<uint32_t>(sizePair.second * count) });
	}

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.flags = flags;
	createInfo.maxSets = count;
	createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
	createInfo.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	if (vkCreateDescriptorPool(m_device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}

	return descriptorPool;
}

void DescriptorAllocator::ResetPools()
{
	for (const auto& pool : m_usedPools)
	{
		vkResetDescriptorPool(m_device, pool, 0);
		m_freePools.push_back(pool);
	}

	m_usedPools.clear();

	m_currentPool = VK_NULL_HANDLE;
}

bool DescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
	if (m_currentPool == VK_NULL_HANDLE)
	{
		m_currentPool = GrabPool();
		m_usedPools.push_back(m_currentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = m_currentPool;
	allocInfo.descriptorSetCount = 1;

	VkResult allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, set);
	bool needReallocate = false;

	switch (allocResult)
	{
	case VK_SUCCESS:
		return true;
		break;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		needReallocate = true;
		break;
	default:
		return false;
		break;
	}

	if (needReallocate)
	{
		m_currentPool = GrabPool();
		m_usedPools.push_back(m_currentPool);

		allocInfo.descriptorPool = m_currentPool;

		allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, set);

		if (allocResult == VK_SUCCESS)
		{
			return true;
		}
	}

	return false;
}
