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
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	std::shared_ptr<Device> m_pDevice;
	std::vector<VkFramebuffer> m_framebuffers{};
	
	VkRenderPass m_renderPass{};
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VkCommandPool m_commandPool{};
	VkCommandBuffer m_commandBuffer{};

	VkSemaphore m_imageAvailableSemaphore{};
	VkSemaphore m_renderFinishedSemaphore{};
	VkFence m_inFlightFence{};
};