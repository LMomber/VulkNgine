#pragma once

#include "vkCommon.h"

class PhysicalDevice
{
public:
	PhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

	VkPhysicalDevice GetDevice() const;
	VkPhysicalDeviceProperties GetProperties() const;
	VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;

	SwapChainSupportDetails QuerrySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	VkFormat FindSupportedFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	uint32_t FindMemoryType(uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const;

private:
	void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	int RatePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) const;
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	bool CheckDeviceExtensionSupport( VkPhysicalDevice device) const;

	VkPhysicalDevice m_physicalDevice;
};