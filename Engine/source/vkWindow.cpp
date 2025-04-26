#include "vkWindow.h"

#include <stdexcept>
#include <cassert>
#include <algorithm>

static void FramebufferResizeCallback(GLFWwindow* window, int, int)
{
	Window* vkWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	vkWindow->SetFrameBufferResized(true);
}


Window::Window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_pVkWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(m_pVkWindow, this);
	glfwSetFramebufferSizeCallback(m_pVkWindow, FramebufferResizeCallback);
}

Window::~Window()
{
	glfwDestroyWindow(m_pVkWindow);
}

VkExtent2D Window::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_pVkWindow, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Window::SetFrameBufferResized(const bool isResized)
{
	m_isFramebufferResized = isResized;
}
