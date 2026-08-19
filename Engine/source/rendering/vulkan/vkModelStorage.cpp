#include "vkModelStorage.h"

#include "vkRender.h"

#include "core/engine.h"

using namespace Vulkan;

MeshStorage& MeshStorage::Get()
{
	static MeshStorage modelStorage;
	return modelStorage;
}

void MeshStorage::AddMeshes(size_t hash, const std::vector<Model>& meshes)
{
	RenderData renderData;
	renderData.m_meshes = meshes;

	auto [it, inserted] = m_meshStorage.try_emplace(hash, renderData);
	m_meshStorage.at(it->first).m_usageCount++;
}

void Vulkan::MeshStorage::EraseMeshes(size_t hash)
{
	auto it = m_meshStorage.find(hash);

	if (it != m_meshStorage.end())
	{
		m_meshStorage.erase(hash);
		return;
	}

	throw std::runtime_error("Index at hash value is already empty.");
}

const RenderData& MeshStorage::GetMeshes(size_t hash) const
{
	auto it = m_meshStorage.find(hash);
	if (it != m_meshStorage.end())
	{
		return it->second;
	}

	throw std::runtime_error("Key does not exist in render data storage.");
}

bool MeshStorage::HasMeshes(size_t hash) const
{
	auto it = m_meshStorage.find(hash);
	if (it != m_meshStorage.end())
	{
		return true;
	}

	return false;
}