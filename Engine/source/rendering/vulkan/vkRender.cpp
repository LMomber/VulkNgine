#include "vkRender.h"

#include "importer.h"
#include "engine.h"
#include "transform.h"
#include "renderComponents.h"
#include "textureResolver.h"

#include "vkPhysicalDevice.h"
#include "vkQueue.h"
#include "vkPipeline.h"
#include "vkPipelineCache.h"

#include "vkData.h"
#include "vkResourceManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdexcept>
#include <array>
#include <set>
#include <unordered_map>

struct MVP
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};

Renderer::Renderer(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	ChooseSharingMode();
	CreateDescriptorPool();
	CreateUniformBuffers();

	CreateMaterialDescriptorSetLayout();
	CreateFallbackMaterial();

	Node root{ Object(ObjectType::TYPE_ROOT) };
	aiScene* pScene = nullptr;
	const std::string modelDir = "../Engine/models/DamagedHelmet.glb";
	//const std::string modelDir = "../Engine/models/viking_room.obj";
	const std::string textureDir = "../Engine/textures/viking_room.png";
	Importer::ImportScene(modelDir, root, pScene);

	const Object* meshObj = nullptr;
	Node* child = &root;

	do
	{
		const Object& obj = child->GetObject();
		if (obj.GetType() == ObjectType::TYPE_MESH)
		{
			meshObj = &obj;
			break;
		}
		else
		{
			child = child->GetChildren()[0].get();
			continue;
		}

	} while (true);

	if (!meshObj)
	{
		throw std::logic_error("No mesh found in the scene");
	}

	const CPU::Model& model = AssetStorage::Get().GetMesh(meshObj->GetHandle());

	if (model.m_meshes.size() > 1)
	{
		throw std::logic_error("this shouldn't be");
	}

	modelsToRender.emplace_back();

	modelsToRender[0].m_material.m_albedoTexture = m_pDevice->CreateGpuTexture(model.m_meshes[0].GetMaterial().GetTexture(CPU::TextureSemantic::Albedo), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	modelsToRender[0].m_material.m_normalTexture = m_pDevice->CreateGpuTexture(model.m_meshes[0].GetMaterial().GetTexture(CPU::TextureSemantic::Normal), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	modelsToRender[0].m_material.m_metallicRoughnessTexture = m_pDevice->CreateGpuTexture(model.m_meshes[0].GetMaterial().GetTexture(CPU::TextureSemantic::MetallicRoughness), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	modelsToRender[0].m_material.m_occlusionTexture = m_pDevice->CreateGpuTexture(model.m_meshes[0].GetMaterial().GetTexture(CPU::TextureSemantic::AO), VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	modelsToRender[0].m_material.m_emissiveTexture = m_pDevice->CreateGpuTexture(model.m_meshes[0].GetMaterial().GetTexture(CPU::TextureSemantic::Emissive), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	modelsToRender[0].m_material.CreateDescriptorSet(m_pDevice->GetVkDevice(), m_materialDescriptorSetLayout, m_pDevice->GetSampler(SamplerType::LinearRepeatAnisotropic), m_descriptorPool, fallbackMaterial);

	for (const auto& mesh : model.m_meshes)
	{
		modelsToRender[0].m_meshes.push_back(m_pDevice->CreateGpuMesh(mesh));
	}

	CreateGraphicsPipeline();
	CreateSyncObjects();
}

Renderer::~Renderer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();

	vkDeviceWaitIdle(vkDevice);

	PipelineCache::Reset();
	ShaderCache::Reset();

	for (int i = 0; i < modelsToRender.size(); ++i)
	{
		vkFreeDescriptorSets(vkDevice, m_descriptorPool, static_cast<uint32_t>(modelsToRender[i].m_material.m_descriptorSets.size()), modelsToRender[i].m_material.m_descriptorSets.data());
	}
	vkDestroyDescriptorPool(vkDevice, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_uboDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_materialDescriptorSetLayout, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_uniformBuffers[i], m_uniformAllocations[i]);
	}

	const auto& rm = Vulkan::ResourceManager::Get();

	rm.meshOwner.FreeAll([this](Vulkan::Mesh& mesh) {
		m_pDevice->DestroyGpuMesh(mesh);
		});

	rm.textureOwner.FreeAll([this](Vulkan::Texture& tex) {
		m_pDevice->DestroyGpuTexture(tex);
		});

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
	UpdateMVP(m_pDevice->GetCurrentFrame());
}

void Renderer::Render()
{
	uint32_t currentFrame = m_pDevice->GetCurrentFrame();
	FrameContext& frame = m_frameContexts[currentFrame];
	VkDevice device = m_pDevice->GetVkDevice();
	VkQueue graphicsQueue = m_pDevice->GetQueue()->GetQueue(QueueType::GRAPHICS);
	VkQueue presentQueue = m_pDevice->GetQueue()->GetQueue(QueueType::PRESENT);
	VkSwapchainKHR swapchain = m_pDevice->GetSwapchain()->GetVkSwapChain();

	// Wait on previous frame
	// Use timeline semaphore as device-host synchronization
	VkSemaphoreWaitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &frame.m_timelineSemaphore;
	waitInfo.pValues = &frame.m_timelineValue;
	waitInfo.flags = 0;

	vkWaitSemaphores(device, &waitInfo, UINT64_MAX);

	// Acquire next swapchain image
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

	// Record draw commands
	m_pDevice->GetQueue()->ResetCommandPools(currentFrame);
	CommandBuffer commandBuffer =
		m_pDevice->GetQueue()->GetOrCreateCommandBuffer(QueueType::GRAPHICS, currentFrame);
	const VkCommandBuffer* pVkCommandBuffer = commandBuffer.GetVkPtr();

	RecordCommandBuffer(commandBuffer, imageIndex);

	// Submit to queue
	uint64_t signalValue = ++frame.m_timelineValue;

	VkTimelineSemaphoreSubmitInfo timelineInfo{};
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.signalSemaphoreValueCount = 2;
	timelineInfo.pSignalSemaphoreValues = &signalValue;

	VkSemaphore waitSemaphores[] = { frame.m_imageAvailableSemaphore };
	VkSemaphore signalSemaphores[] = { frame.m_timelineSemaphore, m_renderFinishedPerImage[imageIndex] };
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

	// Present
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

	m_pDevice->GetQueue()->ResetCommandBufferIndices(currentFrame);

	// Advance to next frame
	m_pDevice->AdvanceCurrentFrame();
}

void Renderer::CreateMaterialDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding albedoLayoutBinding{};
	albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	albedoLayoutBinding.binding = 0;
	albedoLayoutBinding.descriptorCount = 1;
	albedoLayoutBinding.pImmutableSamplers = nullptr;
	albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding normalLayoutBinding{};
	normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	normalLayoutBinding.binding = 1;
	normalLayoutBinding.descriptorCount = 1;
	normalLayoutBinding.pImmutableSamplers = nullptr;
	normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding metallicRougnessLayoutBinding{};
	metallicRougnessLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	metallicRougnessLayoutBinding.binding = 2;
	metallicRougnessLayoutBinding.descriptorCount = 1;
	metallicRougnessLayoutBinding.pImmutableSamplers = nullptr;
	metallicRougnessLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding occlusionLayoutBinding{};
	occlusionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	occlusionLayoutBinding.binding = 3;
	occlusionLayoutBinding.descriptorCount = 1;
	occlusionLayoutBinding.pImmutableSamplers = nullptr;
	occlusionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding emissiveLayoutBinding{};
	emissiveLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	emissiveLayoutBinding.binding = 4;
	emissiveLayoutBinding.descriptorCount = 1;
	emissiveLayoutBinding.pImmutableSamplers = nullptr;
	emissiveLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 5> bindings = { 
		albedoLayoutBinding, 
		normalLayoutBinding, 
		metallicRougnessLayoutBinding, 
		occlusionLayoutBinding, 
		emissiveLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 5;
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->GetVkDevice(), &layoutInfo, nullptr, &m_materialDescriptorSetLayout) != VK_SUCCESS)
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
	bindingDescriptions.push_back(Vulkan::Vertex::GetBindingDescription());
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vulkan::Vertex::GetAttributeDescriptions();

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
	layouts.push_back(m_uboDescriptorSetLayout);
	layouts.push_back(m_materialDescriptorSetLayout);

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

void Renderer::CreateUniformBuffers()
{
	// Create buffers
	const auto bufferSize = sizeof(MVP);

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

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding uniformLayoutBinding{};
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.pImmutableSamplers = nullptr;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uniformLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_pDevice->GetVkDevice(), &layoutInfo, nullptr, &m_uboDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

	// Create descriptor set
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_uboDescriptorSets.emplace_back();
	}

	m_pDevice->CreateDescriptorSets(m_uboDescriptorSets, m_uboDescriptorSetLayout, m_descriptorPool);

	// Write to descriptor set
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<VkWriteDescriptorSet> writes{};
		writes.emplace_back();

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MVP);

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = m_uboDescriptorSets[i];
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &bufferInfo;
		writes[0].pImageInfo = NULL;
		writes[0].pTexelBufferView = NULL;

		m_pDevice->UpdateDescriptorSets(writes);
	}
}

