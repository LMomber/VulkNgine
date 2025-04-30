#include "vkDevice.h"

//#include "vkSwapchain.h"

#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <string>

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else 
const bool enableValidationLayers = true;
#endif

void Device::Initialize()
{
	m_pVkWindow = std::make_shared<Window>();
	CreateInstance();
	InitDebugMessenger();
	m_pSurface = std::make_unique<Surface>(m_instance, m_pVkWindow->GetWindow());
	PickPhysicalDevice();
	CreateLogicalDevice();
	m_pSwapchain = std::make_shared<Swapchain>(m_device, m_physicalDevice, m_pSurface->GetSurface(), m_pVkWindow);
}

void Device::ShutDown()
{
	m_pSwapchain.reset();

	vkDestroyDevice(m_device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	m_pSurface.reset();

	vkDestroyInstance(m_instance, nullptr);

	m_pVkWindow.reset();

	glfwTerminate();
}

void Device::InitDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

void Device::CreateInstance()
{
	if (!CheckValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Rids warning on MacOS, but removes compatibility with RenderDoc
	//createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance");
	}
}

void Device::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

	std::multimap<int, VkPhysicalDevice> devices;
	for (const auto& device : physicalDevices)
	{
		int score = RatePhysicalDevice(device);
		devices.insert(std::pair<int, VkPhysicalDevice>(score, device));
	}

	if (devices.rbegin()->first > 0)
	{
		m_physicalDevice = devices.begin()->second;
	}
	else
	{
		throw std::runtime_error("No devices are suitable for this application");
	}
}

int Device::RatePhysicalDevice(const VkPhysicalDevice& device) const
{
	if (!IsDeviceSuitable(device)) return 0;

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

bool Device::IsDeviceSuitable(const VkPhysicalDevice& device) const
{
	QueueFamilyIndices indices = FindQueueFamilies(device, GetSurface());

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(device, GetSurface());
		swapChainAdequate = !swapChainSupport.m_formats.empty() && !swapChainSupport.m_presentModes.empty();
	}

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	return indices.IsComplete() && extensionsSupported && swapChainAdequate && features.samplerAnisotropy;
}

GLFWwindow* Device::GetWindow() const
{
	ASSERT_GLFW_WINDOW_PTR(m_pVkWindow->GetWindow());
	return m_pVkWindow->GetWindow();
}

VkPhysicalDevice Device::GetPhysicalDevice() const
{
	ASSERT_VK_PHYSICAL_DEVICE(m_physicalDevice);
	return m_physicalDevice;
}

VkDevice Device::GetVkDevice() const
{
	ASSERT_VK_LOGICAL_DEVICE(m_device);
	return m_device;
}

VkInstance Device::GetInstance() const
{
	ASSERT_VK_INSTANCE(m_instance);
	return m_instance;
}

std::shared_ptr<Window> Device::GetVkWindow() const
{
	ASSERT_VK_WINDOW_PTR(m_pVkWindow);
	return m_pVkWindow;
}

VkSurfaceKHR Device::GetSurface() const
{
	ASSERT_VK_SURFACE_PTR(m_pSurface);
	return m_pSurface->GetSurface();
}

std::shared_ptr<Swapchain> Device::GetSwapchain() const
{
	ASSERT_VK_SWAPCHAIN_PTR(m_pSwapchain);
	return m_pSwapchain;
}

VkExtent2D Device::GetExtent() const
{
	ASSERT_VK_SWAPCHAIN_PTR(m_pSwapchain);
	return m_pSwapchain->GetExtent();
}

VkQueue Device::GetQueue(QueueType type) const
{
	switch (type)
	{
	case GRAPHICS:
		return m_graphicsQueue;
		break;
	case PRESENT:
		return m_presentQueue;
		break;
	case TRANSFER:
		return m_transferQueue;
		break;
	case COMPUTE:
		throw std::runtime_error("No functionality for queue type 'COMPUTE' has been implemented");
		break;
	}

	throw std::runtime_error("Undefined queue type specified");
}

void Device::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, GetSurface());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value(), indices.m_transferFamily.value()};

	float queuePriority = 1.f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	vkGetDeviceQueue(m_device, indices.m_graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, indices.m_presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, indices.m_transferFamily.value(), 0, &m_transferQueue);
}

std::vector<const char*> Device::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		glfwExtensionCount++;
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::vector<const char*> requiredGlfwExtensions;

	for (uint32_t i = 0; i < glfwExtensionCount; i++)
	{
		requiredGlfwExtensions.emplace_back(extensions[i]);
	}

	// Rids warning on MacOS, but removes compatibility with RenderDoc
	//requiredGlfwExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	ValidateExtensionAvailability(requiredGlfwExtensions);

	return requiredGlfwExtensions;
}

void Device::ValidateExtensionAvailability(std::vector<const char*>& inputExtensions)
{
	assert((inputExtensions.size() > 0) && "input extension array is empty");

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& input : inputExtensions)
	{
		bool isAvailable = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(input, extension.extensionName) == 0)
			{
				isAvailable = true;
				break;
			}
		}

		if (!isAvailable)
		{
			throw std::runtime_error("extension is not available");
		}
	}
}

bool Device::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

bool Device::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const
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

uint32_t Device::FindMemoryType(uint32_t typeFilter, const VkMemoryPropertyFlags& properties)
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

VKAPI_ATTR VkBool32 VKAPI_CALL Device::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		std::cout << "INFO: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		std::cout << "WARNING: ";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		std::cout << "ERROR: ";
		break;
	default:
		return VK_FALSE;
		break;
	}

	std::cout << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult Device::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Device::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Device::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr; // Optional
}
