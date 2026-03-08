#pragma once

#include "vkCommon.h"
#include "vkDevice.h"

#include "vkData.h"

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
	void CreateUniformBuffers();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateFallbackMaterial();

	void ChooseSharingMode();

	void UpdateMVP(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const;

	std::shared_ptr<Device> m_pDevice;

	Vulkan::Material fallbackMaterial;
	std::vector<Vulkan::Model> modelsToRender;

	VkDescriptorSetLayout m_uboDescriptorSetLayout;
	VkDescriptorSetLayout m_materialDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_uboDescriptorSets;

	std::shared_ptr<Pipeline> m_pipeline;

	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers;
	std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_uniformAllocations;
	std::array<void*, MAX_FRAMES_IN_FLIGHT> m_mappedUniformBuffers;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkSemaphore> m_renderFinishedPerImage;

	std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frameContexts{};

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;
};