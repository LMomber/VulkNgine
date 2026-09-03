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

const VkCommandBuffer CommandBuffer::GetVkCommandBuffer() const
{
	return m_commandBuffer;
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
	vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
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

void CommandBuffer::BindPushConstants(const VkPushConstantsInfo& info) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	vkCmdPushConstants2(m_commandBuffer, &info);
}

void CommandBuffer::TransitionImageLayout(VkImage image, VkFormat format, ImageLayout& imageLayout, VkImageLayout newLayout) const
{
	// If the new layout is the same as the current layout, no transition is needed.
	if (imageLayout.m_layout == newLayout) return;

	VkImageLayout oldLayout = imageLayout.m_layout;
	imageLayout.m_layout = newLayout;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
		&& newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
		&& newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		&& newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		&& newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
		&& newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		&& newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
		&& newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		&& newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
		&& newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::runtime_error("Unsupported layout transition");
	}

	if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	ImageMemoryBarrier(sourceStage, destStage, 1, &barrier);
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

void CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(m_commandBuffer, src, dst, 1, &copyRegion);
}

void CommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
{
	ASSERT_COMMAND_BUFFER(m_commandBuffer);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(m_commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

bool CommandBuffer::HasStencilComponent(VkFormat format) const
{
	return (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT
		|| format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
}
