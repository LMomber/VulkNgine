#pragma once

#include "vkCommon.h"
#include "vkWindow.h"
#include "vkSurface.h"
#include "vkSwapchain.h"

class PhysicalDevice;
class Queue;
class Device
{
public:
	Device() = default;

	void Initialize();
	void ShutDown();
	
	GLFWwindow* GetWindow() const;
	VkDevice GetVkDevice() const;
	VkInstance GetInstance() const;
	VkSurfaceKHR GetSurface() const;
	VkExtent2D GetExtent() const;

	std::shared_ptr<PhysicalDevice> GetPhysicalDevice() const;
	std::shared_ptr<Window> GetVkWindow() const;
	std::shared_ptr<Swapchain> GetSwapchain() const;
	std::shared_ptr<Queue> GetQueue() const;

	VkDeviceMemory AllocateMemory(const VkMemoryAllocateInfo& allocInfo) const;
	void* MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags = 0) const;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

private:
	std::shared_ptr<Swapchain> m_pSwapchain = nullptr;
	std::shared_ptr<Window> m_pVkWindow = nullptr;
	std::shared_ptr<PhysicalDevice> m_pPhysicalDevice = nullptr;
	std::shared_ptr<Queue> m_pQueue = nullptr;

	std::unique_ptr<Surface> m_pSurface = nullptr;

	VkInstance m_instance{};
	VkDevice m_device{};

	VkDebugUtilsMessengerEXT m_debugMessenger{};

	void InitDebugMessenger();

	void CreateInstance();
	void CreateLogicalDevice(QueueFamilyIndices indices);

	std::vector<const char*> GetRequiredExtensions();

	void ValidateExtensionAvailability(std::vector<const char*>& inputExtensions);

	bool CheckValidationLayerSupport();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};