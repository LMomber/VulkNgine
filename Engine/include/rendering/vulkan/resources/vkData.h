#pragma once

#include "vkCommon.h"

#include "vkDevice.h"

#include "model.h"

#include "rid.h"

#include "glm/glm.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#pragma warning(push, 0)
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

namespace Vulkan
{
	struct Texture
	{
		VmaAllocation m_allocation;
		VkImage m_image;
		VkImageView m_imageView;
	};

	struct Vertex
	{
		Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uv0, glm::vec2 uv1);

		glm::vec3 m_pos;
		glm::vec3 m_color;
		glm::vec2 m_uv0;
		glm::vec2 m_uv1 = { 0.f, 0.f };

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		// TODO: include uv1
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
			attributeDescriptions.resize(3);
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, m_pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, m_color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, m_uv0);

			return attributeDescriptions;
		}

		bool operator==(const Vertex& other) const
		{
			return m_pos == other.m_pos && m_color == other.m_color && m_uv0 == other.m_uv0 && m_uv1 == other.m_uv1;
		}
	};

	struct Mesh
	{
		std::vector<Vertex> m_vertices{};
		std::vector<uint32_t> m_indices{};

		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;

		VmaAllocation m_vertexAllocation;
		VmaAllocation m_indexAllocation;
	};

	struct Material
	{
		RID m_albedoTexture{};
		RID m_normalTexture{};
		RID m_metallicRoughnessTexture{};
		RID m_occlusionTexture{};
		RID m_emissiveTexture{};

		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets{};
		void CreateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
	};

	struct Model
	{
		std::vector<RID> m_meshes{};
		Material m_material;
	};

	RID CreateGpuTexture(std::shared_ptr<Device> device, const std::shared_ptr<CPU::MaterialTexture> srcTexture, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlagBits usageFlags, VmaMemoryUsage memoryFlags, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
	void DestroyGpuTexture(std::shared_ptr<Device> device, const Texture& texture);

	RID CreateGpuMesh(std::shared_ptr<Device> device, const CPU::Mesh& mesh);
	void DestroyGpuMesh(std::shared_ptr<Device> device, const Mesh& mesh);

	// VMA
	void CreateBuffer(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags);

	template <typename T>
	void CreateBufferWithStaging(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag);
	void CreateBufferWithStaging(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag);

	void CreateImage(std::shared_ptr<Device> device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation);
	VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	//

	void CreateTextureImage(std::shared_ptr<Device> device, const std::shared_ptr<CPU::MaterialTexture> srcTexture, Vulkan::Texture& dstTexture, VkImageUsageFlagBits flags, VmaMemoryUsage memoryFlag, VkSharingMode sharingMode);

	VkFormat GetVkFormat(CPU::TextureFormat format);

	static struct Samplers
	{
		VkSampler m_linearRepeatAnisotropic;
	} samplers;

	static Material fallbackMaterial;

	static std::vector<Model> modelsToRender{};

	static RID_Owner<Texture> textureOwner;
	static RID_Owner<Mesh> meshOwner;
}

// From Boost
inline void hash_combine(size_t& seed, size_t value)
{
	seed ^= value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
}

// TODO: normals and tangents
template<> struct std::hash<Vulkan::Vertex>
{
	size_t operator()(Vulkan::Vertex const& v) const noexcept
	{
		size_t seed = 0;

		hash_combine(seed, std::hash<glm::vec3>{}(v.m_pos));
		hash_combine(seed, std::hash<glm::vec3>{}(v.m_color));
		hash_combine(seed, std::hash<glm::vec2>{}(v.m_uv0));
		hash_combine(seed, std::hash<glm::vec2>{}(v.m_uv1));

		return seed;
	}

};
