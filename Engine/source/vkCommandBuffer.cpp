#include "vkCommandBuffer.h"

void CommandBuffer::BeginCommandBuffer(const VkCommandBufferBeginInfo* info) const
{
	if (vkBeginCommandBuffer(m_commandBuffer, info) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}
}

void CommandBuffer::EndCommandBuffer() const
{
	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void CommandBuffer::BeginRendering(const VkRenderingInfo* info) const
{
	vkCmdBeginRendering(m_commandBuffer, info);
}

void CommandBuffer::EndRendering() const
{
	vkCmdEndRendering(m_commandBuffer);
}

VkCommandBuffer* CommandBuffer::GetVkPtr()
{
	return &m_commandBuffer;
}

void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const
{
	vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
	m_pipelineBindPoint = pipelineBindPoint;
	vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
}

void CommandBuffer::BindVertexBuffers(const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, uint32_t firstBinding, uint32_t bindingCount) const
{
	vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void CommandBuffer::BindIndexBuffer(VkBuffer buffer, VkIndexType indexType, VkDeviceSize offset) const
{
	vkCmdBindIndexBuffer(m_commandBuffer, buffer, offset, indexType);
}

void CommandBuffer::BindDescriptorSets(VkPipelineLayout layout, const VkDescriptorSet* pDescriptorSets, uint32_t firstSet, uint32_t descriptorSetCount, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const
{
	assert(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM && "Pipeline bind point is not yet initialized. Call BindPipeline() before calling BindDescriptorSets()");
	vkCmdBindDescriptorSets(m_commandBuffer, m_pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void CommandBuffer::SetViewPort(const VkViewport* pViewports, uint32_t firstViewport, uint32_t viewportCount) const
{
	vkCmdSetViewport(m_commandBuffer, firstViewport, viewportCount, pViewports);
}

void CommandBuffer::SetScissor(const VkRect2D* pScissors, uint32_t firstScissor, uint32_t scissorCount) const
{
	vkCmdSetScissor(m_commandBuffer, firstScissor, scissorCount, pScissors);
}

void CommandBuffer::MemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, VkDependencyFlags flags) const
{
	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		memoryBarrierCount, pMemoryBarriers,
		0, nullptr,
		0, nullptr);
}

void CommandBuffer::BufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, VkDependencyFlags flags) const
{
	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		0, nullptr,
		bufferMemoryBarrierCount, pBufferMemoryBarriers,
		0, nullptr);
}

void CommandBuffer::ImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers, VkDependencyFlags flags) const
{
	vkCmdPipelineBarrier(m_commandBuffer,
		srcStageMask, dstStageMask,
		flags,
		0, nullptr,
		0, nullptr,
		imageMemoryBarrierCount, pImageMemoryBarriers);
}

void CommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const
{
	vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void CommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const
{
	vkCmdCopyBufferToImage(m_commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}
