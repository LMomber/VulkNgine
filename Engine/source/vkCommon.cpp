#include "vkCommon.h"

SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	assert(device != VK_NULL_HANDLE && "Physical device is either uninitialized or deleted");
	assert(surface != VK_NULL_HANDLE && "Window surface is either uninitialized or deleted");

	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.m_capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.m_formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.m_formats.data());
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.m_presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.m_presentModes.data());
	}

	return details;
}

QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	assert(device != VK_NULL_HANDLE && "Physical device is either uninitialized or deleted");
	assert(surface != VK_NULL_HANDLE && "Window surface is either uninitialized or deleted");

	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

	int i = 0;
	for (const auto& property : queueFamilyProperties)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport)
		{
			indices.m_presentFamily = i;
		}

		if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.m_graphicsFamily = i;
		}

		if ((property.queueFlags & VK_QUEUE_TRANSFER_BIT) && (property.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0)
		{
			indices.m_transferFamily = i;
		}

		if (indices.IsComplete())
		{
			break;
		}

		i++;
	}

	if (indices.m_transferFamily.has_value() == false)
	{
		indices.m_transferFamily = indices.m_graphicsFamily;
	}

	return indices;
}