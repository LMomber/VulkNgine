#pragma once

#include "vkCommon.h"

class Surface
{
public:
	Surface(const VkInstance& instance, GLFWwindow* window);
	~Surface();

	VkSurfaceKHR GetSurface() const;
private:
	VkSurfaceKHR m_surface;
	VkInstance m_instance;
	GLFWwindow* m_pWindow;
};