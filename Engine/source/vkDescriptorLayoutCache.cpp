#include "vkDescriptorLayoutCache.h"

#include <algorithm>

DescriptorLayoutCache::DescriptorLayoutCache(VkDevice device) :
	m_device(device)
{
}

DescriptorLayoutCache::~DescriptorLayoutCache()
{
	for (const auto& pair : m_layoutCache)
	{
		vkDestroyDescriptorSetLayout(m_device, pair.second, nullptr);
	}
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info)
{
	DescriptorLayoutInfo layoutInfo;
	layoutInfo.m_bindings.reserve(info->bindingCount);
	bool isSorted = true;
	int lastBinding = -1;

	for (uint32_t i = 0; i < info->bindingCount; i++)
	{
		layoutInfo.m_bindings.push_back(info->pBindings[i]);

		if (info->pBindings[i].binding > static_cast<uint32_t>(lastBinding))
		{
			lastBinding = info->pBindings[i].binding;
		}
		else
		{
			isSorted = false;
		}
	}

	if (!isSorted)
	{
		std::sort(layoutInfo.m_bindings.begin(), layoutInfo.m_bindings.end(), 
			[](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) { return a.binding < b.binding; });
	}

	auto it = m_layoutCache.find(layoutInfo);
	if (it != m_layoutCache.end())
	{
		return it->second;
	}
	else
	{
		VkDescriptorSetLayout layout;
		if (vkCreateDescriptorSetLayout(m_device, info, nullptr, &layout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		m_layoutCache[layoutInfo] = layout;

		return layout;
	}
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
	if (other.m_bindings.size() != m_bindings.size())
	{
		return false;
	}
	else
	{
		for (size_t i = 0; i < m_bindings.size(); i++)
		{
			if (other.m_bindings[i].binding != m_bindings[i].binding)
			{
				return false;
			}

			if (other.m_bindings[i].descriptorCount != m_bindings[i].descriptorCount)
			{
				return false;
			}

			if (other.m_bindings[i].descriptorType != m_bindings[i].descriptorType)
			{
				return false;
			}

			if (other.m_bindings[i].stageFlags != m_bindings[i].stageFlags)
			{
				return false;
			}
		}

		return true;
	}
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
{
	using std::size_t;
	using std::hash;

	size_t result = hash<size_t>()(m_bindings.size());

	for (const VkDescriptorSetLayoutBinding& binding : m_bindings)
	{
		size_t bindingHash = binding.binding | binding.descriptorType << 8 | binding.descriptorCount << 16 | binding.stageFlags << 24;

		result ^= hash<size_t>()(bindingHash);
	}

	return result;
}

std::size_t DescriptorLayoutCache::DescriptorLayoutHash::operator()(const DescriptorLayoutInfo& other) const
{
	return other.Hash();
}
