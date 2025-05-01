#pragma once

#include "vkCommon.h"
#include "vkWindow.h"
#include "vkSurface.h"
#include "vkSwapchain.h"

class Device
{
public:
	void Initialize();
	void ShutDown();
	
	// Vulkan specific
	//QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
	//SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device) const;

	GLFWwindow* GetWindow() const;
	VkPhysicalDevice GetPhysicalDevice() const;
	VkDevice GetVkDevice() const;
	VkInstance GetInstance() const;
	std::shared_ptr<Window> GetVkWindow() const;
	VkSurfaceKHR GetSurface() const;
	std::shared_ptr<Swapchain> GetSwapchain() const;
	VkExtent2D GetExtent() const;
	VkQueue GetQueue(QueueType type) const;

private:
	/*std::unique_ptr<GLFWwindow> m_pWindow;*/
	std::shared_ptr<Swapchain> m_pSwapchain = nullptr;
	std::shared_ptr<Window> m_pVkWindow = nullptr;

	std::unique_ptr<Surface> m_pSurface = nullptr;

	VkInstance m_instance{};
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device{};

	VkQueue m_graphicsQueue{};
	VkQueue m_presentQueue{};
	VkQueue m_transferQueue{};

	/*VkSurfaceKHR m_surface;*/
	VkDebugUtilsMessengerEXT m_debugMessenger{};

	/*void InitWindow();*/
	void InitDebugMessenger();

	void CreateInstance();
	void CreateLogicalDevice();

	void PickPhysicalDevice();
	int RatePhysicalDevice(const VkPhysicalDevice& device) const;
	bool IsDeviceSuitable(const VkPhysicalDevice& device) const;

	std::vector<const char*> GetRequiredExtensions();

	void ValidateExtensionAvailability(std::vector<const char*>& inputExtensions);

	bool CheckValidationLayerSupport();
	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};