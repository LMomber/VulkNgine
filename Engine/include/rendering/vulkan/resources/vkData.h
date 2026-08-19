#pragma once

#include "vkCommon.h"
#include "model.h"
#include "rid.h"

#include "vkTexture.h"

namespace Vulkan
{
	struct Material
	{
		RID m_albedoTexture{};
		RID m_normalTexture{};
		RID m_metallicRoughnessTexture{};
		RID m_occlusionTexture{};
		RID m_emissiveTexture{};

		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets{};
		void CreateDescriptorSet(VkDevice device, VkDescriptorSetLayout layout, VkSampler sampler, VkDescriptorPool descriptorPool, const Vulkan::Material& fallback);
	};

	struct Model
	{
		RID m_mesh{};
		Material m_material;
		bool m_isInvisible = false;

		size_t m_hash = 0;
	};
}
