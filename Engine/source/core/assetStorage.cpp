#include "assetStorage.h"

AssetStorage& AssetStorage::Get()
{
    static AssetStorage assetStorage;
    return assetStorage;
}

MeshID AssetStorage::CreateMesh(const aiScene& aiScene, const aiNode& aiNode)
{
    MeshID id{ static_cast<uint32_t>(m_meshStorage.size()) };
    m_meshStorage.emplace_back(aiScene, aiNode);
    return id;
}

MaterialID AssetStorage::CreateMaterial(const aiScene& aiScene, const aiMesh& aiMesh)
{
    MaterialID id{ static_cast<uint32_t>(m_materialStorage.size()) };
    m_materialStorage.emplace_back(aiScene, aiMesh);
    return id;
}

AssetID AssetStorage::CreateAssetID(MeshID id)
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
    if (id.m_raw >= m_meshStorage.size())
    {
        throw std::logic_error("Mesh ID is larger than storage size.");
    }

    return m_meshStorage[id.m_raw];
}
