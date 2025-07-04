#include "vkPipeline.h"

#include "hash.h"
#include "engine.h"
#include "vkDevice.h"

#include "vkPipelineCache.h"

VkPipeline Pipeline::Get() const
{
	return m_pipeline;
}

VkPipelineLayout Pipeline::GetLayout() const
{
	return m_layout;
}

void GraphicsPipelineInfo::SetShader(const std::string& filename, ShaderType type)
{
	assert(type != ShaderType::COMPUTE && "Compute shaders are not compatible with graphics pipelines");

#ifdef _DEBUG
	const auto shaderStageFlag = ShaderCache::GetShaderStageFlag(type);
	for (const auto& shaderStage : m_shaderStages)
	{
		if (shaderStage.stage == shaderStageFlag)
		{
			std::runtime_error("This shader type is already specified for this pipeline");
			return;
		}
	}
#endif

	m_shaderStages.emplace_back();
	m_shaderStages.back() = ShaderCache::GetOrCreateShader(filename, type);
}

void GraphicsPipelineInfo::SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates)
{
	assert(!dynamicStates.empty() && "Dynamic states vector is empty");

	m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	m_dynamicState.pDynamicStates = dynamicStates.data();
}

void GraphicsPipelineInfo::SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
{
	m_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	m_vertexInputState.pVertexBindingDescriptions = bindingDescriptions.data();
	m_vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	m_vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
}

void GraphicsPipelineInfo::SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
{
	m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyState.topology = topology;
	m_inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
}

void GraphicsPipelineInfo::SetViewportState(uint32_t viewportCount, uint32_t scissorCount)
{
	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.viewportCount = viewportCount;
	m_viewportState.scissorCount = scissorCount;
}

void GraphicsPipelineInfo::SetRasterizationState(VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFaceMode, float lineWidth, VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
	m_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_rasterizationState.depthClampEnable = depthClampEnable;
	m_rasterizationState.rasterizerDiscardEnable = rasterizerDiscardEnable;
	m_rasterizationState.polygonMode = polygonMode;
	m_rasterizationState.lineWidth = lineWidth;
	m_rasterizationState.cullMode = cullMode;
	m_rasterizationState.frontFace = frontFaceMode;
	m_rasterizationState.depthBiasEnable = depthBiasEnable;
	m_rasterizationState.depthBiasConstantFactor = depthBiasConstantFactor; // Optional
	m_rasterizationState.depthBiasClamp = depthBiasClamp; // Optional
	m_rasterizationState.depthBiasSlopeFactor = depthBiasSlopeFactor; // Optional
}

void GraphicsPipelineInfo::SetMultisampleState(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, float minSampleShading, const VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable)
{
	m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampleState.sampleShadingEnable = sampleShadingEnable;
	m_multisampleState.rasterizationSamples = rasterizationSamples;
	m_multisampleState.minSampleShading = minSampleShading; // Optional
	m_multisampleState.pSampleMask = pSampleMask; // Optional
	m_multisampleState.alphaToCoverageEnable = alphaToCoverageEnable; // Optional
	m_multisampleState.alphaToOneEnable = alphaToOneEnable; // Optional
}

void GraphicsPipelineInfo::SetColorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, const std::vector<VkPipelineColorBlendAttachmentState>& attachments, float blendConstant1, float blendConstant2, float blendConstant3, float blendConstant4)
{
	m_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_colorBlendState.logicOpEnable = logicOpEnable;
	m_colorBlendState.logicOp = logicOp; // Optional
	m_colorBlendState.attachmentCount = static_cast<uint32_t>(attachments.size());
	m_colorBlendState.pAttachments = attachments.data();
	m_colorBlendState.blendConstants[0] = blendConstant1; // Optional
	m_colorBlendState.blendConstants[1] = blendConstant2; // Optional
	m_colorBlendState.blendConstants[2] = blendConstant3; // Optional
	m_colorBlendState.blendConstants[3] = blendConstant4; // Optional
}

