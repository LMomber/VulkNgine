#include "vkSwapchain.h"

#include "vkWindow.h"

#include <algorithm>

Swapchain::Swapchain(const VkDevice& device, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, std::shared_ptr<Window> window) :
	m_device(device), m_physicalDevice(physicalDevice), m_surface(surface), m_pVkWindow(window)
{
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateFrameBuffers();
}

Swapchain::~Swapchain()
{
	CleanUp();

	vkDestroyRenderPass(m_device, m_mainRenderPass, nullptr);
	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

void Swapchain::CreateSwapchain()
{
	SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(m_physicalDevice, m_surface);

	VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.m_formats);
	VkPresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.m_presentModes);
	VkExtent2D extent = m_pVkWindow->ChooseExtent(swapChainSupport.m_capabilities);

	uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
	uint32_t maxImageCount = swapChainSupport.m_capabilities.maxImageCount;

	// 0 is a special value that means there is no maximum
	if (maxImageCount > 0 && imageCount > maxImageCount)
	{
		imageCount = maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);
	uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

	if (indices.m_graphicsFamily != indices.m_presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = m_oldSwapChain; // Change if I want resizing

	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
	m_images.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_images.data());

	m_imageFormat = surfaceFormat.format;
	m_extent = extent;
}

void Swapchain::RecreateSwapchain()
{
	int width, height = 0;
	auto window = m_pVkWindow->GetWindow();
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device);

	CleanUp();
	
	m_oldSwapChain = m_swapChain;

	CreateSwapchain();
	CreateImageViews();
	CreateFrameBuffers();
}

void Swapchain::CreateImageViews()
{
	m_imageViews.resize(m_images.size());

	for (size_t i = 0; i < m_images.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_imageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view");
		}
	}
}

void Swapchain::CreateFrameBuffers()
{
	const auto& imageViews = GetImageViews();
	m_framebuffers.resize(imageViews.size());

	for (size_t i = 0; i < imageViews.size(); i++)
	{
		VkImageView attachments[]{ imageViews[i] };

		const auto& extent = GetExtent();

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_mainRenderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = extent.width;
		createInfo.height = extent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}

void Swapchain::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_mainRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass");
	}
}

void Swapchain::CleanUp()
{
	for (const auto& frameBuffer : m_framebuffers)
	{
		vkDestroyFramebuffer(m_device, frameBuffer, nullptr);
	}

	for (const auto& imageView : m_imageViews)
	{
		vkDestroyImageView(m_device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_oldSwapChain, nullptr);
}

VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) const
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Swapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR> availableModes) const
{
	for (const auto& availablePresentMode : availableModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
