#include "vkSurface.h"

Surface::Surface(const VkInstance& instance, GLFWwindow* window) :
	m_instance(instance), m_pVkWindow(window)
{
	if (glfwCreateWindowSurface(m_instance, m_pVkWindow, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

VkSurfaceKHR Surface::GetSurface() const
{
	assert(m_pVkWindow && "GLFW window is not initialized");
	return m_surface;
}
