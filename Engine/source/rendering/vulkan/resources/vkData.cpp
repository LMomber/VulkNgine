#include "vkData.h"

#include "vkCommandBuffer.h"

Vulkan::Vertex::Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uv0, glm::vec2 uv1)
	: m_pos(pos), m_color(color), m_uv0(uv0), m_uv1(uv1)
{
}

void Vulkan::Material::CreateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	const Texture* pAlbedo = textureOwner.GetOrNull(m_albedoTexture);
	const Texture* pNormal = textureOwner.GetOrNull(m_normalTexture);
	const Texture* pMetallicRoughness = textureOwner.GetOrNull(m_metallicRoughnessTexture);
	const Texture* pOcclusion = textureOwner.GetOrNull(m_occlusionTexture);
	const Texture* pEmissive = textureOwner.GetOrNull(m_emissiveTexture);

	VkDescriptorImageInfo albedoInfo{};
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoInfo.imageView = pAlbedo ? pAlbedo->m_imageView : textureOwner.GetOrNull(fallbackMaterial.m_albedoTexture)->m_imageView;
	albedoInfo.sampler = samplers.m_linearRepeatAnisotropic;

	VkDescriptorImageInfo normalInfo{};
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = pNormal ? pNormal->m_imageView : textureOwner.GetOrNull(fallbackMaterial.m_normalTexture)->m_imageView;
	normalInfo.sampler = samplers.m_linearRepeatAnisotropic;

	// Make ORM later
	VkDescriptorImageInfo metallicRoughnessInfo{};
	metallicRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicRoughnessInfo.imageView = pMetallicRoughness ? pMetallicRoughness->m_imageView : textureOwner.GetOrNull(fallbackMaterial.m_metallicRoughnessTexture)->m_imageView;
	metallicRoughnessInfo.sampler = samplers.m_linearRepeatAnisotropic;

	VkDescriptorImageInfo occlusionInfo{};
	occlusionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	occlusionInfo.imageView = pOcclusion ? pOcclusion->m_imageView : textureOwner.GetOrNull(fallbackMaterial.m_occlusionTexture)->m_imageView;
	occlusionInfo.sampler = samplers.m_linearRepeatAnisotropic;
	//

	VkDescriptorImageInfo emissiveInfo{};
	emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveInfo.imageView = pEmissive ? pEmissive->m_imageView : textureOwner.GetOrNull(fallbackMaterial.m_emissiveTexture)->m_imageView;
	emissiveInfo.sampler = samplers.m_linearRepeatAnisotropic;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::array<VkWriteDescriptorSet, 5> writes{};

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = m_descriptorSets[i];
		writes[0].dstBinding = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorCount = 1;
		writes[0].pImageInfo = &albedoInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = m_descriptorSets[i];
		writes[1].dstBinding = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].descriptorCount = 1;
		writes[1].pImageInfo = &normalInfo;

		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = m_descriptorSets[i];
		writes[2].dstBinding = 2;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[2].descriptorCount = 1;
		writes[2].pImageInfo = &metallicRoughnessInfo;

		writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[3].dstSet = m_descriptorSets[i];
		writes[3].dstBinding = 3;
		writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[3].descriptorCount = 1;
		writes[3].pImageInfo = &occlusionInfo;

		writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[4].dstSet = m_descriptorSets[i];
		writes[4].dstBinding = 4;
		writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[4].descriptorCount = 1;
		writes[4].pImageInfo = &emissiveInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}

RID Vulkan::CreateGpuTexture(std::shared_ptr<Device> device, const std::shared_ptr<CPU::MaterialTexture> srcTexture, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlagBits usageFlags, VmaMemoryUsage memoryFlags, VkSharingMode sharingMode)
{
	Texture dstTexture{};
	CreateTextureImage(device, srcTexture, dstTexture, usageFlags, memoryFlags, sharingMode);
	dstTexture.m_imageView = CreateImageView(device->GetVkDevice(), dstTexture.m_image, format, aspectFlags);

	return textureOwner.CreateRID(dstTexture);
}

void Vulkan::DestroyGpuTexture(std::shared_ptr<Device> device, const Texture& texture)
{
	const auto vkDevice = device->GetVkDevice();
	vkDestroyImageView(vkDevice, texture.m_imageView, nullptr);
	vmaDestroyImage(device->GetAllocator(), texture.m_image, texture.m_allocation);
}

