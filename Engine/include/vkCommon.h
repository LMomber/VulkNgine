#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <memory>
#include <fstream>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

enum QueueType
{
	GRAPHICS,
	PRESENT,
	COMPUTE
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;

	bool IsComplete() const
	{
		return m_graphicsFamily.has_value() && m_presentFamily.has_value();
	}
};