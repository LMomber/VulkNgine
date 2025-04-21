#include "vkSurface.h"

#include "engine.h"
#include "vkDevice.h"

#include <cassert>
#include <stdexcept>

Surface::Surface()
{
	if (glfwCreateWindowSurface(Core::engine.GetDevice().GetInstance(), Core::engine.GetDevice().GetWindow(), nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(Core::engine.GetDevice().GetInstance(), m_surface, nullptr);
}

VkSurfaceKHR Surface::GetSurface() const
{
	assert(Core::engine.GetDevice().GetWindow() && "GLFW window is not initialized");
	return m_surface;
}
