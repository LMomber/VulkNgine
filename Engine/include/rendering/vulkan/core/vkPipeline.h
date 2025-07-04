#pragma once

#include "vkCommon.h"

#include <string>

struct BasePipelineInfo
{
	virtual void SetShader(const std::string& filename, ShaderType type) = 0;

	void SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts);
	void SetLayoutInfo(const std::vector<VkDescriptorSetLayout>& layouts, const std::vector<VkPushConstantRange>& pushConstants);

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages{};
	VkPipelineLayoutCreateInfo m_layoutInfo{};
};

struct ComputePipelineInfo : public BasePipelineInfo
{	
	void SetShader(const std::string& filename, ShaderType type);

	std::size_t Hash() const;
};

struct GraphicsPipelineInfo : public BasePipelineInfo
{
	void SetShader(const std::string& filename, ShaderType type);
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

	std::size_t Hash() const;

private:
	friend class PipelineCache;

	VkPipelineDynamicStateCreateInfo m_dynamicState{};
	VkPipelineVertexInputStateCreateInfo m_vertexInputState{};
	VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyState{};
	VkPipelineViewportStateCreateInfo m_viewportState{};
	VkPipelineRasterizationStateCreateInfo m_rasterizationState{};
	VkPipelineMultisampleStateCreateInfo m_multisampleState{};
	VkPipelineColorBlendStateCreateInfo m_colorBlendState{};
	VkPipelineDepthStencilStateCreateInfo m_depthStencilState{};
	VkPipelineRenderingCreateInfo m_renderInfo{};
};

class Pipeline
{
public:
	Pipeline() = default;
	~Pipeline();

	VkPipeline Get() const;
	VkPipelineLayout GetLayout() const;

private:
	friend class PipelineCache;

	VkPipelineLayout m_layout{};
	VkPipeline m_pipeline{};
};