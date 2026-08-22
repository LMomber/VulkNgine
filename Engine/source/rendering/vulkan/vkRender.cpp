#include "vkRender.h"

//#include "importer.h"
#include "engine.h"
#include "transform.h"
#include "renderComponents.h"
#include "textureResolver.h"

#include "../../core/assetStorage.h"

#include "vkPhysicalDevice.h"
#include "vkQueue.h"
#include "vkPipeline.h"
#include "vkPipelineCache.h"

#include "vkData.h"
#include "vkResourceManager.h"
#include "vkModelStorage.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "imgui/imgui_impl_vulkan.h"

#include <stdexcept>
#include <array>
#include <set>
#include <unordered_map>
#include <unordered_set>

struct CameraUBO
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec4 position;
};
static_assert(sizeof(CameraUBO) == 144);

struct ObjectParameters
{
	glm::mat4 model;
};
static_assert(sizeof(ObjectParameters) == 64);

struct ObjectBuffer
{
	ObjectParameters objects[MAX_MODELS_IN_SCENE];
} objectData;

struct LightParameters
{
	glm::vec4 position; // vec3 with padding
	glm::vec4 intensity; // rgb = color, w = intensity
};
static_assert(sizeof(LightParameters) == 32);

struct LightBuffer
{
	uint32_t lightCount;
	uint32_t padding[3]; // Padding for 16 byte boundary
	LightParameters lights[MAX_LIGHTS_IN_SCENE];
} lightData;

Renderer::Renderer(std::shared_ptr<Device> device) :
	m_pDevice(device)
{
	ChooseSharingMode();
	CreateBuffers();

	CreateMaterialDescriptorSetLayout();
	CreateFallbackMaterial();

	// Hard coded for now.
	lightData.lights[lightData.lightCount] = { glm::vec4(-2.f, 3.f, 0.f, 0.f), glm::vec4(1.f, 1.f, 1.f, 10.f) };
	lightData.lightCount++;
	/*lightData.lights[lightData.lightCount] = { glm::vec4(-2.f, 3.f, 4.f, 0.f), glm::vec4(1.f, 1.f, 1.f, 10.f) };
	lightData.lightCount++;
	lightData.lights[lightData.lightCount] = { glm::vec4(2.f, 1.f, 0.f, 0.f), glm::vec4(1.f, 1.f, 1.f, 10.f) };
	lightData.lightCount++;*/

	CreateGraphicsPipeline();
	CreateSyncObjects();

	INIT_WRAPPER("editor UI", m_pEditorUI = std::make_shared<EditorUI>(m_pDevice));

	CreateRenderTargets();
}

Renderer::~Renderer()
{
	const auto vkDevice = m_pDevice->GetVkDevice();

	vkDeviceWaitIdle(vkDevice);

	PipelineCache::Reset();
	ShaderCache::Reset();

	std::unordered_set<size_t> hashes;
	for (int i = 0; i < m_sceneRenderData.m_meshesToRender.size(); ++i)
	{
		auto& model = m_sceneRenderData.m_meshesToRender[i];
		if (hashes.insert(model.m_hash).second && !model.m_isInvisible)
		{
			vkFreeDescriptorSets(vkDevice, m_pDevice->GetDescriptorPool(), static_cast<uint32_t>(model.m_material.m_descriptorSets.size()),
				model.m_material.m_descriptorSets.data());
		}
	}
	vkDestroyDescriptorPool(vkDevice, m_pDevice->GetDescriptorPool(), nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_sceneBuffersDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(vkDevice, m_materialDescriptorSetLayout, nullptr);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_cameraBuffers[i], m_cameraAllocations[i]);
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_objectBuffers[i], m_objectAllocations[i]);
		vmaDestroyBuffer(m_pDevice->GetAllocator(), m_lightBuffers[i], m_lightAllocations[i]);
	}

	DestroyRenderTargets();

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
	uint32_t currentImage = m_pDevice->GetCurrentFrame();

	// Camera
	const auto cameraEntity = Core::engine.GetRegistry().view<Transform, Camera>().front();
	auto& cameraTransform = Core::engine.GetRegistry().get<Transform>(cameraEntity);
	const auto& camera = Core::engine.GetRegistry().get<Camera>(cameraEntity);

	const glm::vec3 trans = cameraTransform.GetTranslation();
	const glm::quat rot = cameraTransform.GetRotation();

	const glm::vec3 localForward = glm::vec3(0.f, 0.f, 1.f);
	const glm::vec3 forward = glm::normalize(glm::rotate(rot, localForward));

	const glm::vec3 focusPoint = trans + forward;
	const glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);

	CameraUBO cameraUBO{};
	cameraUBO.view = glm::lookAtRH(trans, focusPoint, worldUp);
	cameraUBO.projection = camera.projection;

	memcpy(m_mappedCameraBuffers[currentImage], &cameraUBO, sizeof(CameraUBO));

	UpdateBuffers(currentImage);
}

