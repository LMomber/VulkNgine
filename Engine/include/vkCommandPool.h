#pragma once

#include "vkCommon.h"

class CommandBuffer;
class CommandPool
{
public:
	CommandPool(VkDevice device, const QueueFamilyIndices& queueFamilyIndices);
	~CommandPool();

	const CommandBuffer& GetOrCreateCommandBuffer(QueueType type, unsigned int currentFrame);
	std::vector<CommandBuffer> GetOrCreateCommandBuffers(QueueType type, unsigned int count, unsigned int currentFrame);

	void ResetCommandBuffers(unsigned int currentFrame);

	CommandPool(const CommandPool&) = delete;
	CommandPool& operator=(const CommandPool&) = delete;

private:
	const CommandBuffer& CreateCommandBuffer(QueueType type, unsigned int currentFrame);
	std::vector<CommandBuffer> CreateCommandBuffers(QueueType type, unsigned int count, unsigned int currentFrame);

	std::vector<CommandBuffer>& GetCommandBufferList(QueueType type, unsigned int currentFrame);
	int& GetCurrentIndex(QueueType type, unsigned int currentFrame);

	VkCommandPool GetVkCommandPool(QueueType type, unsigned int currentFrame) const;

	std::array<VkCommandPool, MAX_FRAMES_IN_FLIGHT> m_graphicsCommandPools{};
	std::array<VkCommandPool, MAX_FRAMES_IN_FLIGHT> m_transferCommandPools{};

	std::array<std::vector<CommandBuffer>, MAX_FRAMES_IN_FLIGHT> m_graphicsCommandBuffers;
	std::array<std::vector<CommandBuffer>, MAX_FRAMES_IN_FLIGHT> m_transferCommandBuffers;

	std::array<int, MAX_FRAMES_IN_FLIGHT> m_currentGraphicsIndex;
	std::array<int, MAX_FRAMES_IN_FLIGHT> m_currentTransferIndex;

	VkDevice m_device;
};