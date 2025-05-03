#pragma once

#include "vkCommon.h"

class CommandPool;
class Queue
{
public:
	Queue(VkDevice device, const QueueFamilyIndices& queueFamilyIndices);

	// Change name.. probably the class name over the function name. Make it plural?
	VkQueue GetQueue(const QueueType type) const;

	// Delete when CommandBuffer class is in place
	std::shared_ptr<CommandPool> GetCommandPool() const;

	VkCommandBuffer GetOrCreateCommandBuffer(const QueueType type, const unsigned int currentFrame);
	std::vector<VkCommandBuffer> GetOrCreateCommandBuffers(const QueueType type, const uint32_t count, const unsigned int currentFrame);

	void ResetCommandBuffers(const unsigned int currentFrame) const;

	Queue(const Queue&) = delete;
	Queue& operator=(const Queue&) = delete;
private:
	VkQueue m_graphicsQueue{};
	VkQueue m_presentQueue{};
	VkQueue m_transferQueue{};

	VkDevice m_device;

	QueueFamilyIndices m_queueFamilyIndices;

	std::shared_ptr<CommandPool> m_pCommandPool = nullptr;
};