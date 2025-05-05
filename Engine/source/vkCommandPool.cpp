#include "vkCommandPool.h"

#include "vkCommandBuffer.h"

#define ASSERT_CURRENT_FRAME(currentFrame) assert(currentFrame < MAX_FRAMES_IN_FLIGHT && "currentFrame has a higher value that the maximum amount of frames in flight")

CommandPool::CommandPool(VkDevice device, const QueueFamilyIndices& queueFamilyIndices) : m_device(device)
{
	m_currentGraphicsIndex.fill(-1);
	m_currentTransferIndex.fill(-1);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamily.value();

		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPools[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}
#
		poolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.m_transferFamily.value();

		if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_transferCommandPools[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}

		// TODO: Compute pool
	}
}

CommandPool::~CommandPool()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyCommandPool(m_device, m_graphicsCommandPools[i], nullptr);
		vkDestroyCommandPool(m_device, m_transferCommandPools[i], nullptr);
	}
}

const CommandBuffer& CommandPool::GetOrCreateCommandBuffer(const QueueType type, const unsigned int currentFrame)
{
	ASSERT_CURRENT_FRAME(currentFrame);

	const std::vector<CommandBuffer>& commandBufferList = GetCommandBufferList(type, currentFrame);
	int& currentIndex = GetCurrentIndex(type, currentFrame);
	currentIndex++;

	if (currentIndex < commandBufferList.size())
	{
		return commandBufferList[currentIndex];
	}
	else
	{
		return CreateCommandBuffer(type, currentFrame);
	}
}

const std::vector<CommandBuffer>& CommandPool::GetOrCreateCommandBuffers(const QueueType type, const unsigned int count, const unsigned int currentFrame)
{
	ASSERT_CURRENT_FRAME(currentFrame);

	const std::vector<CommandBuffer>& commandBufferList = GetCommandBufferList(type, currentFrame);
	int& currentIndex = GetCurrentIndex(type, currentFrame);
	currentIndex++;
	const int endIndex = currentIndex + count;

	if (endIndex < commandBufferList.size())
	{
		return std::vector<CommandBuffer>(commandBufferList.begin() + currentIndex, commandBufferList.begin() + endIndex);
	}
	else
	{
		return CreateCommandBuffers(type, count, currentFrame);
	}
}

void CommandPool::ResetCommandBuffers(const unsigned int currentFrame)
{
	ASSERT_CURRENT_FRAME(currentFrame);

	vkResetCommandPool(m_device, m_graphicsCommandPools[currentFrame], 0);
	vkResetCommandPool(m_device, m_transferCommandPools[currentFrame], 0);

	m_currentGraphicsIndex[currentFrame] = 0;
	m_currentTransferIndex[currentFrame] = 0;
}

const CommandBuffer& CommandPool::CreateCommandBuffer(const QueueType type, const unsigned int currentFrame)
{
	VkCommandPool commandPool = GetCommandPool(type, currentFrame);
	std::vector<CommandBuffer>& commandBufferList = GetCommandBufferList(type, currentFrame);
	commandBufferList.emplace_back();

	// Index already gets incremented in GetOrCreateCommandBuffer()
	int& currentIndex = GetCurrentIndex(type, currentFrame);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer* pCommandBuffer = commandBufferList[currentIndex].GetVkPtr();

	if (vkAllocateCommandBuffers(m_device, &allocInfo, pCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}

	return commandBufferList[currentIndex];
}

std::vector<CommandBuffer> CommandPool::CreateCommandBuffers(const QueueType type, const unsigned int count, const unsigned int currentFrame)
{
	VkCommandPool commandPool = GetCommandPool(type, currentFrame);
	std::vector<CommandBuffer>& commandBufferList = GetCommandBufferList(type, currentFrame);

	// CurrentIndex already gets incremented in GetOrCreateCommandBuffer()
	int& currentIndex = GetCurrentIndex(type, currentFrame);
	const int startingIndex = currentIndex;
	const int endIndex = currentIndex + count;
	currentIndex = endIndex - 1;

	// TODO: Make function in commandBuffer class to check how many free buffers are available, copy this functionality
	const int amountOfAvailableBuffers = static_cast<int>(commandBufferList.size()) - startingIndex;
	const int amountOfNewBuffers = count - amountOfAvailableBuffers;

	assert(amountOfNewBuffers > 0 && "Amount of buffers to create is < 1");

	commandBufferList.reserve(commandBufferList.size() + amountOfNewBuffers);
	for (int i = 0; i < amountOfNewBuffers; i++)
	{
		commandBufferList.emplace_back();
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;

	VkCommandBuffer* pCommandBuffer = commandBufferList[startingIndex].GetVkPtr();

	if (vkAllocateCommandBuffers(m_device, &allocInfo, pCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}

	return std::vector<CommandBuffer>(commandBufferList.begin() + startingIndex, commandBufferList.begin() + endIndex);
}

// Make private when CommandBuffer class is in place, // Change name..
VkCommandPool CommandPool::GetCommandPool(const QueueType type, const unsigned int currentFrame) const
{
	// Get rid of assert when function is private
	ASSERT_CURRENT_FRAME(currentFrame);

	switch (type)
	{
	case QueueType::PRESENT:
	case QueueType::GRAPHICS:
		return m_graphicsCommandPools[currentFrame];
		break;
	case QueueType::TRANSFER:
		return m_transferCommandPools[currentFrame];
		break;
	case QueueType::COMPUTE:
		throw std::runtime_error("'COMPUTE' command pool is not yet implemented");
	default:
		throw std::runtime_error("Unsupported queue type specified");
	}
}

std::vector<CommandBuffer>& CommandPool::GetCommandBufferList(const QueueType type, const unsigned int currentFrame)
{
	switch (type)
	{
	case QueueType::PRESENT:
	case QueueType::GRAPHICS:
		return m_graphicsCommandBuffers[currentFrame];
		break;
	case QueueType::TRANSFER:
		return m_transferCommandBuffers[currentFrame];
		break;
	case QueueType::COMPUTE:
		throw std::runtime_error("'COMPUTE' command buffers are not yet implemented");
	default:
		throw std::runtime_error("Unsupported queue type specified");
	}
}

int& CommandPool::GetCurrentIndex(const QueueType type, const unsigned int currentFrame)
{
	switch (type)
	{
	case QueueType::PRESENT:
	case QueueType::GRAPHICS:
		return m_currentGraphicsIndex[currentFrame];
		break;
	case QueueType::TRANSFER:
		return m_currentTransferIndex[currentFrame];
		break;
	case QueueType::COMPUTE:
		throw std::runtime_error("'COMPUTE' index counter is not yet implemented");
	default:
		throw std::runtime_error("Unsupported queue type specified");
	}
}
