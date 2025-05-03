#pragma once

#include "vkCommon.h"
#include "vkDevice.h"

class Renderer
{
public:
	Renderer(std::shared_ptr<Device> device);
	~Renderer();

	void Update();
	void Render();

private:
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void LoadModel();

	void ChooseSharingMode();

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void UpdateMVP(const int currentFrame);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	bool HasStencilComponent(VkFormat format);

	std::shared_ptr<Device> m_pDevice;
	
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_mappedUniformBuffers;

	VkImage m_textureImage;
	VkDeviceMemory m_textureImageMemory;
	VkImageView m_textureImageView;
	VkSampler m_textureSampler;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;

	std::vector<VkSemaphore> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores{};
	std::vector<VkFence> m_inFlightFences{};

	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_commandBuffers{};

	uint32_t m_currentFrame = 0;

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;

	const int m_amountOfCubes = 2;
};