void Renderer::Render()
{
	int width, height;
	GLFWwindow* pWindow = m_pDevice->GetWindow();
	glfwGetFramebufferSize(pWindow, &width, &height);

	if (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(pWindow, &width, &height);
		glfwWaitEvents();
		return;
	}

	VkExtent2D extent = m_pDevice->GetSwapchain()->GetExtent();
	if (static_cast<uint32_t>(width) != extent.width || static_cast<uint32_t>(height) != extent.height)
	{
		m_pDevice->GetSwapchain()->RecreateSwapchain();
		DestroyRenderTargets();
		CreateRenderTargets();
		return;
	}

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

	uint32_t imageIndex = m_pDevice->GetSwapchain()->AcquireNextImage(frame.m_imageAvailableSemaphore);

	// If the swapchain is recreated, skip rendering
	if (imageIndex == std::numeric_limits<uint32_t>().max())
	{
		return;
	}

	// Record main command buffer
	m_pDevice->GetQueue()->ResetCommandPools(currentFrame);
	CommandBuffer mainCmdBuffer =
		m_pDevice->GetQueue()->GetOrCreateCommandBuffer(QueueType::GRAPHICS, currentFrame);
	RecordCommandBuffer(mainCmdBuffer, imageIndex);

	// Record imgui command buffer
	CommandBuffer imguiCmdBuffer =
		m_pDevice->GetQueue()->GetOrCreateCommandBuffer(QueueType::GRAPHICS, currentFrame);
	m_pEditorUI->Render(&imguiCmdBuffer, m_renderTargets[imageIndex]);

	// Submit to queue
	std::vector<VkCommandBuffer> commandBuffers{};
	commandBuffers.push_back(*mainCmdBuffer.GetVkPtr());
	commandBuffers.push_back(*imguiCmdBuffer.GetVkPtr());

	std::vector<VkSemaphore> waitSemaphores;
	waitSemaphores.push_back(frame.m_imageAvailableSemaphore);

	std::vector<VkSemaphore> signalSemaphores;
	signalSemaphores.push_back(frame.m_timelineSemaphore);
	signalSemaphores.push_back(m_renderFinishedPerImage[imageIndex]);

	uint64_t signalValue = ++frame.m_timelineValue;

	m_pDevice->SubmitToQueue(commandBuffers, waitSemaphores, signalSemaphores, graphicsQueue, signalValue);

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
	layouts.push_back(m_sceneBuffersDescriptorSetLayout);
	layouts.push_back(m_materialDescriptorSetLayout);

	std::vector<VkFormat> imageFormats;
	imageFormats.push_back(m_pDevice->GetSwapchain()->GetImageFormat());

	//setup push constants
	VkPushConstantRange objectIndexConstant{};
	objectIndexConstant.offset = 0;
	objectIndexConstant.size = sizeof(uint32_t);
	objectIndexConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::vector<VkPushConstantRange> pushConstants;
	pushConstants.push_back(objectIndexConstant);

	GraphicsPipelineInfo pipelineInfo{};
	pipelineInfo.SetShader("../Engine/shaders/forwardRenderVert.spv", ShaderType::VERTEX);
	pipelineInfo.SetShader("../Engine/shaders/forwardRenderFrag.spv", ShaderType::FRAGMENT);
	pipelineInfo.SetDynamicStates(dynamicStates);
	pipelineInfo.SetVertexInputState(bindingDescriptions, attributeDescriptions);
	pipelineInfo.SetInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
	pipelineInfo.SetViewportState();
	pipelineInfo.SetRasterizationState(VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineInfo.SetMultisampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);
	pipelineInfo.SetColorBlendState(VK_FALSE, VK_LOGIC_OP_COPY, colorBlendAttachments);
	pipelineInfo.SetLayoutInfo(layouts, pushConstants);
	pipelineInfo.SetDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE);
	pipelineInfo.SetRenderInfo(imageFormats, m_pDevice->GetPhysicalDevice()->FindSupportedFormat(
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT));

	m_pipeline = PipelineCache::GetOrCreateGraphicsPipeline(pipelineInfo);
}

