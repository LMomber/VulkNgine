#include "vkRender.h"

#include "engine.h"
#include "transform.h"
#include "fileIO.h"
#include "renderComponents.h"

#include "vkPhysicalDevice.h"
#include "vkQueue.h"
#include "vkCommandBuffer.h"
#include "vkPipeline.h"
#include "vkPipelineCache.h"

#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <stdexcept>
#include <array>
#include <set>
#include <unordered_map>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.resize(3);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct MVP
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};

std::vector<Vertex> vertices;
std::vector<uint32_t> indices;

Renderer::Renderer(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	ChooseSharingMode();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	LoadModel();

	// Vertex data
	const VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
	CreateBufferWithStaging(vertexBufferSize, m_vertexBuffer, m_vertexAllocation, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	// Index data
	const VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
	CreateBufferWithStaging(indexBufferSize, m_indexBuffer, m_indexAllocation, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateSyncObjects();
}

Renderer::~Renderer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();

	vkDeviceWaitIdle(vkDevice);

	PipelineCache::Reset();
	ShaderCache::Reset();

	vkDestroySampler(vkDevice, m_textureSampler, nullptr);
	vkDestroyImageView(vkDevice, m_textureImageView, nullptr);

	vmaDestroyImage(m_pDevice->GetAllocator(), m_textureImage, m_textureAllocation);

	vkFreeDescriptorSets(vkDevice, m_descriptorPool, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data());
	vkDestroyDescriptorPool(vkDevice, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_descriptorSetLayout, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_uniformBuffers[i], m_uniformAllocations[i]);
	}

	vmaDestroyBuffer(m_pDevice->GetAllocator(), m_indexBuffer, m_indexAllocation);
	vmaDestroyBuffer(m_pDevice->GetAllocator(), m_vertexBuffer, m_vertexAllocation);

	vkDestroySemaphore(vkDevice, m_globalTimelineSemaphore, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_frameContexts[i].Destroy(m_pDevice);
	}

	for (size_t i = 0; i < m_renderFinishedPerImage.size(); i++)
	{
		if (m_renderFinishedPerImage[i]) vkDestroySemaphore(m_pDevice->GetVkDevice(), m_renderFinishedPerImage[i], nullptr);
	}
}

void Renderer::Update()
{
	UpdateMVP(m_currentFrame);
}

