#pragma once

#include "vkCommon.h"
#include "vkDevice.h"
#include "vkData.h"

#include "sceneObject.h"
#include "transform.h"

#include "textureResolver.h"

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
	void CreateDescriptorSets(RID texture);

	RID CreateGpuTexture(const MaterialTexture& srcTexture, VkFormat format, VkImageAspectFlags aspectFlags, VkImageUsageFlagBits usageFlags, VmaMemoryUsage memoryFlags, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);
	void DestroyGpuTexture(const Vulkan::Texture& texture);

	VkFormat GetVkFormat(TextureFormat format) const;
	void CreateTextureImage(const MaterialTexture& srcTexture, Vulkan::Texture& dstTexture, VkImageUsageFlagBits flags, VmaMemoryUsage memoryFlag, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	void ChooseSharingMode();

	// VMA
	void CreateBuffer(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, VkBufferUsageFlags bufferUsageFlags, VmaMemoryUsage memoryUsageFlags);

	template <typename T>
	void CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, std::vector<T>& bufferData, VkBufferUsageFlags usageFlag);
	void CreateBufferWithStaging(VkDeviceSize size, VkBuffer& buffer, VmaAllocation& allocation, void* bufferData, VkBufferUsageFlags usageFlag);

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags ImageUsageFlags, VmaMemoryUsage memoryUsageFlags, VkImage& image, VmaAllocation& imageAllocation) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
	//

	void UpdateMVP(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(CommandBuffer commandBuffer, uint32_t imageIndex) const;
	CommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(CommandBuffer commandBuffer) const;

	std::shared_ptr<Device> m_pDevice;

	VkDescriptorSetLayout m_descriptorSetLayout;

	std::shared_ptr<Pipeline> m_pipeline;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VmaAllocation> m_uniformAllocations;
	std::vector<void*> m_mappedUniformBuffers;

	std::vector<Vulkan::RenderObject> m_objectsToRender{};

	VkSampler m_linearRepeatAnisoSampler;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkSemaphore> m_renderFinishedPerImage;

	uint32_t m_currentFrame = 0;

	std::array<FrameContext, MAX_FRAMES_IN_FLIGHT> m_frameContexts{};

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;

	RID_Owner<Vulkan::Texture> m_textureOwner;
	RID_Owner<Vulkan::Mesh> m_meshOwner;
};