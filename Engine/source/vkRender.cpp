#include "vkRender.h"

#include "engine.h"
#include "transform.h"
#include "renderComponents.h"

#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdexcept>
#include <array>
#include <set>

// Delete later
#include <chrono>
//

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

struct MVP
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};

const std::vector<Vertex> vertices =
{
	// Front face
	{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // 1
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 2
	{{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // 3

	// Back face
	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 4
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 5
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // 6
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}}, // 7
};

const std::vector<uint16_t> indices =
{
	// Front face
	0, 1, 2, 2, 3, 0,
	// Right face
	1, 5, 6, 6, 2, 1,
	// Back face
	5, 4, 7, 7, 6, 5,
	// Left face
	4, 0, 3, 3, 7, 4,
	// Top face
	3, 2, 6, 6, 7, 3,
	// Bottom face
	4, 5, 1, 1, 0, 4
};

static std::vector<char> ReadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

Renderer::Renderer(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPools();
	ChooseSharingMode();
	CreateTextureImage();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSyncObjects();
}

Renderer::~Renderer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();

	vkDeviceWaitIdle(vkDevice);

	vkDestroyImage(vkDevice, m_textureImage, nullptr);
	vkFreeMemory(vkDevice, m_textureImageMemory, nullptr);

	vkFreeDescriptorSets(vkDevice, m_descriptorPool, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data());
	vkDestroyDescriptorPool(vkDevice, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_descriptorSetLayout, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(vkDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(vkDevice, m_uniformBuffersMemory[i], nullptr);
	}

	vkDestroyBuffer(vkDevice, m_indexBuffer, nullptr);
	vkFreeMemory(vkDevice, m_indexBufferMemory, nullptr);

	vkDestroyBuffer(vkDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(vkDevice, m_vertexBufferMemory, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(vkDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(vkDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(vkDevice, m_inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(vkDevice, m_commandPool, nullptr);
	vkDestroyCommandPool(vkDevice, m_transferCommandPool, nullptr);

	vkDestroyPipeline(vkDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(vkDevice, m_pipelineLayout, nullptr);
}

void Renderer::Update()
{
	UpdateMVP(m_currentFrame);
}

void Renderer::Render()
{
	const auto vkDevice = m_pDevice->GetVkDevice();
	const auto swapchain = m_pDevice->GetSwapchain()->GetVkSwapChain();

	vkWaitForFences(vkDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vkDevice, swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_pDevice->GetSwapchain()->RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swapchain image");
	}

	vkResetFences(vkDevice, 1, &m_inFlightFences[m_currentFrame]);

	vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

	RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_pDevice->GetQueue(QueueType::GRAPHICS), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(m_pDevice->GetQueue(QueueType::PRESENT), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		m_pDevice->GetSwapchain()->RecreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swapchain image");
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.pImmutableSamplers = nullptr;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descSetCreateInfo{};
	descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetCreateInfo.bindingCount = 1;
	descSetCreateInfo.pBindings = &layoutBinding;
	
	if (vkCreateDescriptorSetLayout(m_pDevice->GetVkDevice(), &descSetCreateInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void Renderer::CreateGraphicsPipeline()
{
	const auto vertShaderCode = ReadFile("../Engine/shaders/vert.spv");
	const auto fragShaderCode = ReadFile("../Engine/shaders/frag.spv");

	const VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	const VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	const auto& extent = m_pDevice->GetSwapchain()->GetExtent();

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(m_pDevice->GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = *m_pDevice->GetSwapchain()->GetMainRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(m_pDevice->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	vkDestroyShaderModule(m_pDevice->GetVkDevice(), vertShaderModule, nullptr);
	vkDestroyShaderModule(m_pDevice->GetVkDevice(), fragShaderModule, nullptr);
}

void Renderer::CreateCommandPools()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_pDevice->GetPhysicalDevice(), m_pDevice->GetSurface());

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();

	if (vkCreateCommandPool(m_pDevice->GetVkDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
#
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_transferFamily.value();

	if (vkCreateCommandPool(m_pDevice->GetVkDevice(), &poolInfo, nullptr, &m_transferCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}

void Renderer::CreateTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("../Engine/textures/statue.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture image");
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

	void* data;
	vkMapMemory(m_pDevice->GetVkDevice(), stagingMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(m_pDevice->GetVkDevice(), stagingMemory);

	stbi_image_free(pixels);

	VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, usageFlags, memFlags, m_textureImage, m_textureImageMemory);

	TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_pDevice->GetVkDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_pDevice->GetVkDevice(), stagingMemory, nullptr);
}

void Renderer::CreateVertexBuffer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	const VkBufferUsageFlags stagingUsageFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	const VkMemoryPropertyFlags stagingMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	CreateBuffer(bufferSize, stagingUsageFlag, stagingMemFlags, stagingBuffer, stagingBufferMemory);
	
	void* data;
	vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<uint32_t>(bufferSize));
	vkUnmapMemory(vkDevice, stagingBufferMemory);

	const VkBufferUsageFlags deviceLocalUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	const VkMemoryPropertyFlags deviceLocalMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CreateBuffer(bufferSize, deviceLocalUsageFlag, deviceLocalMemFlags, m_vertexBuffer, m_vertexBufferMemory);

	CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
}

void Renderer::CreateIndexBuffer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();

	const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	const VkBufferUsageFlags stagingUsageFlag = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	const VkMemoryPropertyFlags stagingMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	CreateBuffer(bufferSize, stagingUsageFlag, stagingMemFlags, stagingBuffer, stagingMemory);

	void* data;
	vkMapMemory(vkDevice, stagingMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(vkDevice, stagingMemory);

	const VkBufferUsageFlags deviceLocalUsageFlag = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	const VkMemoryPropertyFlags deviceLocalMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	CreateBuffer(bufferSize, deviceLocalUsageFlag, deviceLocalMemFlags, m_indexBuffer, m_indexBufferMemory);

	CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(vkDevice, stagingMemory, nullptr);
}

void Renderer::CreateUniformBuffers()
{
	auto bufferSize = sizeof(MVP);

	m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	m_mappedUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memoryFlags, m_uniformBuffers[i], m_uniformBuffersMemory[i]);

		vkMapMemory(m_pDevice->GetVkDevice(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_mappedUniformBuffers[i]);
	}
}

void Renderer::CreateCommandBuffers()
{
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(m_pDevice->GetVkDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}
}

void Renderer::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_pDevice->GetVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_pDevice->GetVkDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_pDevice->GetVkDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphore/fence");
		}
	}
}

void Renderer::CreateDescriptorPool()
{	
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &poolSize;
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

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_pDevice->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
	}
}

void Renderer::ChooseSharingMode()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_pDevice->GetPhysicalDevice(), m_pDevice->GetSurface());

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

void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(m_pDevice->GetVkDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_pDevice->GetVkDevice(), image, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = m_pDevice->FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_pDevice->GetVkDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory");
	}

	vkBindImageMemory(m_pDevice->GetVkDevice(), image, imageMemory, 0);
}

void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	assert(size > 0 && "Invalid buffer size");

	auto vkDevice = m_pDevice->GetVkDevice();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = m_sharingMode;
	bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_queueSetIndices.size());
	bufferInfo.pQueueFamilyIndices = m_queueSetIndices.data();

	if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_pDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
}

void Renderer::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}

void Renderer::UpdateMVP(const int currentImage)
{
	assert(currentImage < MAX_FRAMES_IN_FLIGHT && "Current frame value is higher than the amount of frames in flight");

	const auto cameraEntity = Core::engine.GetRegistry().view<Transform>().front();
	auto& cameraTransform = Core::engine.GetRegistry().get<Transform>(cameraEntity);
	const auto& camera = Core::engine.GetRegistry().get<Camera>(cameraEntity);

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	Transform cubeTransform{};
	cubeTransform.SetTranslation(glm::vec3(1.f, 2.f, 5.f));

	MVP ubo{};
	ubo.model = glm::rotate(cubeTransform.World(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	const glm::vec3 trans = cameraTransform.GetTranslation();
	const glm::quat rot = cameraTransform.GetRotation();

	const glm::vec3 localForward = glm::vec3(0.f, 0.f, 1.f);
	const glm::vec3 forward = glm::normalize(glm::rotate(rot, localForward));

	const glm::vec3 focusPoint = trans + forward;
	const glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);

	ubo.view = glm::lookAtRH(trans, focusPoint, worldUp);
	ubo.projection = camera.projection;

	memcpy(m_mappedUniformBuffers[currentImage], &ubo, sizeof(ubo));
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

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	const auto swapchain = m_pDevice->GetSwapchain();
	const auto& extent = swapchain->GetExtent();
	const auto& frameBuffers = swapchain->GetFrameBuffers();

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = *swapchain->GetMainRenderPass();
	renderPassInfo.framebuffer = frameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	VkBuffer vertexBuffers[] = { m_vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

VkCommandBuffer Renderer::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_pDevice->GetVkDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_pDevice->GetQueue(QueueType::GRAPHICS), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_pDevice->GetQueue(QueueType::GRAPHICS));

	vkFreeCommandBuffers(m_pDevice->GetVkDevice(), m_commandPool, 1, &commandBuffer);
}

void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::runtime_error("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(commandBuffer, 
		sourceStage, destStage, 
		0, 
		0, nullptr, 
		0, nullptr, 
		1, &barrier);

	EndSingleTimeCommands(commandBuffer);
}
