#pragma once

#include "vkCommon.h"

class Window;
class PhysicalDevice;
class Swapchain
{
public:
	Swapchain(const VkDevice& device, const VkSurfaceKHR& surface, std::shared_ptr<Window> window, std::shared_ptr<PhysicalDevice> physicalDevice);
	~Swapchain();

	uint32_t AcquireNextImage(VkSemaphore imageAvailableSemaphore);
	void RecreateSwapchain();

	VkSwapchainKHR GetVkSwapChain() const { return m_swapChain; }
	const VkExtent2D GetExtent() const { return m_extent; }

	VkImage GetCurrentImage() const;
	VkImageView GetCurrentImageView() const;
	ImageLayout& GetCurrentImageLayout();
	VkImage GetCurrentDepthImage() const;

	const std::vector<VkImage>& GetImages() const { return m_images; }
	const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }
	std::array<ImageLayout, 3>& GetImageLayouts() { return m_imageLayouts; }
	const std::array<VkImage, 3>& GetDepthImages() const { return m_depthImages; }
	const std::array<VkImageView, 3>& GetDepthViews() const { return m_depthImageViews; }
	std::array<ImageLayout, 3>& GetDepthImageLayouts() { return m_depthImageLayouts; }
	const VkFormat GetDepthFormat() const{ return m_depthFormat; }
	const VkFormat GetImageFormat() const{ return m_imageFormat; }
	uint32_t GetImageCount() const { return m_imageCount; };

private:
	void CreateSwapchain();
	void CreateImageViews();
	void CreateDepthResources();

	void CleanUp();

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableModes) const;

	VkSwapchainKHR m_swapChain;
	VkSwapchainKHR m_oldSwapChain = VK_NULL_HANDLE;
	VkDevice m_device;
	VkSurfaceKHR m_surface;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;

	std::shared_ptr<Window> m_pVkWindow;
	std::shared_ptr<PhysicalDevice> m_pPhysicalDevice;

	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
	std::array<ImageLayout, 3> m_imageLayouts;
	uint32_t m_currentImageIndex = std::numeric_limits<uint32_t>().max();

	VkFormat m_depthFormat;
	std::array<VkImage, 3> m_depthImages;
	std::array<VkDeviceMemory, 3> m_depthImageMemory;
	std::array<VkImageView, 3> m_depthImageViews;
	std::array<ImageLayout, 3> m_depthImageLayouts;

	uint32_t m_imageCount = 0;
};