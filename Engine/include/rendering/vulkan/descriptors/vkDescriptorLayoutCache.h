#pragma once

#include "vkCommon.h"

#include <unordered_map>

class DescriptorLayoutCache
{
public:
	DescriptorLayoutCache(VkDevice device);
	~DescriptorLayoutCache();

	VkDescriptorSetLayout CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info);

	struct DescriptorLayoutInfo
	{
		// TODO: Turn into inline array? Research why
		std::vector<VkDescriptorSetLayoutBinding> m_bindings;

		bool operator==(const DescriptorLayoutInfo& other) const;

		size_t Hash() const;
	};

private:
	struct DescriptorLayoutHash
	{
		std::size_t operator()(const DescriptorLayoutInfo& other) const;
	};

	VkDevice m_device;
	std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
};