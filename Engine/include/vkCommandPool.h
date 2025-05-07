#pragma once

#include "vkCommon.h"

class CommandBuffer;
class CommandPool
{
public:
	CommandPool(VkDevice device, const QueueFamilyIndices& queueFamilyIndices);
	~CommandPool();

	const CommandBuffer& GetOrCreateCommandBuffer(const QueueType type, const unsigned int currentFrame);
	std::vector<CommandBuffer> GetOrCreateCommandBuffers(const QueueType type, const unsigned int count, const unsigned int currentFrame);

	void ResetCommandBuffers(const unsigned int currentFrame);

	CommandPool(const CommandPool&) = delete;
	CommandPool& operator=(const CommandPool&) = delete;

	// Make private when CommandBuffer class is in place, // Change name..
	VkCommandPool GetCommandPool(const QueueType type, const unsigned int currentFrame) const;
private:
	const CommandBuffer& CreateCommandBuffer(const QueueType type, const unsigned int currentFrame);
	std::vector<CommandBuffer> CreateCommandBuffers(const QueueType type, const unsigned int count, const unsigned int currentFrame);

	std::vector<CommandBuffer>& GetCommandBufferList(const QueueType type, const unsigned int currentFrame);
	int& GetCurrentIndex(const QueueType type, const unsigned int currentFrame);

	std::array<VkCommandPool, MAX_FRAMES_IN_FLIGHT> m_graphicsCommandPools{};
	std::array<VkCommandPool, MAX_FRAMES_IN_FLIGHT> m_transferCommandPools{};

	// TODO: Replace with vectors of command buffer class
	std::array<std::vector<CommandBuffer>, MAX_FRAMES_IN_FLIGHT> m_graphicsCommandBuffers;
	std::array<std::vector<CommandBuffer>, MAX_FRAMES_IN_FLIGHT> m_transferCommandBuffers;

	// Initialized to -1 so that after the first buffer creation it becomes 0;
	std::array<int, MAX_FRAMES_IN_FLIGHT> m_currentGraphicsIndex;
	std::array<int, MAX_FRAMES_IN_FLIGHT> m_currentTransferIndex;

	VkDevice m_device;
};