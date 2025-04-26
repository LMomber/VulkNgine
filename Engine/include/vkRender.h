#pragma once

#include "vkCommon.h"
#include "vkDevice.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer
{
public:
	Renderer(std::shared_ptr<Device> device);
	~Renderer();

	void Update();
	void Render();

private:
	void CreateGraphicsPipeline();
	void CreateCommandPools();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void ChooseSharingMode();

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	std::shared_ptr<Device> m_pDevice;
	
	/*VkRenderPass m_mainRenderPass{};*/
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VkCommandPool m_commandPool{};
	VkCommandPool m_transferCommandPool{};

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	std::vector<VkCommandBuffer> m_commandBuffers{};
	std::vector<VkSemaphore> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores{};
	std::vector<VkFence> m_inFlightFences{};

	uint32_t m_currentFrame = 0;

	std::vector<uint32_t> m_queueSetIndices;
	VkSharingMode m_sharingMode;
};