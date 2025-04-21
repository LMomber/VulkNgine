#pragma once

#include "vkCommon.h"
#include "vkWindow.h"
#include "vkSurface.h"
#include "vkSwapchain.h"

#include <vector>
#include <memory>

class Device
{
public:
	void Initialize();
	void ShutDown();

	void DrawFrame();
	
	GLFWwindow* GetWindow() const { return m_pWindow->GetWindow(); }

	// Vulkan specific
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device) const;
	SwapChainSupportDetails QuerrySwapChainSupport(const VkPhysicalDevice& device) const;

	VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	VkDevice GetVkDevice() const { return m_device; }
	VkInstance GetInstance() const { return m_instance; }
	std::shared_ptr<Window> GetVkWindow() const { return m_pWindow; }
	VkSurfaceKHR GetSurface() const { return m_pSurface->GetSurface(); }

private:
	/*std::unique_ptr<GLFWwindow> m_pWindow;*/
	std::unique_ptr<Swapchain> m_pSwapchain = nullptr;
	std::shared_ptr<Window> m_pWindow = nullptr;

	std::unique_ptr<Surface> m_pSurface = nullptr;

	VkInstance m_instance{};
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device{};

	std::vector<VkFramebuffer> m_framebuffers{};

	VkRenderPass m_renderPass{};
	VkPipelineLayout m_pipelineLayout{};
	VkPipeline m_graphicsPipeline{};
	VkCommandPool m_commandPool{};
	VkCommandBuffer m_commandBuffer{};

	VkQueue m_graphicsQueue{};
	VkQueue m_presentQueue{};

	/*VkSurfaceKHR m_surface;*/
	VkDebugUtilsMessengerEXT m_debugMessenger{};

	VkSemaphore m_imageAvailableSemaphore{};
	VkSemaphore m_renderFinishedSemaphore{};
	VkFence m_inFlightFence{};

	/*void InitWindow();*/
	void InitDebugMessenger();

	void CreateInstance();
	void CreateLogicalDevice();
	/*void CreateSurface();*/
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