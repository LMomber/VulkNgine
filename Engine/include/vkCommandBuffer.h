#pragma once

#include "vkCommon.h"

class CommandPool;
class CommandBuffer
{
public:
	// Doesn't need initializers, allocating is done in the command pool, as well as resetting.
	void BeginCommandBuffer(const VkCommandBufferBeginInfo* info) const;
	void EndCommandBuffer() const;

	void BeginRendering(const VkRenderingInfo* info) const;
	void EndRendering() const;

	VkCommandBuffer* GetVkPtr();

	void SetViewPort(const VkViewport* pViewports, uint32_t firstViewport = 0, uint32_t viewportCount = 1) const;
	void SetScissor(const VkRect2D* pScissors, uint32_t firstScissor = 0, uint32_t scissorCount = 1) const;

	void DrawIndexed(uint32_t indexCount,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0,
		int32_t vertexOffset = 0,
		uint32_t firstInstance = 0) const;

	void BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);

	void BindVertexBuffers(const VkBuffer* pBuffers, const VkDeviceSize* pOffsets, uint32_t firstBinding = 0, uint32_t bindingCount = 1) const;
	void BindIndexBuffer(VkBuffer buffer, VkIndexType indexType, VkDeviceSize offset = 0) const;
	void BindDescriptorSets(
		VkPipelineLayout layout,
		const VkDescriptorSet* pDescriptorSets,
		uint32_t firstSet = 0,
		uint32_t descriptorSetCount = 1,
		uint32_t dynamicOffsetCount = 0,
		const uint32_t* pDynamicOffsets = nullptr) const;

	void MemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, VkDependencyFlags flags = 0) const;
	void BufferMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, VkDependencyFlags flags = 0) const;
	void ImageMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers, VkDependencyFlags flags = 0) const;

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
	void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
private:
	VkCommandBuffer m_commandBuffer;
	VkPipelineBindPoint m_pipelineBindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
};