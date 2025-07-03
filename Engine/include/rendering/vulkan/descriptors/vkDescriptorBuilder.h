#pragma once

#include "vkCommon.h"

#include "vkDescriptorAllocator.h"

class DescriptorLayoutCache;
class DescriptorBuilder
{
public:
	static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

	DescriptorBuilder& BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
	DescriptorBuilder& BindImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

	bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
	bool Build(VkDescriptorSet& set);

private:
	DescriptorBuilder();
	
	std::vector<VkWriteDescriptorSet> m_writes;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;

	DescriptorLayoutCache* m_cache;
	DescriptorAllocator* m_alloc;
};