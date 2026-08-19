#include "assetStorage.h"

#include "engine.h"
#include "vkRender.h"

AssetStorage& AssetStorage::Get()
{
	static AssetStorage assetStorage;
	return assetStorage;
}

AssetID AssetStorage::CreateAssetID(ModelID id)
{
	AssetID h;
	h.m_type = AssetType::Model;
	h.m_model = id;
	return h;
}

const std::vector<uint32_t>& AssetStorage::GetRenderIndices(AssetID id) const
{
	if (id.m_raw == std::numeric_limits<size_t>().max())
	{
		throw std::logic_error("Asset ID is uninitialized.");
	}

	if (id.m_raw >= m_renderIndices.size())
	{
		throw std::logic_error("Render ID is larger than models to render.");
	}

	return m_renderIndices[id.m_raw];
}

ModelID AssetStorage::AddToRenderIndices(const std::vector<uint32_t>& ids)
{
	uint32_t index;
	if (m_freeList.empty())
	{
		m_renderIndices.push_back(ids);
		index = static_cast<uint32_t>(m_renderIndices.size()) - 1;
	}
	else
	{
		index = m_freeList.back();
		m_renderIndices[index] = ids;
	}

	ModelID id{ index };
	return id;
}