void GraphicsPipelineInfo::SetDepthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back)
{
	m_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilState.depthTestEnable = depthTestEnable;
	m_depthStencilState.depthWriteEnable = depthWriteEnable;
	m_depthStencilState.depthCompareOp = depthCompareOp;
	m_depthStencilState.depthBoundsTestEnable = depthBoundsTestEnable;
	m_depthStencilState.minDepthBounds = minDepthBounds;
	m_depthStencilState.maxDepthBounds = maxDepthBounds;
	m_depthStencilState.stencilTestEnable = stencilTestEnable;
	m_depthStencilState.front = front;
	m_depthStencilState.back = back;
}

void GraphicsPipelineInfo::SetRenderInfo(const std::vector<VkFormat>& imageFormats, VkFormat depthFormat, VkFormat StencilFormat)
{
	m_renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	m_renderInfo.colorAttachmentCount = static_cast<uint32_t>(imageFormats.size());
	m_renderInfo.pColorAttachmentFormats = imageFormats.data();
	m_renderInfo.depthAttachmentFormat = depthFormat;
	m_renderInfo.stencilAttachmentFormat = StencilFormat;
}

std::size_t GraphicsPipelineInfo::Hash() const
{
	assert(m_shaderStages.size() > 0 && "No shaders are specified in this pipeline.");

	std::size_t result = 0;

	// Shaders
	for (std::size_t i = 0; i < m_shaderStages.size(); ++i)
	{
		HashCombine(result, m_shaderStages[i].stage);
		HashCombine(result, reinterpret_cast<std::uintptr_t>(m_shaderStages[i].module));
		// HashCombine(result, m_shaderStages[i].pName); // I think this causes issues
	}
	//

	// Dynamic states
	if (m_dynamicState.dynamicStateCount > 0 && m_dynamicState.pDynamicStates)
	{
		std::vector<VkDynamicState> states;
		states.reserve(m_dynamicState.dynamicStateCount);

		bool isSorted = true;
		int lastEnum = std::numeric_limits<int>::min();

		for (uint32_t i = 0; i < m_dynamicState.dynamicStateCount; ++i)
		{
			states.push_back(m_dynamicState.pDynamicStates[i]);

			if (m_dynamicState.pDynamicStates[i] < lastEnum)
			{
				isSorted = false;
			}

			lastEnum = m_dynamicState.pDynamicStates[i];
		}

		if (!isSorted)
		{
			std::sort(states.begin(), states.end(), [](const VkDynamicState& a, const VkDynamicState& b)
				{return static_cast<int>(a) < static_cast<int>(b); });
		}

		for (const auto& state : states)
		{
			HashCombine(result, state);
		}
	}
	//

	// TODO: Maybe only hash the attribute and binding counts? Less expensive but also less accurate
	// 
	// Vertex Descriptions
	if (m_vertexInputState.vertexAttributeDescriptionCount > 0)
	{
		for (uint32_t i = 0; i < m_vertexInputState.vertexAttributeDescriptionCount; ++i)
		{
			HashCombine(result, m_vertexInputState.pVertexAttributeDescriptions->binding);
			HashCombine(result, m_vertexInputState.pVertexAttributeDescriptions->location);
			HashCombine(result, m_vertexInputState.pVertexAttributeDescriptions->format);
			HashCombine(result, m_vertexInputState.pVertexAttributeDescriptions->offset);
		}
	}

	if (m_vertexInputState.vertexBindingDescriptionCount > 0)
	{
		for (uint32_t i = 0; i < m_vertexInputState.vertexBindingDescriptionCount; ++i)
		{
			HashCombine(result, m_vertexInputState.pVertexBindingDescriptions->binding);
			HashCombine(result, m_vertexInputState.pVertexBindingDescriptions->stride);
			HashCombine(result, m_vertexInputState.pVertexBindingDescriptions->inputRate);
		}
	}
	//

	// Input Assembly State
	HashCombine(result, m_inputAssemblyState.topology);
	HashCombine(result, m_inputAssemblyState.primitiveRestartEnable);
	//

	// Viewport State
	HashCombine(result, m_viewportState.scissorCount);
	HashCombine(result, m_viewportState.viewportCount);
	//

	// Rasterization State
	HashCombine(result, m_rasterizationState.polygonMode);
	HashCombine(result, m_rasterizationState.cullMode);
	HashCombine(result, m_rasterizationState.frontFace);
	HashCombine(result, m_rasterizationState.depthBiasEnable);
	//

	// Multisample State
	HashCombine(result, m_multisampleState.rasterizationSamples);
	HashCombine(result, m_multisampleState.alphaToCoverageEnable);
	HashCombine(result, m_multisampleState.alphaToOneEnable);
	HashCombine(result, m_multisampleState.sampleShadingEnable);
	//

	// Color Blend State
	HashCombine(result, m_colorBlendState.attachmentCount);
	HashCombine(result, m_colorBlendState.logicOpEnable);
	//

	// Depth Stencil State
	HashCombine(result, m_depthStencilState.depthBoundsTestEnable);
	HashCombine(result, m_depthStencilState.depthTestEnable);
	HashCombine(result, m_depthStencilState.depthWriteEnable);
	HashCombine(result, m_depthStencilState.stencilTestEnable);
	//

	// Layout Info 
	// (I'm pretty sure that hashing the actual layouts & push constant ranges would cause issues, 
	// since you can have multiple identical layouts in different memory locations)
	// TODO: Look into caching Pipeline Layouts and Push Constant ranges.
	HashCombine(result, m_layoutInfo.setLayoutCount);
	HashCombine(result, m_layoutInfo.pushConstantRangeCount);
	//

	// Rendering Info
	HashCombine(result, m_renderInfo.viewMask);
	HashCombine(result, m_renderInfo.colorAttachmentCount);
	if (m_renderInfo.colorAttachmentCount > 0)
	{
		for (uint32_t i = 0; i < m_renderInfo.colorAttachmentCount; ++i)
		{
			HashCombine(result, m_renderInfo.pColorAttachmentFormats[i]);
		}
	}
	HashCombine(result, m_renderInfo.depthAttachmentFormat);
	HashCombine(result, m_renderInfo.stencilAttachmentFormat);
	//

	return result;
}

