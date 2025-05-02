#pragma once

#include "vkCommon.h"

class Window;
class PhysicalDevice;
class Swapchain
{
public:
	Swapchain(const VkDevice& device, const VkSurfaceKHR& surface, std::shared_ptr<Window> window, std::shared_ptr<PhysicalDevice> physicalDevice);
	~Swapchain();

	void RecreateSwapchain();
	
	VkSwapchainKHR GetVkSwapChain() const { return m_swapChain; }
	const VkExtent2D& GetExtent() const { return m_extent; }
	VkFormat GetFormat() const { return m_imageFormat; }
	VkRenderPass* GetMainRenderPass() { return &m_mainRenderPass; }

	const std::vector<VkImage>& GetImages() const { return m_images; }
	const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }
	const std::vector<VkFramebuffer>& GetFrameBuffers() { return m_framebuffers; }

private:
	void CreateSwapchain();
	void CreateImageViews();
	void CreateDepthResources();
	void CreateFrameBuffers();

	void CreateRenderPass();

	void CleanUp();

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> availableModes) const;

	VkSwapchainKHR m_swapChain;
	VkSwapchainKHR m_oldSwapChain = VK_NULL_HANDLE;
	VkDevice m_device;
	VkSurfaceKHR m_surface;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;
	VkRenderPass m_mainRenderPass{};

	std::shared_ptr<Window> m_pVkWindow;
	std::shared_ptr<PhysicalDevice> m_pPhysicalDevice;

	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	std::vector<VkFramebuffer> m_framebuffers{};

	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;
};