#pragma once

#include "vkCommon.h"

#include <memory>

class Surface
{
public:
	Surface();
	~Surface();

	VkSurfaceKHR GetSurface() const;
private:

	VkSurfaceKHR m_surface;
};