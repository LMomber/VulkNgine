#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<uint32_t> m_graphicsFamily;
	std::optional<uint32_t> m_presentFamily;

	bool IsComplete() const
	{
		return m_graphicsFamily.has_value() && m_presentFamily.has_value();
	}
};

class Device
{
public:
	void Initialize();

	GLFWwindow* GetWindow() { return m_pWindow; }

private:
	GLFWwindow* m_pWindow;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;

	VkQueue m_graphicsQueue;

	VkSurfaceKHR m_surface;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	void InitWindow();
	void InitDebugMessenger();

	void Cleanup();

	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();

	void PickPhysicalDevice();
	int RatePhysicalDevice(const VkPhysicalDevice& device) const; 
	bool IsDeviceSuitable(const VkPhysicalDevice& device) const;

	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;


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