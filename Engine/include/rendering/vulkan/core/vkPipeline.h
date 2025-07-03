#pragma once

#include "vkCommon.h"

#include <string>

enum ShaderType
{
	VERTEX,
	FRAGMENT
};

class GraphicsPipeline
{
public:
	GraphicsPipeline() = default;
	~GraphicsPipeline();

	void Create();

	VkPipeline Get() const;
	VkPipelineLayout GetLayout() const;

	void SetShader(const std::vector<char>& shaderCode, ShaderType type);
	void SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates);
	void SetVertexInputState(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions, const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
	void SetInputAssemblyState(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable);
	void SetViewportState(uint32_t viewportCount = 1, uint32_t scissorCount = 1);

	void SetRasterizationState(VkBool32 depthClampEnable, 
		VkBool32 rasterizerDiscardEnable, 
		VkPolygonMode polygonMode, 
		VkCullModeFlags cullMode, 
		VkFrontFace frontFaceMode, 
		float lineWidth = 1.f, 
		VkBool32 depthBiasEnable = VK_FALSE, 
		float depthBiasConstantFactor = 0.f, 
		float depthBiasClamp = 0.f, 
		float depthBiasSlopeFactor = 0.f);

	void SetMultisampleState(VkBool32 sampleShadingEnable, 
		VkSampleCountFlagBits rasterizationSamples, 
		float minSampleShading = 1.0f, 
		const VkSampleMask* pSampleMask = nullptr, 
		VkBool32 alphaToCoverageEnable = VK_FALSE, 
		VkBool32 alphaToOneEnable = VK_FALSE);

	void SetColorBlendState(VkBool32 logicOpEnable, 
		VkLogicOp logicOp, 
		const std::vector<VkPipelineColorBlendAttachmentState>& attachments, 
		float blendConstant1 = 0.f, 
		float blendConstant2 = 0.f, 
		float blendConstant3 = 0.f, 
		float blendConstant4 = 0.f);

	void SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts);
	void SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<VkPushConstantRange>& pushConstants);
	void SetDepthStencilState(VkBool32 depthTestEnable, 
		VkBool32 depthWriteEnable, 
		VkCompareOp depthCompareOp, 
		VkBool32 depthBoundsTestEnable, 
		float minDepthBounds = 0.f, 
		float maxDepthBounds = 1.f, 
		VkBool32 stencilTestEnable = VK_FALSE, 
		VkStencilOpState front = {}, 
		VkStencilOpState back = {});

	void SetRenderInfo(const std::vector<VkFormat>& imageFormats, VkFormat depthFormat = VK_FORMAT_UNDEFINED, VkFormat StencilFormat = VK_FORMAT_UNDEFINED);
private:
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	VkShaderStageFlagBits GetShaderStageFlag(ShaderType type) const;

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	std::vector<VkDynamicState> m_dynamicStates;
	VkPipelineDynamicStateCreateInfo m_dynamicState;
	VkPipelineVertexInputStateCreateInfo m_vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState;
	VkPipelineViewportStateCreateInfo m_viewportState;
	VkPipelineRasterizationStateCreateInfo m_rasterizationState;
	VkPipelineMultisampleStateCreateInfo m_multisampleState;
	VkPipelineColorBlendStateCreateInfo m_colorBlendState;
	VkPipelineDepthStencilStateCreateInfo m_depthStencilState;
	VkPipelineLayoutCreateInfo m_layoutInfo;
	VkPipelineRenderingCreateInfo m_renderInfo;
	VkGraphicsPipelineCreateInfo m_pipelineInfo;
	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};