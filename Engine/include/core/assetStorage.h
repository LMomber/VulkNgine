#pragma once

#include "pch.h"

#include "material.h"

#include "dataStructures.h"

// Singleton
class AssetStorage
{
public:
	static AssetStorage& Get();

	AssetID CreateAssetID(ModelID id);

	ModelID AddToRenderIndices(const std::vector<uint32_t>& ids);
	const std::vector<uint32_t>& GetRenderIndices(AssetID id) const;

private:
	std::vector<std::vector<uint32_t>> m_renderIndices;
	std::vector<uint32_t> m_freeList;
};