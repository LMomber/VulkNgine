#pragma once

#include "vkCommon.h"

#include "rid.h"
#include "textureResolver.h"

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

	struct Model
	{
		std::vector<RID> m_meshes{};
	};

	struct RenderObject
	{
		Model m_model;
		std::vector<RID> m_textures;
	};

	struct Material
	{
		std::vector<ResolvedTextureSource> m_textures{};
	};
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
