#pragma once

#include "vkCommon.h"

class Window
{
public:
	Window();
	~Window();

	GLFWwindow* GetWindow() const { return m_pVkWindow; } // Not sure how to do this with smart pointers since there's no destructor..
	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

	void SetFrameBufferResized(const bool isResized);
private:
	GLFWwindow* m_pVkWindow;

	bool m_isFramebufferResized = false;
};