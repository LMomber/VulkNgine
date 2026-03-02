#pragma once

#include "vkCommon.h"
#include "vkDevice.h"

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
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateTextureSampler(VkSampler* sampler, VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 useAnisotropy);
	void CreateUniformBuffers();
	void CreateSyncObjects();
	void CreateDescriptorPool();

	void ChooseSharingMode();

	void UpdateMVP(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const;

	std::shared_ptr<Device> m_pDevice;

	VkDescriptorSetLayout m_descriptorSetLayout;

	std::shared_ptr<Pipeline> m_pipeline;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VmaAllocation> m_uniformAllocations;
	std::vector<void*> m_mappedUniformBuffers;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkSemaphore> m_renderFinishedPerImage;

	std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frameContexts{};

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;
};