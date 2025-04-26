#include "vkSurface.h"

Surface::Surface(const VkInstance& instance, GLFWwindow* window) :
	m_instance(instance), m_pGlfwWindow(window)
{
	ASSERT_GLFW_WINDOW_PTR(window);

	if (glfwCreateWindowSurface(m_instance, m_pGlfwWindow, nullptr, &m_surface) != VK_SUCCESS)
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
	return m_surface;
}
