#pragma once

#include "vkCommon.h"

class Window;
class Swapchain
{
public:
	Swapchain(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, std::shared_ptr<Window> window);
	~Swapchain();

	VkSwapchainKHR GetVkSwapChain() const { return m_swapChain; }
	const VkExtent2D& GetExtent() const { return m_extent; }
	VkFormat GetFormat() const { return m_imageFormat; }

	const std::vector<VkImage>& GetImages() const { return m_images; }
	const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }

private:
	void CreateSwapchain();
	void CreateImageViews();

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> availableModes) const;

	VkSwapchainKHR m_swapChain;
	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;
	VkSurfaceKHR m_surface;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;

	std::shared_ptr<Window> m_pWindow;

	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
};