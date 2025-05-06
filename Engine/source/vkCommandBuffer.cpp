#include "vkCommandBuffer.h"

#define ASSERT_COMMAND_BUFFER(commandBuffer) assert(commandBuffer != VK_NULL_HANDLE && "Command buffer is not yet initialized");

void CommandBuffer::BeginCommandBuffer(const VkCommandBufferBeginInfo* info) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	if (vkBeginCommandBuffer(m_commandBuffer, info) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}
}

void CommandBuffer::EndCommandBuffer() const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void CommandBuffer::BeginRendering(const VkRenderingInfo* info) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdBeginRendering(m_commandBuffer, info);
}

void CommandBuffer::EndRendering() const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdEndRendering(m_commandBuffer);
}

VkCommandBuffer* CommandBuffer::GetVkPtr()
{
	return &m_commandBuffer;
}

void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	m_pipelineBindPoint = pipelineBindPoint;
	vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
}

void CommandBuffer::BindVertexBuffers(const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, uint32_t firstBinding, uint32_t bindingCount) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void CommandBuffer::BindIndexBuffer(VkBuffer buffer, VkIndexType indexType, VkDeviceSize offset) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdBindIndexBuffer(m_commandBuffer, buffer, offset, indexType);
}

void CommandBuffer::BindDescriptorSets(VkPipelineLayout layout, const VkDescriptorSet* pDescriptorSets, uint32_t firstSet, uint32_t descriptorSetCount, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	assert(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM && "Pipeline bind point is not yet initialized. Call BindPipeline() before calling BindDescriptorSets()");
	vkCmdBindDescriptorSets(m_commandBuffer, m_pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void CommandBuffer::SetViewPort(const VkViewport* pViewports, uint32_t firstViewport, uint32_t viewportCount) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdSetViewport(m_commandBuffer, firstViewport, viewportCount, pViewports);
}

void CommandBuffer::SetScissor(const VkRect2D* pScissors, uint32_t firstScissor, uint32_t scissorCount) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdSetScissor(m_commandBuffer, firstScissor, scissorCount, pScissors);
}

void CommandBuffer::MemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, VkDependencyFlags flags) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		memoryBarrierCount, pMemoryBarriers,
		0, nullptr,
		0, nullptr);
}

void CommandBuffer::BufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, VkDependencyFlags flags) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		0, nullptr,
		bufferMemoryBarrierCount, pBufferMemoryBarriers,
		0, nullptr);
}

void CommandBuffer::ImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers, VkDependencyFlags flags) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		0, nullptr,
		0, nullptr,
		imageMemoryBarrierCount, pImageMemoryBarriers);
}

void CommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void CommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdCopyBufferToImage(m_commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}