void Renderer::Render()
{
	FrameContext& frame = m_frameContexts[m_currentFrame];
	VkDevice device = m_pDevice->GetVkDevice();
	VkQueue graphicsQueue = m_pDevice->GetQueue()->GetQueue(QueueType::GRAPHICS);
	VkQueue presentQueue = m_pDevice->GetQueue()->GetQueue(QueueType::PRESENT);
	VkSwapchainKHR swapchain = m_pDevice->GetSwapchain()->GetVkSwapChain();
		
	// 1. Wait on previous frame
	// Use timeline semaphore as device-host synchronization
	VkSemaphoreWaitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &m_globalTimelineSemaphore;
	waitInfo.pValues = &frame.m_timelineValue;
	waitInfo.flags = 0;

	vkWaitSemaphores(device, &waitInfo, UINT64_MAX);

	// 2. Acquire next swapchain image
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		device,
		swapchain,
		UINT64_MAX,
		frame.m_imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_pDevice->GetSwapchain()->RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swapchain image");
	}

	// 3. Record draw commands
	m_pDevice->GetQueue()->ResetCommandPools(m_currentFrame);
	CommandBuffer commandBuffer =
		m_pDevice->GetQueue()->GetOrCreateCommandBuffer(QueueType::GRAPHICS, m_currentFrame);
	const VkCommandBuffer* pVkCommandBuffer = commandBuffer.GetVkPtr();

	RecordCommandBuffer(commandBuffer, imageIndex);

	// 4. Submit to queue
	uint64_t signalValue = ++m_currentTimelineValue;
	frame.m_timelineValue = signalValue;

	VkTimelineSemaphoreSubmitInfo timelineInfo{};
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.signalSemaphoreValueCount = 2;
	timelineInfo.pSignalSemaphoreValues = &signalValue;

	VkSemaphore waitSemaphores[] = { frame.m_imageAvailableSemaphore };
	VkSemaphore signalSemaphores[] = { m_globalTimelineSemaphore, m_renderFinishedPerImage[imageIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pVkCommandBuffer;
	submitInfo.signalSemaphoreCount = 2;
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.pNext = &timelineInfo;

	VkResult submitRes = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (submitRes != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	// 5. Present
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderFinishedPerImage[imageIndex];
	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	VkResult presentRes = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
	{
		m_pDevice->GetSwapchain()->RecreateSwapchain();
		return;
	}
	else if (presentRes != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swapchain image");
	}

	m_pDevice->GetQueue()->ResetCommandBufferIndices(m_currentFrame);

	// 6. Advance to next frame
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uniformLayoutBinding{};
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array< VkDescriptorSetLayoutBinding, 2> bindings = { uniformLayoutBinding , samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->GetVkDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateGraphicsPipeline()
{
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	bindingDescriptions.push_back(Vertex::GetBindingDescription());
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	colorBlendAttachments.push_back(colorBlendAttachment);

	std::vector<VkDescriptorSetLayout> layouts;
	layouts.push_back(m_descriptorSetLayout);

	std::vector<VkFormat> imageFormats;
	imageFormats.push_back(m_pDevice->GetSwapchain()->GetImageFormat());

	GraphicsPipelineInfo pipelineInfo{};
	pipelineInfo.SetShader("../Engine/shaders/vert.spv", ShaderType::VERTEX);
	pipelineInfo.SetShader("../Engine/shaders/frag.spv", ShaderType::FRAGMENT);
	pipelineInfo.SetDynamicStates(dynamicStates);
	pipelineInfo.SetVertexInputState(bindingDescriptions, attributeDescriptions);
	pipelineInfo.SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	pipelineInfo.SetViewportState();
	pipelineInfo.SetRasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineInfo.SetMultisampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);
	pipelineInfo.SetColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, colorBlendAttachments);
	pipelineInfo.SetLayoutInfo(layouts);
	pipelineInfo.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE);
	pipelineInfo.SetRenderInfo(imageFormats, m_pDevice->GetPhysicalDevice()->FindSupportedFormat(
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT));

	m_pipeline = PipelineCache::GetOrCreateGraphicsPipeline(pipelineInfo);
}

void Renderer::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture image");
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		vmaCreateBuffer(m_pDevice->GetAllocator(), &bufferInfo, &allocInfo,
			&stagingBuffer, &stagingAllocation, nullptr);

		void* data;
		vmaMapMemory(m_pDevice->GetAllocator(), stagingAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(m_pDevice->GetAllocator(), stagingAllocation);
	}

	stbi_image_free(pixels);

	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(texWidth);
		imageInfo.extent.height = static_cast<uint32_t>(texHeight);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(m_pDevice->GetAllocator(), &imageInfo, &allocInfo,
			&m_textureImage, &m_textureAllocation, nullptr);
	}

	CommandBuffer commandBuffer = BeginSingleTimeCommands();
	commandBuffer.TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	commandBuffer.CopyBufferToImage(stagingBuffer, m_textureImage,
		static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	commandBuffer.TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	EndSingleTimeCommands(commandBuffer);


	vmaDestroyBuffer(m_pDevice->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Renderer::CreateTextureImageView()
{
	m_textureImageView = CreateImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::CreateTextureSampler()
{
	auto properties = m_pDevice->GetPhysicalDevice()->GetProperties();

	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (vkCreateSampler(m_pDevice->GetVkDevice(), &createInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create sampler");
	}
}

void Renderer::CreateUniformBuffers()
{
	const auto bufferSize = sizeof(MVP);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformAllocations.resize(MAX_FRAMES_IN_FLIGHT);
	m_mappedUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocInfo{};
		if (vmaCreateBuffer(m_pDevice->GetAllocator(), &bufferInfo, &allocCreateInfo, &m_uniformBuffers[i], &m_uniformAllocations[i], &allocInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Memory allocation failed");
		}

		m_mappedUniformBuffers[i] = allocInfo.pMappedData;
	}
}

void Renderer::CreateSyncObjects()
{
	VkSemaphoreTypeCreateInfo timelineCreateInfo{};
	timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = &timelineCreateInfo;

	vkCreateSemaphore(m_pDevice->GetVkDevice(), &createInfo, nullptr, &m_globalTimelineSemaphore);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_frameContexts[i].Init(m_pDevice);
	}

	// Submit semaphores per swapchain images instead of per frame
	size_t imageCount = m_pDevice->GetSwapchain()->GetImages().size();
	m_renderFinishedPerImage.resize(imageCount);
	VkSemaphoreCreateInfo semInfo{};
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < imageCount; ++i) {
		if (vkCreateSemaphore(m_pDevice->GetVkDevice(), &semInfo, nullptr, &m_renderFinishedPerImage[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create renderFinished semaphore for image");
		}
	}
}

void Renderer::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(m_pDevice->GetVkDevice(), &createInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void Renderer::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(m_pDevice->GetVkDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_textureImageView;
		imageInfo.sampler = m_textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_pDevice->GetVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::LoadModel() const
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Renderer::ChooseSharingMode()
{
	QueueFamilyIndices queueFamilyIndices = m_pDevice->GetPhysicalDevice()->FindQueueFamilies(m_pDevice->GetPhysicalDevice()->GetDevice(), m_pDevice->GetSurface());

	std::set<uint32_t> queueSet = { queueFamilyIndices.m_graphicsFamily.value(), queueFamilyIndices.m_transferFamily.value() };
	std::vector<uint32_t> uniqueQueueFamilyIndices;

	// Iterator-based loop for practice
	for (auto it = queueSet.begin(); it != queueSet.end(); it++)
	{
		uniqueQueueFamilyIndices.push_back(*it);
	}

	m_queueSetIndices = uniqueQueueFamilyIndices;

	m_sharingMode = m_queueSetIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
}

void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation) const
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

	vmaCreateImage(m_pDevice->GetAllocator(), &imageInfo, &allocInfo,
		&image, &imageAllocation, nullptr);
}

VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
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

	if (vkCreateImageView(m_pDevice->GetVkDevice(), &createInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view");
	}

	return imageView;
}

void Renderer::CreateBuffer(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsageFlags;

	vmaCreateBuffer(m_pDevice->GetAllocator(), &bufferInfo, &allocInfo,
		&buffer, &allocation, nullptr);
}

template <typename T>
void Renderer::CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag)
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_pDevice->GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData.data(), static_cast<size_t>(size));
		vmaUnmapMemory(m_pDevice->GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = BeginSingleTimeCommands();
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(m_pDevice->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Renderer::CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag)
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	{
		CreateBuffer(size, stagingBuffer, stagingAllocation, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_pDevice->GetAllocator(), stagingAllocation, &data);
		memcpy(data, bufferData, static_cast<size_t>(size));
		vmaUnmapMemory(m_pDevice->GetAllocator(), stagingAllocation);
	}

	// Create vertex buffer in device local memory
	CreateBuffer(size, buffer, allocation, usageFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

	CommandBuffer commandBuffer = BeginSingleTimeCommands();
	commandBuffer.CopyBuffer(stagingBuffer, buffer, size);
	EndSingleTimeCommands(commandBuffer);

	// Cleanup staging
	vmaDestroyBuffer(m_pDevice->GetAllocator(), stagingBuffer, stagingAllocation);
}

void Renderer::UpdateMVP(const int currentImage)
{
	assert(currentImage < MAX_FRAMES_IN_FLIGHT && "Current frame value is higher than the amount of frames in flight");

	const auto cameraEntity = Core::engine.GetRegistry().view<Transform>().front();
	auto& cameraTransform = Core::engine.GetRegistry().get<Transform>(cameraEntity);
	const auto& camera = Core::engine.GetRegistry().get<Camera>(cameraEntity);

	Transform transform{};
	transform.SetTranslation(glm::vec3(1.f, 2.f, 5.f));

	const glm::vec3 trans = cameraTransform.GetTranslation();
	const glm::quat rot = cameraTransform.GetRotation();

	const glm::vec3 localForward = glm::vec3(0.f, 0.f, 1.f);
	const glm::vec3 forward = glm::normalize(glm::rotate(rot, localForward));

	const glm::vec3 focusPoint = trans + forward;
	const glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);

	MVP ubo{};
	auto world = glm::rotate(transform.World(), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.model = glm::rotate(world, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	ubo.view = glm::lookAtRH(trans, focusPoint, worldUp);
	ubo.projection = camera.projection;

	memcpy(m_mappedUniformBuffers[(currentImage)], &ubo, sizeof(ubo));
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_pDevice->GetVkDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Shader module creation failed");
	}

	return shaderModule;
}

void Renderer::RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	commandBuffer.BeginCommandBuffer(&beginInfo);

	const auto swapchain = m_pDevice->GetSwapchain();
	const auto& extent = swapchain->GetExtent();
	const auto& imageViews = swapchain->GetImageViews();
	const auto imageFormat = swapchain->GetImageFormat();
	VkImage swapchainImage = swapchain->GetImages()[imageIndex];

	// 1. Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
	commandBuffer.TransitionImageLayout(swapchainImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// 2. Begin dynamic rendering
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = imageViews[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = { 0.f, 0.f, 0.f, 0.f };

	VkRenderingAttachmentInfo depthAttachment{};
	depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachment.imageView = swapchain->GetDepthView();
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.clearValue.color = { 1.f, 0.f };

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.renderArea.offset = { 0, 0 };
	renderInfo.renderArea.extent = extent;
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachment;
	renderInfo.pDepthAttachment = &depthAttachment;
	renderInfo.pStencilAttachment = nullptr;

	commandBuffer.BeginRendering(&renderInfo);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	commandBuffer.SetViewPort(&viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	commandBuffer.SetScissor(&scissor);

	commandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Get());

	VkBuffer vertexBuffers[] = { m_vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	commandBuffer.BindVertexBuffers(vertexBuffers, offsets);
	commandBuffer.BindIndexBuffer(m_indexBuffer, VK_INDEX_TYPE_UINT32);

	const int descriptorSetIndex = m_currentFrame;
	commandBuffer.BindDescriptorSets(m_pipeline->GetLayout(), &m_descriptorSets[descriptorSetIndex]);
	commandBuffer.DrawIndexed(static_cast<uint32_t>(indices.size()));

	commandBuffer.EndRendering();

	// 3. Transition swapchain image back to PRESENT_SRC_KHR
	commandBuffer.TransitionImageLayout(swapchainImage, imageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	commandBuffer.EndCommandBuffer();
}

// Port to command buffer class
const CommandBuffer& Renderer::BeginSingleTimeCommands() const
{
	const QueueType type = QueueType::GRAPHICS;
	const auto queue = m_pDevice->GetQueue();

	const auto& commandBuffer = queue->CreateSingleTimeCommandBuffer(type, m_currentFrame);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	commandBuffer.BeginCommandBuffer(&beginInfo);

	return commandBuffer;
}

// Port to command buffer class
void Renderer::EndSingleTimeCommands(CommandBuffer commandBuffer) const
{
	commandBuffer.EndCommandBuffer();

	const QueueType type = QueueType::GRAPHICS;

	const auto vkCommandBuffer = commandBuffer.GetVkPtr();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = vkCommandBuffer;

	auto queue = m_pDevice->GetQueue();

	vkQueueSubmit(queue->GetQueue(type), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue->GetQueue(type));
}

void FrameContext::Init(std::shared_ptr<Device> device)
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device->GetVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS)
	{
		throw std::logic_error("Failed to create semaphores for FrameContext");
	}
}

void FrameContext::Destroy(std::shared_ptr<Device> device) const
{
	if (m_imageAvailableSemaphore) vkDestroySemaphore(device->GetVkDevice(), m_imageAvailableSemaphore, nullptr);
}
