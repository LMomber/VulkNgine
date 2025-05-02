#include "vkPhysicalDevice.h"

#include <map>

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
	ASSERT_VK_INSTANCE(instance);
	assert(surface != VK_NULL_HANDLE && "Surface is either uninitialized or deleted");

	PickPhysicalDevice(instance, surface);
}

VkPhysicalDevice PhysicalDevice::GetDevice() const
{
	ASSERT_VK_PHYSICAL_DEVICE(m_physicalDevice);
	return m_physicalDevice;
}

VkPhysicalDeviceProperties PhysicalDevice::GetProperties() const
{
	ASSERT_VK_PHYSICAL_DEVICE(m_physicalDevice);

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
	return properties;
}

SwapChainSupportDetails PhysicalDevice::QuerrySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface) const
{
	assert(device != VK_NULL_HANDLE && "Specified physical device is null");
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

QueueFamilyIndices PhysicalDevice::FindQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) const
{
	assert(device != VK_NULL_HANDLE && "Specified physical device is null");
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

VkFormat PhysicalDevice::FindSupportedFormat(const VkFormat format, const VkImageTiling tiling, const VkFormatFeatureFlags features) const
{
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

	if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
	{
		return format;
	}
	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
	{
		return format;
	}

	throw std::runtime_error("Specified format is not supported");
}

uint32_t PhysicalDevice::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags& properties) const
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable meory type");
}

void PhysicalDevice::PickPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

	std::multimap<int, VkPhysicalDevice> devices;
	for (const auto& device : physicalDevices)
	{
		int score = RatePhysicalDevice(device, surface);
		devices.insert(std::pair<int, VkPhysicalDevice>(score, device));
	}

	if (devices.rbegin()->first > 0)
	{
		m_physicalDevice = devices.rbegin()->second;
	}
	else
	{
		throw std::runtime_error("No devices are suitable for this application");
	}
}

int PhysicalDevice::RatePhysicalDevice(const VkPhysicalDevice device, const VkSurfaceKHR surface) const
{
	if (!IsDeviceSuitable(device, surface)) return 0;

	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	unsigned int score = 0;
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

bool PhysicalDevice::IsDeviceSuitable(const VkPhysicalDevice device, const VkSurfaceKHR surface) const
{
	QueueFamilyIndices indices = FindQueueFamilies(device, surface);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	return indices.IsComplete() && extensionsSupported && swapChainAdequate && features.samplerAnisotropy;
}

bool PhysicalDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}
