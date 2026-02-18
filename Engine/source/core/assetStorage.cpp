#include "assetStorage.h"

AssetStorage& AssetStorage::Get()
{
    static AssetStorage assetStorage;
    return assetStorage;
}

ModelID AssetStorage::CreateModel(const aiScene& aiScene, const aiNode& aiNode)
{
    ModelID id{ static_cast<uint32_t>(m_modelStorage.size()) };
    m_modelStorage.emplace_back(aiScene, aiNode);
    return id;
}

MaterialID AssetStorage::CreateMaterial(const aiScene& aiScene, const aiMesh& aiMesh)
{
    MaterialID id{ static_cast<uint32_t>(m_materialStorage.size()) };
    m_materialStorage.emplace_back(aiScene, aiMesh);
    return id;
}

AssetID AssetStorage::CreateAssetID(ModelID id)
{
    AssetID h;
    h.m_type = AssetType::Mesh;
    h.m_mesh = id;
    return h;
}

AssetID AssetStorage::CreateAssetID(MaterialID id)
{
    AssetID h;
    h.m_type = AssetType::Material;
    h.m_material = id;
    return h;
}

const Model& AssetStorage::GetMesh(AssetID id) const
{
    if (id.m_raw >= m_modelStorage.size())
    {
        throw std::logic_error("Mesh ID is larger than storage size.");
    }

    return m_modelStorage[id.m_raw];
}
