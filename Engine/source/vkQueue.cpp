#include "vkQueue.h"

#include "vkCommandPool.h"
#include "vkCommandBuffer.h"

Queue::Queue(VkDevice device, const QueueFamilyIndices& queueFamilyIndices) :
	m_device(device)
{
	m_queueFamilyIndices = queueFamilyIndices;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.m_graphicsFamily.value(), queueFamilyIndices.m_presentFamily.value(), queueFamilyIndices.m_transferFamily.value() };

	float queuePriority = 1.f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vkGetDeviceQueue(m_device, queueFamilyIndices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, queueFamilyIndices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, queueFamilyIndices.m_transferFamily.value(), 0, &m_transferQueue);

	m_pCommandPool = std::make_shared<CommandPool>(device, queueFamilyIndices);
}

VkQueue Queue::GetQueue(const QueueType type) const
{
	switch (type)
	{
	case GRAPHICS:
		return m_graphicsQueue;
		break;
	case PRESENT:
		return m_presentQueue;
		break;
	case TRANSFER:
		return m_transferQueue;
		break;
	case COMPUTE:
		throw std::runtime_error("No functionality for queue type 'COMPUTE' has been implemented");
		// TODO
		break;
	}

	throw std::runtime_error("Undefined queue type specified");
}

std::shared_ptr<CommandPool> Queue::GetCommandPool() const
{
	return m_pCommandPool;
}

const CommandBuffer& Queue::GetOrCreateCommandBuffer(const QueueType type, const unsigned int currentFrame)
{
	return m_pCommandPool->GetOrCreateCommandBuffer(type, currentFrame);
}

std::vector<CommandBuffer> Queue::GetOrCreateCommandBuffers(const QueueType type, const uint32_t count, const unsigned int currentFrame)
{
	return m_pCommandPool->GetOrCreateCommandBuffers(type, count, currentFrame);
}

void Queue::ResetCommandBuffers(const unsigned int currentFrame) const
{
	m_pCommandPool->ResetCommandBuffers(currentFrame);
}
