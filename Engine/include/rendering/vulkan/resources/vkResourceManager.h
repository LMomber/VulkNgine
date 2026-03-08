#pragma once

#include "rid.h"
#include "vkTexture.h"
#include "vkMesh.h"

namespace Vulkan
{
	class ResourceManager
	{
	public:
		static ResourceManager& Get()
		{
			static ResourceManager instance;
			return instance;
		}

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator=(const ResourceManager&) = delete;
		ResourceManager(ResourceManager&&) = delete;
		ResourceManager& operator=(ResourceManager&&) = delete;

		// TODO: Encapsulate properly later
		static RID_Owner<Vulkan::Texture> textureOwner;
		static RID_Owner<Vulkan::Mesh> meshOwner;
		//
	private:
		ResourceManager() = default;
		~ResourceManager() = default;
	};
}