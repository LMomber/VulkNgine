#pragma once

#include "pch.h"

#include "material.h"
#include "model.h"

// TODO: Add camera
enum class AssetType : uint8_t
{
	None,
	Mesh,
	Material
};

struct AssetID
{
	AssetType m_type = AssetType::None;

	union
	{
		uint32_t m_raw;
		MeshID m_mesh;
		MaterialID m_material;
	};

	AssetID() : m_raw(0) {}
};

// Singleton
class AssetStorage
{
public:
	static AssetStorage& Get();

	MeshID CreateMesh(const aiScene& aiScene, const aiNode& aiNode);
	MaterialID CreateMaterial(const aiScene& aiScene, const aiMesh& aiMesh);

	AssetID CreateAssetID(MeshID id);
	AssetID CreateAssetID(MaterialID id); // Not used for now

	const Model& GetMesh(AssetID id) const;

private:
	std::vector<Model> m_meshStorage{};
	std::vector<Material> m_materialStorage{};
};