void Renderer::CreateBuffers()
{
	// Create buffers
	const auto cameraBufferSize = sizeof(CameraUBO);
	const auto objectBufferSize = sizeof(ObjectBuffer);
	const auto lightBufferSize = sizeof(LightBuffer);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		// Uniform
		m_pDevice->CreateAndMapBuffer(m_mappedCameraBuffers[i], cameraBufferSize, m_cameraBuffers[i], m_cameraAllocations[i],
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT);

		// Storage
		m_pDevice->CreateAndMapBuffer(m_mappedObjectBuffers[i], objectBufferSize, m_objectBuffers[i], m_objectAllocations[i],
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
		m_pDevice->CreateAndMapBuffer(m_mappedLightBuffers[i], lightBufferSize, m_lightBuffers[i], m_lightAllocations[i],
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
	}

	const int bindingCount = 3;
	std::array<VkDescriptorSetLayoutBinding, bindingCount> bindings{};

	// Camera
	{
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].binding = 0;
		bindings[0].descriptorCount = 1;
		bindings[0].pImmutableSamplers = nullptr;
		bindings[0].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
	}

	// Object
	{
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[1].binding = 1;
		bindings[1].descriptorCount = 1;
		bindings[1].pImmutableSamplers = nullptr;
		bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	}

	// Light
	{
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[2].binding = 2;
		bindings[2].descriptorCount = 1;
		bindings[2].pImmutableSamplers = nullptr;
		bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindingCount;
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->GetVkDevice(), &layoutInfo, nullptr, &m_sceneBuffersDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}

	// Create descriptor set
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_sceneBuffersDescriptorSets.emplace_back();
	}

	m_pDevice->CreateDescriptorSets(m_sceneBuffersDescriptorSets, m_sceneBuffersDescriptorSetLayout, m_pDevice->GetDescriptorPool());

	// Write to descriptor set
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<VkWriteDescriptorSet> writes(bindingCount);

		// Camera buffer
		VkDescriptorBufferInfo cameraBufferInfo = {};
		cameraBufferInfo.buffer = m_cameraBuffers[i];
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = sizeof(CameraUBO);

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = m_sceneBuffersDescriptorSets[i];
		writes[0].dstBinding = 0;
		writes[0].dstArrayElement = 0;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].descriptorCount = 1;
		writes[0].pBufferInfo = &cameraBufferInfo;
		writes[0].pImageInfo = nullptr;
		writes[0].pTexelBufferView = nullptr;

		// Object buffer
		VkDescriptorBufferInfo objectBufferInfo = {};
		objectBufferInfo.buffer = m_objectBuffers[i];
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(ObjectBuffer);

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = m_sceneBuffersDescriptorSets[i];
		writes[1].dstBinding = 1;
		writes[1].dstArrayElement = 0;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[1].descriptorCount = 1;
		writes[1].pBufferInfo = &objectBufferInfo;
		writes[1].pImageInfo = nullptr;
		writes[1].pTexelBufferView = nullptr;

		// Light buffer
		VkDescriptorBufferInfo lightBufferInfo = {};
		lightBufferInfo.buffer = m_lightBuffers[i];
		lightBufferInfo.offset = 0;
		lightBufferInfo.range = sizeof(LightBuffer);

		writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[2].dstSet = m_sceneBuffersDescriptorSets[i];
		writes[2].dstBinding = 2;
		writes[2].dstArrayElement = 0;
		writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[2].descriptorCount = 1;
		writes[2].pBufferInfo = &lightBufferInfo;
		writes[2].pImageInfo = nullptr;
		writes[2].pTexelBufferView = nullptr;

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