Pipeline::~Pipeline()
{
	vkDestroyPipeline(Core::engine.GetDevice().GetVkDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(Core::engine.GetDevice().GetVkDevice(), m_layout, nullptr);
}

void ComputePipelineInfo::SetShader(const std::string& filename, ShaderType type)
{
	assert(type == ShaderType::COMPUTE && "Only compute shaders are compatible with the compute pipeline");
	assert(m_shaderStages.size() <= 1 && "Compute pipelines can only have a single shader");

	m_shaderStages.emplace_back();
	m_shaderStages[0] = ShaderCache::GetOrCreateShader(filename, type);
}

std::size_t ComputePipelineInfo::Hash() const
{
	assert(m_shaderStages.size() > 0 && "No shaders are specified in this pipeline.");

	std::size_t result = 0;

	// Shaders
	for (std::size_t i = 0; i < m_shaderStages.size(); ++i)
	{
		HashCombine(result, m_shaderStages[i].stage);
		HashCombine(result, reinterpret_cast<std::uintptr_t>(m_shaderStages[i].module));
		// HashCombine(result, m_shaderStages[i].pName); // I think this causes issues
	}
	//

	// Layout Info 
	// (I'm pretty sure that hashing the actual layouts & push constant ranges would cause issues, 
	// since you can have multiple identical layouts in different memory locations)
	// TODO: Look into caching Pipeline Layouts and Push Constant ranges.
	HashCombine(result, m_layoutInfo.setLayoutCount);
	HashCombine(result, m_layoutInfo.pushConstantRangeCount);
	//

	return result;
}

void BasePipelineInfo::SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts)
{
	m_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	m_layoutInfo.pSetLayouts = layouts.data();
	m_layoutInfo.pushConstantRangeCount = 0;
	m_layoutInfo.pPushConstantRanges = nullptr;
}

void BasePipelineInfo::SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<VkPushConstantRange>& pushConstants)
{
	m_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	m_layoutInfo.pSetLayouts = layouts.data();
	m_layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	m_layoutInfo.pPushConstantRanges = pushConstants.data();
}
