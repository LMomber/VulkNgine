#pragma once

#include "pch.h"

#include "dataStructures.h"
#include "vkData.h"

namespace Vulkan
{
	// A scene element's render data
	struct RenderData
	{
		std::vector<Model> m_meshes; // Indices to the ModelsToRender vector in the renderer.
		uint32_t m_usageCount = 0; // How many times this RenderData is used in the SceneData vector.
	};

	// Singleton
	class MeshStorage
	{
	public:
		static MeshStorage& Get();

		void AddMeshes(size_t hash, const std::vector<Model>& meshes);
		const RenderData& GetMeshes(size_t hash) const;
		void EraseMeshes(size_t hash);
		bool HasMeshes(size_t hash) const;

	private:
		std::unordered_map<size_t, RenderData> m_meshStorage;
	};
}