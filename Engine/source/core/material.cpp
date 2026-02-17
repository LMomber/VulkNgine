#include "material.h"

Material::Material(const aiScene& scene, const aiMesh& mesh)
{
	ParseAssimpMaterial(scene, mesh);
}

void Material::ParseAssimpMaterial(const aiScene& scene, const aiMesh& mesh)
{
	aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];

	aiColor4D color;
	if (aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS)
	{
		m_properties.m_baseColor = { color.r, color.g, color.b, color.a };
	}

	ExtractTexture(mat, mesh, aiTextureType_BASE_COLOR, TextureSemantic::Albedo);
	ExtractTexture(mat, mesh, aiTextureType_DIFFUSE, TextureSemantic::Albedo); // fallback

	ExtractTexture(mat, mesh, aiTextureType_NORMALS, TextureSemantic::Normal);
	ExtractTexture(mat, mesh, aiTextureType_HEIGHT, TextureSemantic::Normal); // fallback

	ExtractTexture(mat, mesh, aiTextureType_EMISSIVE, TextureSemantic::Emissive);
	ExtractTexture(mat, mesh, aiTextureType_EMISSION_COLOR, TextureSemantic::Emissive);

	ExtractTexture(mat, mesh, aiTextureType_METALNESS, TextureSemantic::MetallicRoughness);

	ExtractTexture(mat, mesh, aiTextureType_AMBIENT_OCCLUSION, TextureSemantic::AO);

    if (m_textures.empty())
    {
        m_textures.emplace_back();
    }
}

// Followed: https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/use_the_lib.html#how-to-map-uv-channels-to-textures-matkey-uvwsrc
void Material::ExtractTexture(aiMaterial* mat, const aiMesh& mesh, aiTextureType type, TextureSemantic m_semantic)
{
    const uint32_t uv_channelCount = mesh.GetNumUVChannels();

    for (uint32_t i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString path;
        mat->GetTexture(type, i, &path);

        uint32_t uv_index = 0;
        const bool hasExplicit_uv =
            mat->Get(AI_MATKEY_UVWSRC(type, i), uv_index) == AI_SUCCESS;

        if (uv_channelCount == 1)
        {
            uv_index = 0;
        }
        else if (!hasExplicit_uv)
        {
            uv_index = i % uv_channelCount;
        }
        else
        {
            uv_index = std::min(uv_index, uv_channelCount - 1);
        }

        m_textures.push_back(MaterialTexture{
            m_semantic,
            uv_index,
            path.C_Str()
            });
    }
}

//MaterialStorage& MaterialStorage::Get()
//{
//	static MaterialStorage materialStorage;
//	return materialStorage;
//}
//
//RID MaterialStorage::CreateMaterial(const aiScene& scene, const aiMesh& mesh)
//{
//	return m_materialOwner.CreateRID(Material{ scene, mesh });
//}
