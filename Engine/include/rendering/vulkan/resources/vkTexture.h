#pragma once

#include "vkCommon.h"

#pragma warning(push, 0)
#include <vma/vk_mem_alloc.h>
#pragma warning(pop)

namespace Vulkan
{
	struct Texture
	{
		VmaAllocation m_allocation;
		VkImage m_image;
		VkImageView m_imageView;
	};
}