void Renderer::CreateRenderTargets()
{
	const auto format = VK_FORMAT_B8G8R8A8_SRGB;
	const auto extent = m_pDevice->GetSwapchain()->GetExtent();

	for (uint16_t i = 0; i < m_renderTargets.size(); i++)
	{
		m_pDevice->CreateImage(extent.width, extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_AUTO, m_renderTargets[i].m_image, m_renderTargets[i].m_allocation);
		m_pDevice->CreateImageView(m_renderTargets[i].m_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_renderTargets[i].m_imageView);
		m_renderTargets[i].m_format = format;
		m_renderTargets[i].m_extent = extent;
		m_renderTargets[i].m_imguiTexture = ImGui_ImplVulkan_AddTexture(m_renderTargets[i].m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void Renderer::DestroyRenderTargets()
{
	for (auto& rt : m_renderTargets)
	{
		Vulkan::Texture texture{};
		texture.m_image = rt.m_image;
		texture.m_imageView = rt.m_imageView;
		texture.m_allocation = rt.m_allocation;

		m_pDevice->DestroyGpuTexture(texture);
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

void Renderer::UpdateBuffers(const int currentImage)
{
	assert(currentImage < MAX_FRAMES_IN_FLIGHT && "Current frame value is higher than the amount of frames in flight");

	// Objects
	if (m_objectBufferUpdate[currentImage])
	{
		memcpy(m_mappedObjectBuffers[currentImage], &objectData, sizeof(ObjectBuffer));
		m_objectBufferUpdate[currentImage] = false;
	}

	// Lights
	if (lightData.lightCount != m_lightCounts[currentImage])
	{
		memcpy(m_mappedLightBuffers[currentImage], &lightData, sizeof(LightBuffer));
		m_lightCounts[currentImage] = lightData.lightCount;
	}
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
	const VkImage image = m_renderTargets[imageIndex].m_image;
	const VkImageView imageView = m_renderTargets[imageIndex].m_imageView;
	const VkFormat imageFormat = m_renderTargets[imageIndex].m_format;
	const auto depthFormat = swapchain->GetDepthFormat();
	const VkImage depthImage = swapchain->GetDepthImages()[imageIndex];
	const VkImageView depthImageView = swapchain->GetDepthViews()[imageIndex];

	// 1. Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL
	commandBuffer.TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	commandBuffer.TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// 2. Begin dynamic rendering
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = imageView;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = { 0.f, 0.f, 0.f, 0.f };

	VkRenderingAttachmentInfo depthAttachment{};
	depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachment.imageView = depthImageView;
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

	for (uint32_t i = 0; i < m_sceneRenderData.m_meshesToRender.size(); i++)
	{
		if (m_sceneRenderData.m_meshesToRender[i].m_isInvisible)
		{
			continue;
		}

		const Vulkan::Mesh* pMesh = Vulkan::ResourceManager::Get().meshOwner.GetOrNull(m_sceneRenderData.m_meshesToRender[i].m_mesh);
		assert(pMesh && "Mesh given by RID is null");

		VkBuffer vertexBuffers[] = { pMesh->m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		commandBuffer.BindVertexBuffers(vertexBuffers, offsets);
		commandBuffer.BindIndexBuffer(pMesh->m_indexBuffer, VK_INDEX_TYPE_UINT32);

		{
			uint32_t objectIndex = i;
			VkPushConstantsInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
			info.layout = m_pipeline->GetLayout();
			info.pValues = &objectIndex;
			info.size = sizeof(uint32_t);
			info.offset = 0;
			info.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			info.pNext = nullptr;
			commandBuffer.BindPushConstants(info);
		}

		const int descriptorSetIndex = m_pDevice->GetCurrentFrame();
		const std::array<VkDescriptorSet, 2> sets{ m_sceneBuffersDescriptorSets[descriptorSetIndex], m_sceneRenderData.m_meshesToRender[i].m_material.m_descriptorSets[descriptorSetIndex] };
		commandBuffer.BindDescriptorSets(m_pipeline->GetLayout(), sets.data(), 0, 2);

		commandBuffer.DrawIndexed(static_cast<uint32_t>(pMesh->m_indices.size()));
	}

	commandBuffer.EndRendering();

	commandBuffer.EndCommandBuffer();
}

std::vector<uint32_t> Renderer::CreateGpuModel(const CPU::Model& model, size_t hash)
{
	// If already exists:
	Vulkan::MeshStorage& meshStorage = Vulkan::MeshStorage::Get();
	if (meshStorage.HasMeshes(hash))
	{
		const Vulkan::RenderData& data = meshStorage.GetMeshes(hash);
		//meshStorage.IncrementUsageCount(hash);

		return AddMeshesToRenderList(data.m_meshes);
	}

	// Else create gpu meshes:
	std::vector<Vulkan::Model> gpuMeshes;
	for (uint16_t i = 0; i < model.m_meshes.size(); i++)
	{
		Vulkan::Model gpuMesh;
		gpuMesh.m_material.m_albedoTexture = m_pDevice->CreateGpuTexture(model.m_meshes[i].GetMaterial().GetTexture(CPU::TextureSemantic::Albedo), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		gpuMesh.m_material.m_normalTexture = m_pDevice->CreateGpuTexture(model.m_meshes[i].GetMaterial().GetTexture(CPU::TextureSemantic::Normal), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		gpuMesh.m_material.m_metallicRoughnessTexture = m_pDevice->CreateGpuTexture(model.m_meshes[i].GetMaterial().GetTexture(CPU::TextureSemantic::MetallicRoughness), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		gpuMesh.m_material.m_occlusionTexture = m_pDevice->CreateGpuTexture(model.m_meshes[i].GetMaterial().GetTexture(CPU::TextureSemantic::AO), VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		gpuMesh.m_material.m_emissiveTexture = m_pDevice->CreateGpuTexture(model.m_meshes[i].GetMaterial().GetTexture(CPU::TextureSemantic::Emissive), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		gpuMesh.m_material.CreateDescriptorSet(m_pDevice->GetVkDevice(), m_materialDescriptorSetLayout, m_pDevice->GetSampler(SamplerType::LinearRepeatAnisotropic), m_pDevice->GetDescriptorPool(), fallbackMaterial);

		const auto& mesh = model.m_meshes[i];
		gpuMesh.m_mesh = m_pDevice->CreateGpuMesh(mesh);
		gpuMesh.m_hash = hash;

		gpuMeshes.push_back(gpuMesh);
	}

	meshStorage.AddMeshes(hash, gpuMeshes);

	return AddMeshesToRenderList(gpuMeshes);
}

void Renderer::FreeGpuModels(const std::vector<uint32_t>& ids, bool shouldBeDestroyed)
{
	for (auto& id : ids)
	{
		auto& model = m_sceneRenderData.m_meshesToRender[id];

		if (shouldBeDestroyed)
		{
			DestroyGpuModel(model);
		}

		m_sceneRenderData.m_meshesToRender[id].m_isInvisible = true;
		m_sceneRenderData.m_freeList.push_back(id);
	}
}

void Renderer::UpdateModelTransform(const std::vector<uint32_t>& ids, const glm::mat4& worldMatrix)
{
	for (uint32_t id : ids)
	{
		objectData.objects[id].model = worldMatrix;
	}
}

void Renderer::QueueObjectBufferUpdate()
{
	for (bool& isUpdate : m_objectBufferUpdate)
	{
		isUpdate = true;
	}
}

const std::vector<uint32_t> Renderer::AddMeshesToRenderList(const std::vector<Vulkan::Model>& meshes)
{
	std::vector<uint32_t> outputVector;
	for (const auto& mesh : meshes)
	{
		uint32_t index;
		if (m_sceneRenderData.m_freeList.empty())
		{
			m_sceneRenderData.m_meshesToRender.emplace_back();
			index = static_cast<uint32_t>(m_sceneRenderData.m_meshesToRender.size()) - 1;
		}
		else
		{
			index = m_sceneRenderData.m_freeList.back();
			m_sceneRenderData.m_meshesToRender[index].m_isInvisible = false;
			m_sceneRenderData.m_freeList.pop_back();
		}

		m_sceneRenderData.m_meshesToRender[index] = mesh;
		outputVector.push_back(index);
	}

	return outputVector;
}

void Renderer::DestroyGpuModel(const Vulkan::Model& model)
{
	const auto& rm = Vulkan::ResourceManager::Get();

	// Textures
	if (model.m_material.m_albedoTexture.GetValue() > -1)
	{
		rm.textureOwner.Free(model.m_material.m_albedoTexture, [this](Vulkan::Texture& tex) {
			m_pDevice->DestroyGpuTexture(tex);
			});
	}
	if (model.m_material.m_emissiveTexture.GetValue() > -1)
	{
		rm.textureOwner.Free(model.m_material.m_emissiveTexture, [this](Vulkan::Texture& tex) {
			m_pDevice->DestroyGpuTexture(tex);
			});
	}
	if (model.m_material.m_metallicRoughnessTexture.GetValue() > -1)
	{
		rm.textureOwner.Free(model.m_material.m_metallicRoughnessTexture, [this](Vulkan::Texture& tex) {
			m_pDevice->DestroyGpuTexture(tex);
			});
	}
	if (model.m_material.m_normalTexture.GetValue() > -1)
	{
		rm.textureOwner.Free(model.m_material.m_normalTexture, [this](Vulkan::Texture& tex) {
			m_pDevice->DestroyGpuTexture(tex);
			});
	}
	if (model.m_material.m_occlusionTexture.GetValue() > -1)
	{
		rm.textureOwner.Free(model.m_material.m_occlusionTexture, [this](Vulkan::Texture& tex) {
			m_pDevice->DestroyGpuTexture(tex);
			});
	}

	// Mesh
	rm.meshOwner.Free(model.m_mesh, [this](Vulkan::Mesh& mesh) {
		m_pDevice->DestroyGpuMesh(mesh);
		});

	// Descriptor Set
	vkFreeDescriptorSets(m_pDevice->GetVkDevice(), m_pDevice->GetDescriptorPool(), static_cast<uint32_t>(model.m_material.m_descriptorSets.size()),
		model.m_material.m_descriptorSets.data());
}