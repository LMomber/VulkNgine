#pragma once

#include "vkCommon.h"

class Window
{
public:
	Window();
	~Window();

	GLFWwindow* GetWindow() const { return m_pWindow; } // Not sure how to do this with smart pointers since there's no destructor..
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	GLFWwindow* m_pWindow;
};