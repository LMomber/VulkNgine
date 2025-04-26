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
	void CreateCommandPool();
	void CreateVertexBuffer();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	std::shared_ptr<Device> m_pDevice;
	
	/*VkRenderPass m_mainRenderPass{};*/
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VkCommandPool m_commandPool{};

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;

	std::vector<VkCommandBuffer> m_commandBuffers{};
	std::vector<VkSemaphore> m_imageAvailableSemaphores{};
	std::vector<VkSemaphore> m_renderFinishedSemaphores{};
	std::vector<VkFence> m_inFlightFences{};

	uint32_t m_currentFrame = 0;
};