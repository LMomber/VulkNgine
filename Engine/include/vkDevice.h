#pragma once

#include "vkCommon.h"
#include "vkWindow.h"
#include "vkSurface.h"
#include "vkSwapchain.h"

class PhysicalDevice;
class Device
{
public:
	void Initialize();
	void ShutDown();
	
	// Vulkan specific
	//QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
	//SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device) const;

	GLFWwindow* GetWindow() const;
	VkDevice GetVkDevice() const;
	VkInstance GetInstance() const;
	VkSurfaceKHR GetSurface() const;
	VkExtent2D GetExtent() const;
	VkQueue GetQueue(QueueType type) const;

	std::shared_ptr<PhysicalDevice> GetPhysicalDevice() const;
	std::shared_ptr<Window> GetVkWindow() const;
	std::shared_ptr<Swapchain> GetSwapchain() const;

	VkDeviceMemory AllocateMemory(const VkMemoryAllocateInfo& allocInfo) const;
	void* MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags = 0) const;

private:
	/*std::unique_ptr<GLFWwindow> m_pWindow;*/
	std::shared_ptr<Swapchain> m_pSwapchain = nullptr;
	std::shared_ptr<Window> m_pVkWindow = nullptr;
	std::shared_ptr<PhysicalDevice> m_pPhysicalDevice = nullptr;

	std::unique_ptr<Surface> m_pSurface = nullptr;

	VkInstance m_instance{};
	//VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
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