RID Vulkan::CreateGpuMesh(std::shared_ptr<Device> device, const CPU::Mesh& mesh)
{
	Mesh vkMesh;
	const auto& verts = mesh.GetVertices();
	const auto& coords = mesh.GetTexCoords();

	vkMesh.m_vertices.reserve(verts.size());
	for (uint32_t i = 0; i < verts.size(); i++)
	{
		vkMesh.m_vertices.emplace_back(verts[i], glm::vec3{ 0 }, coords[0][i], coords.size() > 1 ? coords[1][i] : glm::vec2{ 0, 0 });
	}
	const VkDeviceSize vertexBufferSize = sizeof(vkMesh.m_vertices[0]) * vkMesh.m_vertices.size();
	CreateBufferWithStaging(device, vertexBufferSize, vkMesh.m_vertexBuffer, vkMesh.m_vertexAllocation, vkMesh.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	vkMesh.m_indices = mesh.GetIndices();
	const VkDeviceSize indexBufferSize = sizeof(vkMesh.m_indices[0]) * vkMesh.m_indices.size();
	CreateBufferWithStaging(device, indexBufferSize, vkMesh.m_indexBuffer, vkMesh.m_indexAllocation, vkMesh.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	return meshOwner.CreateRID(vkMesh);
}

void Vulkan::DestroyGpuMesh(std::shared_ptr<Device> device, const Mesh& mesh)
{
	vmaDestroyBuffer(device->GetAllocator(), mesh.m_vertexBuffer, mesh.m_vertexAllocation);
	vmaDestroyBuffer(device->GetAllocator(), mesh.m_indexBuffer, mesh.m_indexAllocation);
}

VkImageView Vulkan::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageView imageView;

	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view");
	}

	return imageView;
}

VkFormat Vulkan::GetVkFormat(CPU::TextureFormat format)
{
	switch (format)
	{
	case CPU::TextureFormat::RGBA8_UNORM:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case CPU::TextureFormat::RGB8_UNORM:
		return VkFormat::VK_FORMAT_R8G8B8_UNORM;
		break;
	case CPU::TextureFormat::RG8_UNORM:
		return VkFormat::VK_FORMAT_R8G8_UNORM;
		break;
	case CPU::TextureFormat::R8_UNORM:
		return VkFormat::VK_FORMAT_R8_UNORM;
		break;
	case CPU::TextureFormat::SRGBA8:
		return VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		break;
	default:
		throw std::logic_error("Non-specified format used.");
		break;
	}
}

void Vulkan::CreateBuffer(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsageFlags;

	vmaCreateBuffer(device->GetAllocator(), &bufferInfo, &allocInfo,
		&buffer, &allocation, nullptr);
}

template <typename T>
void Vulkan::CreateBufferWithStaging(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag)
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(device, size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(device->GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData.data(), static_cast<size_t>(size));
		vmaUnmapMemory(device->GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(device, size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = device->BeginSingleTimeCommands(device->GetCurrentFrame());
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	device->EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(device->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Vulkan::CreateBufferWithStaging(std::shared_ptr<Device> device, VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag)
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(device, size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(device->GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData, static_cast<size_t>(size));
		vmaUnmapMemory(device->GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(device, size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = device->BeginSingleTimeCommands(device->GetCurrentFrame());
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	device->EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(device->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Vulkan::CreateImage(std::shared_ptr<Device> device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | ImageUsageFlags;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsageFlags;

	vmaCreateImage(device->GetAllocator(), &imageInfo, &allocInfo,
		&image, &imageAllocation, nullptr);
}

void Vulkan::CreateTextureImage(std::shared_ptr<Device> device, const std::shared_ptr<CPU::MaterialTexture> srcTexture, Vulkan::Texture& dstTexture, VkImageUsageFlagBits flags, VmaMemoryUsage memoryFlag, VkSharingMode sharingMode)
{
	VkDeviceSize imageSize = srcTexture->m_width * srcTexture->m_height * 4;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation{};
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		vmaCreateBuffer(device->GetAllocator(), &bufferInfo, &allocInfo,
			&stagingBuffer, &stagingAllocation, nullptr);

		void* data;
		vmaMapMemory(device->GetAllocator(), stagingAllocation, &data);
		memcpy(data, srcTexture->m_pixels.data(), srcTexture->m_pixels.size());
		vmaUnmapMemory(device->GetAllocator(), stagingAllocation);
	}

	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(srcTexture->m_width);
		imageInfo.extent.height = static_cast<uint32_t>(srcTexture->m_height);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = GetVkFormat(srcTexture->m_format);;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | flags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = sharingMode;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = memoryFlag;

		vmaCreateImage(device->GetAllocator(), &imageInfo, &allocInfo,
			&dstTexture.m_image, &dstTexture.m_allocation, nullptr);
	}

	CommandBuffer commandBuffer = device->BeginSingleTimeCommands(device->GetCurrentFrame());
	commandBuffer.TransitionImageLayout(dstTexture.m_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	commandBuffer.CopyBufferToImage(stagingBuffer, dstTexture.m_image,
		static_cast<uint32_t>(srcTexture->m_width), static_cast<uint32_t>(srcTexture->m_height));
	commandBuffer.TransitionImageLayout(dstTexture.m_image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	device->EndSingleTimeCommands(commandBuffer);

	vmaDestroyBuffer(device->GetAllocator(), stagingBuffer, stagingAllocation);
}