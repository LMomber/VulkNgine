#include "vkData.h"

#include "vkCommandBuffer.h"

#include "vkResourceManager.h"

Vulkan::Vertex::Vertex(glm::vec3 pos, glm::vec3 normal, glm::vec3 tangent, glm::vec2 uv0, glm::vec2 uv1)
	: m_pos(pos), m_normal(normal), m_tangent(tangent), m_uv0(uv0), m_uv1(uv1)
{
}

void Vulkan::Material::CreateDescriptorSet(VkDevice device, VkDescriptorSetLayout layout, VkSampler sampler, VkDescriptorPool descriptorPool, const Vulkan::Material& fallback)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	const auto& rm = ResourceManager::Get();

	const Texture* pAlbedo = rm.textureOwner.GetOrNull(m_albedoTexture);
	const Texture* pNormal = rm.textureOwner.GetOrNull(m_normalTexture);
	const Texture* pMetallicRoughness = rm.textureOwner.GetOrNull(m_metallicRoughnessTexture);
	const Texture* pOcclusion = rm.textureOwner.GetOrNull(m_occlusionTexture);
	const Texture* pEmissive = rm.textureOwner.GetOrNull(m_emissiveTexture);

	VkDescriptorImageInfo albedoInfo{};
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoInfo.imageView = pAlbedo ? pAlbedo->m_imageView : rm.textureOwner.GetOrNull(fallback.m_albedoTexture)->m_imageView;
	albedoInfo.sampler = sampler;

	VkDescriptorImageInfo normalInfo{};
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = pNormal ? pNormal->m_imageView : rm.textureOwner.GetOrNull(fallback.m_normalTexture)->m_imageView;
	normalInfo.sampler = sampler;

	// Make ORM later
	VkDescriptorImageInfo metallicRoughnessInfo{};
	metallicRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicRoughnessInfo.imageView = pMetallicRoughness ? pMetallicRoughness->m_imageView : rm.textureOwner.GetOrNull(fallback.m_metallicRoughnessTexture)->m_imageView;
	metallicRoughnessInfo.sampler = sampler;

	VkDescriptorImageInfo occlusionInfo{};
	occlusionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	occlusionInfo.imageView = pOcclusion ? pOcclusion->m_imageView : rm.textureOwner.GetOrNull(fallback.m_occlusionTexture)->m_imageView;
	occlusionInfo.sampler = sampler;
	//

	VkDescriptorImageInfo emissiveInfo{};
	emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveInfo.imageView = pEmissive ? pEmissive->m_imageView : rm.textureOwner.GetOrNull(fallback.m_emissiveTexture)->m_imageView;
	emissiveInfo.sampler = sampler;

	std::array<VkDescriptorImageInfo, 5> infos{
	albedoInfo,
	normalInfo,
	metallicRoughnessInfo,
	occlusionInfo,
	emissiveInfo
	};

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkWriteDescriptorSet write{};

		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_descriptorSets[i];
		write.dstBinding = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = static_cast<uint32_t>(infos.size());
		write.pImageInfo = infos.data();

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}