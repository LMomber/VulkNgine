#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class Device
{
public:
	void Initialize();

	GLFWwindow* GetWindow() { return window; }

private:
	GLFWwindow* window;
	VkInstance m_instance;

	VkDebugUtilsMessengerEXT m_debugMessenger;

	void InitWindow();
	void InitDebugMessenger();

	void Cleanup();

	void CreateInstance();

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