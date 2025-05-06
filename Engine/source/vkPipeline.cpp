#include "vkPipeline.h"

#include "engine.h"
#include "vkDevice.h"

GraphicsPipeline::~GraphicsPipeline()
{
	for (const auto& shaderStage : m_shaderStages)
	{
		vkDestroyShaderModule(Core::engine.GetDevice().GetVkDevice(), shaderStage.module, nullptr);
	}

	vkDestroyPipeline(Core::engine.GetDevice().GetVkDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(Core::engine.GetDevice().GetVkDevice(), m_layout, nullptr);
}

void GraphicsPipeline::Create()
{
	assert(!m_shaderStages.empty() && "No shaders specified");

	if (vkCreatePipelineLayout(Core::engine.GetDevice().GetVkDevice(), &m_layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
	pipelineInfo.pStages = m_shaderStages.data();
	pipelineInfo.pVertexInputState = &m_vertexInputState;
	pipelineInfo.pInputAssemblyState = &m_inputAssemblyState;
	pipelineInfo.pViewportState = &m_viewportState;
	pipelineInfo.pRasterizationState = &m_rasterizationState;
	pipelineInfo.pMultisampleState = &m_multisampleState;
	pipelineInfo.pDepthStencilState = &m_depthStencilState;
	pipelineInfo.pColorBlendState = &m_colorBlendState;
	pipelineInfo.pDynamicState = &m_dynamicState;
	pipelineInfo.layout = m_layout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.pNext = &m_renderInfo;

	if (vkCreateGraphicsPipelines(Core::engine.GetDevice().GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}
}

VkPipeline GraphicsPipeline::Get() const
{
	return m_pipeline;
}

VkPipelineLayout GraphicsPipeline::GetLayout() const
{
	return m_layout;
}

void GraphicsPipeline::SetShader(const std::vector<char>& shaderCode, ShaderType type)
{
	assert(!shaderCode.empty() && "Shader code vector is empty");

	auto shaderStageFlag = GetShaderStageFlag(type);

#ifdef _DEBUG
	for (const auto& shaderStage : m_shaderStages)
	{
		if (shaderStage.stage == shaderStageFlag)
		{
			std::runtime_error("This shader type is already specified for this pipeline");
		}
	}
#endif

	const VkShaderModule shaderModule = CreateShaderModule(shaderCode);

	m_shaderStages.emplace_back();
	
	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	m_shaderStages.back().sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_shaderStages.back().stage = shaderStageFlag;
	m_shaderStages.back().module = shaderModule;
	m_shaderStages.back().pName = "main";

}

void GraphicsPipeline::SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates)
{
	assert(!dynamicStates.empty() && "Dynamic states vector is empty");

	// Needed for check in Create()
	m_dynamicStates = dynamicStates;

	m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
	m_dynamicState.pDynamicStates = m_dynamicStates.data();
}

void GraphicsPipeline::SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions)
{
	m_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	m_vertexInputState.pVertexBindingDescriptions = bindingDescriptions.data();
	m_vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	m_vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
}

void GraphicsPipeline::SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
{
	m_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_inputAssemblyState.topology = topology;
	m_inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
}

void GraphicsPipeline::SetViewportState(uint32_t viewportCount, uint32_t scissorCount)
{
	m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_viewportState.viewportCount = 1;
	m_viewportState.scissorCount = 1;
}

void GraphicsPipeline::SetRasterizationState(VkBool32 depthClampEnable, VkBool32 rasterizerDiscardEnable, VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFaceMode, float lineWidth, VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
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

void GraphicsPipeline::SetMultisampleState(VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, float minSampleShading, const VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable)
{
	m_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_multisampleState.sampleShadingEnable = sampleShadingEnable;
	m_multisampleState.rasterizationSamples = rasterizationSamples;
	m_multisampleState.minSampleShading = minSampleShading; // Optional
	m_multisampleState.pSampleMask = pSampleMask; // Optional
	m_multisampleState.alphaToCoverageEnable = alphaToCoverageEnable; // Optional
	m_multisampleState.alphaToOneEnable = alphaToOneEnable; // Optional
}

void GraphicsPipeline::SetColorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp, const std::vector<VkPipelineColorBlendAttachmentState>& attachments, float blendConstant1, float blendConstant2, float blendConstant3, float blendConstant4)
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

void GraphicsPipeline::SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts)
{
	m_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	m_layoutInfo.pSetLayouts = layouts.data();
	m_layoutInfo.pushConstantRangeCount = 0;
	m_layoutInfo.pPushConstantRanges = nullptr;
}

void GraphicsPipeline::SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<VkPushConstantRange>& pushConstants)
{
	m_layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_layoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	m_layoutInfo.pSetLayouts = layouts.data();
	m_layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	m_layoutInfo.pPushConstantRanges = pushConstants.data();
}

void GraphicsPipeline::SetDepthStencilState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp, VkBool32 depthBoundsTestEnable, float minDepthBounds, float maxDepthBounds, VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back)
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

void GraphicsPipeline::SetRenderInfo(const std::vector<VkFormat>& imageFormats, VkFormat depthFormat, VkFormat StencilFormat)
{
	m_renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	m_renderInfo.colorAttachmentCount = static_cast<uint32_t>(imageFormats.size());
	m_renderInfo.pColorAttachmentFormats = imageFormats.data();
	m_renderInfo.depthAttachmentFormat = depthFormat;
	m_renderInfo.stencilAttachmentFormat = StencilFormat;
}

VkShaderModule GraphicsPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Core::engine.GetDevice().GetVkDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Shader module creation failed");
	}

	return shaderModule;
}

VkShaderStageFlagBits GraphicsPipeline::GetShaderStageFlag(ShaderType type) const
{
	switch (type)
	{
	case VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
		break;
	case FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
		break;
	default:
		std::runtime_error("Specified shader type is not yet implemented");
		break;
	}
}
