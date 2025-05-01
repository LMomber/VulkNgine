#pragma once

#define GLFW_INCLUDE_VULKAN
#include "glfw/include/GLFW/glfw3.h"

#include <vector>
#include <memory>
#include <fstream>
#include <optional>
#include <set>
#include <iostream>

#include <cassert>
#include <stdexcept>

#define ASSERT_VK_LOGICAL_DEVICE(device) assert(device != VK_NULL_HANDLE && "Vulkan device is either uninitialized or deleted")
#define ASSERT_VK_PHYSICAL_DEVICE(device) assert(device != VK_NULL_HANDLE && "Vulkan physical device is either uninitialized or deleted")
#define ASSERT_VK_INSTANCE(swapchain) assert(swapchain != VK_NULL_HANDLE && "Vulkan instance is either uninitialized or deleted")

#define ASSERT_VK_SWAPCHAIN_PTR(swapchain) assert(swapchain && "Vulkan swapchain is either uninitialized or deleted")
#define ASSERT_VK_SURFACE_PTR(surface) assert(surface && "Vulkan window surface is either uninitialized or deleted")
#define ASSERT_VK_WINDOW_PTR(window) assert(window && "Vulkan window is either uninitialized or deleted")
#define ASSERT_GLFW_WINDOW_PTR(window) assert(window && "GLFW window is either uninitialized or deleted")

// Some macro practice
// Simple log to check if a system is initialized successfully
#ifdef _DEBUG
	#define INIT_WRAPPER(name, function)		\
		do {										\
		std::cout << "Initializing " << name << "..." << "\n"; \
		function;									\
		std::cout << "Initialized "<< name << " successfully!" << "\n";				\
		} while(0)
#else
	#define INIT_WRAPPER(name, function)		\
		do {										\
		function;									\
		} while(0)
#endif
	

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

enum QueueType
{
	GRAPHICS,
	PRESENT,
	COMPUTE,
	TRANSFER
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;
	std::optional<uint32_t> m_transferFamily;

	bool IsComplete() const
	{
		return m_graphicsFamily.has_value() && m_presentFamily.has_value() && m_transferFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

VkFormat FindSupportedFormat(VkPhysicalDevice device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, const VkMemoryPropertyFlags& properties);

