#pragma once

#include "vkCommon.h"

class PhysicalDevice
{
public:
	PhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface);

	VkPhysicalDevice GetDevice() const;
	VkPhysicalDeviceProperties GetProperties() const;

	SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface) const;
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) const;
	VkFormat FindSupportedFormat(const VkFormat format, const VkImageTiling tiling, const VkFormatFeatureFlags features) const;
	uint32_t FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const;

private:
	void PickPhysicalDevice(VkInstance instance, const VkSurfaceKHR surface);
	int RatePhysicalDevice(const VkPhysicalDevice device, const VkSurfaceKHR surface) const;
	bool IsDeviceSuitable(const VkPhysicalDevice device, const VkSurfaceKHR surface) const;

	bool CheckDeviceExtensionSupport(const VkPhysicalDevice device) const;

	VkPhysicalDevice m_physicalDevice;
};