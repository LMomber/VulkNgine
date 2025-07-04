#include "vkDescriptorBuilder.h"

#include "vkDescriptorLayoutCache.h"

#include "engine.h"
#include "vkDevice.h"

DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
{
	DescriptorBuilder builder;

	builder.m_cache = layoutCache;
	builder.m_alloc = allocator;

	return builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding newBinding{};
	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	m_bindings.push_back(newBinding);

	VkWriteDescriptorSet newWrite{};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;
	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.pBufferInfo = bufferInfo;
	newWrite.dstBinding = binding;

	m_writes.push_back(newWrite);

	return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding newBinding{};
	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	m_bindings.push_back(newBinding);

	VkWriteDescriptorSet newWrite{};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;
	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.pBufferInfo = nullptr;
	newWrite.pImageInfo = imageInfo;
	newWrite.dstBinding = binding;

	m_writes.push_back(newWrite);

	return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;

	layoutInfo.pBindings = m_bindings.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

	layout = m_cache->GetOrCreateDescriptorLayout(&layoutInfo);

	bool success = m_alloc->Allocate(&set, layout);
	if (!success)
	{ 
		return false; 
	}

	for (VkWriteDescriptorSet& w : m_writes)
	{
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(Core::engine.GetDevice().GetVkDevice(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

	return true;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;

	layoutInfo.pBindings = m_bindings.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

	VkDescriptorSetLayout layout = m_cache->GetOrCreateDescriptorLayout(&layoutInfo);

	bool success = m_alloc->Allocate(&set, layout);
	if (!success)
	{
		return false;
	}

	for (VkWriteDescriptorSet& w : m_writes)
	{
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(Core::engine.GetDevice().GetVkDevice(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);

	return true;
}

