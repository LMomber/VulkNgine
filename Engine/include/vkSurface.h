#pragma once

#include "vkCommon.h"

class Surface
{
public:
	Surface();
	~Surface();

	VkSurfaceKHR GetSurface() const;
private:

	VkSurfaceKHR m_surface;
};