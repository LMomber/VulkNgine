#pragma once

#include "vkCommon.h"
#include "vkDevice.h"

#include "vkData.h"

#include "sceneObject.h"

#pragma warning(push, 0)
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

struct FrameContext
{
	void Init(std::shared_ptr<Device> device);
	void Destroy(std::shared_ptr<Device> device) const;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_timelineSemaphore;
	uint64_t m_timelineValue;
};

struct SceneData
{
	std::vector<Vulkan::Model> m_modelsToRender;
};

class CommandBuffer;
class Pipeline;
class Renderer
{
public:
	Renderer(std::shared_ptr<Device> device);
	~Renderer();

	void Update();
	void Render();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
private:
	void CreateMaterialDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateBuffers();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateFallbackMaterial();

	void ChooseSharingMode();

	void UpdateBuffers(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const;

	void AddNodeTreeToSceneData(const Node& root);

	std::shared_ptr<Device> m_pDevice;
	std::shared_ptr<Pipeline> m_pipeline;

	SceneData m_sceneData;

	// PBR materials
	VkDescriptorSetLayout m_materialDescriptorSetLayout;
	Vulkan::Material fallbackMaterial;

	// Scene buffers
	VkDescriptorSetLayout m_sceneBuffersDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_sceneBuffersDescriptorSets;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_cameraBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_cameraAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedCameraBuffers;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_objectBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_objectAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedObjectBuffers;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_lightBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_lightAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedLightBuffers;
	//

	VkDescriptorPool m_descriptorPool;
	std::vector<VkSemaphore> m_renderFinishedPerImage;

	std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frameContexts{};

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;

	std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> m_lightCounts{ 0 };
};