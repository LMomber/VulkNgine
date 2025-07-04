#include "vkPipelineCache.h"

#include "fileIO.h"
#include "engine.h"
#include "vkDevice.h"
#include "vkPipeline.h"

#include <algorithm>

VkPipelineShaderStageCreateInfo ShaderCache::GetOrCreateShader(const std::string& filename, ShaderType type)
{
	auto it = m_shaderCache.find(filename);
	if (it != m_shaderCache.end())
	{
		return it->second;
	}
	else
	{
		const auto shaderCode = ReadFile(filename);

		assert(!shaderCode.empty() && "Shader code vector is empty");

		const auto shaderStageFlag = GetShaderStageFlag(type);
		const VkShaderModule shaderModule = CreateShaderModule(shaderCode);

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderStageFlag;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";

		m_shaderCache[filename] = shaderStageInfo;

		return shaderStageInfo;
	}
}
	
void ShaderCache::Reset()
{
	VkDevice device = Core::engine.GetDevice().GetVkDevice();

	for (auto& [key, shader] : m_shaderCache) 
	{
		if (shader.module != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(device, shader.module, nullptr);
		}
	}

	m_shaderCache.clear();
}

VkShaderModule ShaderCache::CreateShaderModule(const std::vector<char>& code)
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

VkShaderStageFlagBits ShaderCache::GetShaderStageFlag(ShaderType type)
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
		throw std::runtime_error("Specified shader type is not yet implemented");
		break;
	}
}

// The hash is pretty expensive so try to call this function only in initialization, afterwards store it for reuse.
std::shared_ptr<Pipeline> PipelineCache::GetOrCreateGraphicsPipeline(const GraphicsPipelineInfo& info)
{
	size_t hash = info.Hash();

	auto it = m_graphicsPipelineCache.find(hash);
	if (it != m_graphicsPipelineCache.end())
	{
		return it->second;
	}
	else
	{
		const auto pipeline = CreateGraphicsPipeline(info);
		m_graphicsPipelineCache[hash] = pipeline;

		return pipeline;
	}
}

void PipelineCache::Reset()
{
	m_graphicsPipelineCache.clear();
}

std::shared_ptr<Pipeline> PipelineCache::CreateGraphicsPipeline(const GraphicsPipelineInfo& info)
{
	assert(!info.m_shaderStages.empty() && "No shaders specified");

	std::shared_ptr<Pipeline> output = std::make_shared<Pipeline>();

	if (vkCreatePipelineLayout(Core::engine.GetDevice().GetVkDevice(), &info.m_layoutInfo, nullptr, &output.get()->m_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(info.m_shaderStages.size());
	pipelineInfo.pStages = info.m_shaderStages.data();
	pipelineInfo.pVertexInputState = &info.m_vertexInputState;
	pipelineInfo.pInputAssemblyState = &info.m_inputAssemblyState;
	pipelineInfo.pViewportState = &info.m_viewportState;
	pipelineInfo.pRasterizationState = &info.m_rasterizationState;
	pipelineInfo.pMultisampleState = &info.m_multisampleState;
	pipelineInfo.pDepthStencilState = &info.m_depthStencilState;
	pipelineInfo.pColorBlendState = &info.m_colorBlendState;
	pipelineInfo.pDynamicState = &info.m_dynamicState;
	pipelineInfo.layout = output.get()->m_layout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	pipelineInfo.pNext = &info.m_renderInfo;

	if (vkCreateGraphicsPipelines(Core::engine.GetDevice().GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &output.get()->m_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	return output;
}
