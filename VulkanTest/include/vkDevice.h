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

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

class Device
{
public:
	void Initialize();
	void DrawFrame();
	void Cleanup();

	GLFWwindow* GetWindow() { return m_pWindow; }

private:
	GLFWwindow* m_pWindow;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	std::vector<VkFramebuffer> m_framebuffers;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;
	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	VkCommandPool m_commandPool;
	VkCommandBuffer m_commandBuffer;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSurfaceKHR m_surface;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	void InitWindow();
	void InitDebugMessenger();

	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void PickPhysicalDevice();
	int RatePhysicalDevice(const VkPhysicalDevice& device) const; 
	bool IsDeviceSuitable(const VkPhysicalDevice& device) const;

	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
	SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device) const;

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> availableModes) const;
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

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