void Renderer::CreateSyncObjects()
{
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
	const uint16_t maxMaterials = 1000;

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * maxMaterials;

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * maxMaterials;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(m_pDevice->GetVkDevice(), &createInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void Renderer::CreateFallbackMaterial()
{
	auto albedo = std::make_shared<CPU::MaterialTexture>(
		CPU::TextureSemantic::Albedo,
		0,
		1,
		1,
		4,
		std::vector<uint8_t>{255, 255, 255, 255},
		CPU::TextureFormat::SRGBA8,
		std::string()
	);

	auto normal = std::make_shared<CPU::MaterialTexture>(
		CPU::TextureSemantic::Normal,
		0,
		1,
		1,
		3,
		std::vector<uint8_t>{128, 128, 255, 255},
		CPU::TextureFormat::RGBA8_UNORM,
		std::string()
	);

	auto roughnessMetallic = std::make_shared<CPU::MaterialTexture>(
		CPU::TextureSemantic::MetallicRoughness,
		0,
		1,
		1,
		2,
		std::vector<uint8_t>{255, 0, 0, 255},
		CPU::TextureFormat::RG8_UNORM,
		std::string()
	);

	auto ambientOcclusion = std::make_shared<CPU::MaterialTexture>(
		CPU::TextureSemantic::AO,
		0,
		1,
		1,
		1,
		std::vector<uint8_t>{255, 255, 255, 255},
		CPU::TextureFormat::R8_UNORM,
		std::string()
	);

	auto emissive = std::make_shared<CPU::MaterialTexture>(
		CPU::TextureSemantic::Emissive,
		0,
		1,
		1,
		3,
		std::vector<uint8_t>{0, 0, 0, 255},
		CPU::TextureFormat::SRGBA8,
		std::string()
	);

	CPU::Material cpuMat;
	cpuMat.SetTexture(albedo, CPU::TextureSemantic::Albedo);
	cpuMat.SetTexture(normal, CPU::TextureSemantic::Normal);
	cpuMat.SetTexture(roughnessMetallic, CPU::TextureSemantic::MetallicRoughness);
	cpuMat.SetTexture(ambientOcclusion, CPU::TextureSemantic::AO);
	cpuMat.SetTexture(emissive, CPU::TextureSemantic::Emissive);

	fallbackMaterial.m_albedoTexture = m_pDevice->CreateGpuTexture(cpuMat.GetTexture(CPU::TextureSemantic::Albedo), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	fallbackMaterial.m_normalTexture = m_pDevice->CreateGpuTexture(cpuMat.GetTexture(CPU::TextureSemantic::Normal), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	fallbackMaterial.m_metallicRoughnessTexture = m_pDevice->CreateGpuTexture(cpuMat.GetTexture(CPU::TextureSemantic::MetallicRoughness), VK_FORMAT_R8G8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	fallbackMaterial.m_occlusionTexture = m_pDevice->CreateGpuTexture(cpuMat.GetTexture(CPU::TextureSemantic::AO), VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	fallbackMaterial.m_emissiveTexture = m_pDevice->CreateGpuTexture(cpuMat.GetTexture(CPU::TextureSemantic::Emissive), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

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
	auto world = glm::rotate(transform.GetWorld(), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
	const auto depthFormat = swapchain->GetDepthFormat();
	VkImage swapchainImage = swapchain->GetImages()[imageIndex];
	VkImage depthImage = swapchain->GetDepthImages()[imageIndex];

	// 1. Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
	commandBuffer.TransitionImageLayout(swapchainImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	commandBuffer.TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

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
	depthAttachment.imageView = swapchain->GetDepthViews()[imageIndex];
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

	for (uint32_t i = 0; i < modelsToRender.size(); i++)
	{
		for (uint32_t j = 0; j < modelsToRender[i].m_meshes.size(); j++)
		{
			const Vulkan::Mesh* pMesh = Vulkan::ResourceManager::Get().meshOwner.GetOrNull(modelsToRender[i].m_meshes[j]);
			assert(pMesh && "Mesh given by RID is null");

			VkBuffer vertexBuffers[] = { pMesh->m_vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			commandBuffer.BindVertexBuffers(vertexBuffers, offsets);
			commandBuffer.BindIndexBuffer(pMesh->m_indexBuffer, VK_INDEX_TYPE_UINT32);

			const int descriptorSetIndex = m_pDevice->GetCurrentFrame();
			const std::array<VkDescriptorSet, 2> sets{ m_uboDescriptorSets[descriptorSetIndex], modelsToRender[i].m_material.m_descriptorSets[descriptorSetIndex] };
			commandBuffer.BindDescriptorSets(m_pipeline->GetLayout(), sets.data(), 0, 2);

			commandBuffer.DrawIndexed(static_cast<uint32_t>(pMesh->m_indices.size()));
		}
	}

	commandBuffer.EndRendering();

	// 3. Transition swapchain image back to PRESENT_SRC_KHR
	commandBuffer.TransitionImageLayout(swapchainImage, imageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	commandBuffer.EndCommandBuffer();
}

void FrameContext::Init(std::shared_ptr<Device> device)
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device->GetVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS)
	{
		throw std::logic_error("Failed to create semaphore for FrameContext");
	}

	VkSemaphoreTypeCreateInfo timelineCreateInfo{};
	timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = &timelineCreateInfo;

	if (vkCreateSemaphore(device->GetVkDevice(), &createInfo, nullptr, &m_timelineSemaphore) != VK_SUCCESS)
	{
		throw std::logic_error("Failed to create timeline semaphore for FrameContext");
	}
}

void FrameContext::Destroy(std::shared_ptr<Device> device) const
{
	if (m_imageAvailableSemaphore) vkDestroySemaphore(device->GetVkDevice(), m_imageAvailableSemaphore, nullptr);
	if (m_timelineSemaphore) vkDestroySemaphore(device->GetVkDevice(), m_timelineSemaphore, nullptr);
}