#pragma once

#include "vkCommon.h"

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR m_capabilities;
	std::vector<VkSurfaceFormatKHR> m_formats;
	std::vector<VkPresentModeKHR> m_presentModes;
};

class Device;
class Swapchain
{
public:
	Swapchain();
	~Swapchain();

	VkSwapchainKHR GetVkSwapChain() const { return m_swapChain; }
	const VkExtent2D& GetExtent() const { return m_extent; }
	VkFormat GetFormat() const { return m_imageFormat; }
	/*VkSurfaceKHR GetSurface() const { return m_surface; }*/

	const std::vector<VkImage>& GetImages() const { return m_images; }
	const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }

private:
	/*void CreateWindow();
	void CreateSurface();*/
	void CreateSwapchain();
	void CreateImageViews();

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) const;
	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> availableModes) const;

	VkSwapchainKHR m_swapChain;
	VkFormat m_imageFormat;
	VkExtent2D m_extent;

	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
};