#pragma once

#include "vkCommon.h"

#include <string>
#include <unordered_map>

class ShaderCache
{
public:
	static VkPipelineShaderStageCreateInfo GetOrCreateShader(const std::string& filename, ShaderType type);
	static void Reset();

	static VkShaderStageFlagBits GetShaderStageFlag(ShaderType type);
private:
	static VkShaderModule CreateShaderModule(const std::vector<char>& code);

	// TODO: Make shared_ptr to reduce copy costs
	inline static std::unordered_map<std::string, VkPipelineShaderStageCreateInfo> m_shaderCache;
};

struct GraphicsPipelineInfo;
class Pipeline;
// TODO: Add compute support
class PipelineCache
{
public:
	static std::shared_ptr<Pipeline> GetOrCreateGraphicsPipeline(const GraphicsPipelineInfo& info);
	static void Reset();

private:
	static std::shared_ptr<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineInfo& info);

	inline static std::unordered_map<size_t, std::shared_ptr<Pipeline>> m_graphicsPipelineCache;
};