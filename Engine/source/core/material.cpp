#include "material.h"

#include "textureResolver.h"

using namespace CPU;

Material::Material()
{
	m_albedoTexture = std::make_shared<MaterialTexture>(
		TextureSemantic::Albedo,
		0,
		1,
		1,
		4,
		std::vector<uint8_t>({ {255}, {255}, {255}, {255} }),
		TextureFormat::RGBA8_UNORM,
		std::string{}
	);

	m_normalTexture = std::make_shared<MaterialTexture>(
		TextureSemantic::Normal,
		0,
		1,
		1,
		3,
		std::vector<uint8_t>({ {128}, {128}, {255} }),
		TextureFormat::RGB8_UNORM,
		std::string{}
	);

	// Will make ORM later
	m_metallicRoughnessTexture = std::make_shared<MaterialTexture>(
		TextureSemantic::MetallicRoughness,
		0,
		1,
		1,
		2,
		std::vector<uint8_t>({ {0}, {255} }),
		TextureFormat::RGB8_UNORM,
		std::string{}
	);

	m_occlusionTexture = std::make_shared<MaterialTexture>(
		TextureSemantic::AO,
		0,
		1,
		1,
		1,
		std::vector<uint8_t>({ {255} }),
		TextureFormat::R8_UNORM,
		std::string{}
	);
	//

	m_occlusionTexture = std::make_shared<MaterialTexture>(
		TextureSemantic::Emissive,
		0,
		1,
		1,
		3,
		std::vector<uint8_t>({ {0},{0},{0} }),
		TextureFormat::RGB8_UNORM,
		std::string{}
	);
}

Material::Material(const aiScene& scene, const aiMesh& mesh)
{
	aiMaterial* mat = scene.mMaterials[mesh.mMaterialIndex];

	aiColor4D color;
	if (aiGetMaterialColor(mat, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS)
	{
		m_properties.m_baseColor = { color.r, color.g, color.b, color.a };
	}

	// Not sure about the fallback handling. Right now diffuse & height always overwrite base color & normals.
	ExtractTexture(scene, mat, mesh, aiTextureType_BASE_COLOR, TextureSemantic::Albedo);
	ExtractTexture(scene, mat, mesh, aiTextureType_DIFFUSE, TextureSemantic::Albedo); // fallback

	ExtractTexture(scene, mat, mesh, aiTextureType_NORMALS, TextureSemantic::Normal);
	ExtractTexture(scene, mat, mesh, aiTextureType_HEIGHT, TextureSemantic::Normal); // fallback

	ExtractTexture(scene, mat, mesh, aiTextureType_EMISSIVE, TextureSemantic::Emissive);
	ExtractTexture(scene, mat, mesh, aiTextureType_EMISSION_COLOR, TextureSemantic::Emissive);

	ExtractTexture(scene, mat, mesh, aiTextureType_METALNESS, TextureSemantic::MetallicRoughness);

	ExtractTexture(scene, mat, mesh, aiTextureType_AMBIENT_OCCLUSION, TextureSemantic::AO);
}

const std::shared_ptr<MaterialTexture> Material::GetTexture(TextureSemantic semantic) const
{
	switch (semantic)
	{
	case TextureSemantic::Albedo:
		return m_albedoTexture;
		break;
	case TextureSemantic::Normal:
		return m_normalTexture;
		break;
	case TextureSemantic::MetallicRoughness:
		return m_metallicRoughnessTexture;
		break;
	case TextureSemantic::AO:
		return m_occlusionTexture;
		break;
	case TextureSemantic::Emissive:
		return m_emissiveTexture;
		break;
	default:
		throw std::runtime_error("Texture type not supported by MaterialTexture.");
		break;
	}
}

// Followed: https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/use_the_lib.html#how-to-map-uv-channels-to-textures-matkey-uvwsrc
void Material::ExtractTexture(const aiScene& scene, aiMaterial* mat, const aiMesh& mesh, aiTextureType type, TextureSemantic m_semantic)
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

		// TODO: Not necessary, merge ResolvedTextureSource & MaterialTexture all together.
		ResolvedTextureSource src = TextureResolver::Get().ResolveTexture(scene, path.C_Str(), m_semantic);

		std::shared_ptr<MaterialTexture> materialTexture = std::make_shared<MaterialTexture>(
			m_semantic,
			uv_index,
			src.m_width,
			src.m_height,
			src.m_channels,
			src.m_pixels,
			src.m_format,
			path.C_Str()
		);

		switch (m_semantic)
		{
		case TextureSemantic::Albedo:
			m_albedoTexture = materialTexture;
			break;
		case TextureSemantic::Normal:
			m_normalTexture = materialTexture;
			break;
		case TextureSemantic::MetallicRoughness:
			m_metallicRoughnessTexture = materialTexture;
			break;
		case TextureSemantic::AO:
			m_occlusionTexture = materialTexture;
			break;
		case TextureSemantic::Emissive:
			m_emissiveTexture = materialTexture;
			break;
		default:
			throw std::runtime_error("Texture type not supported by MaterialTexture.");
			break;
		}
	}
}

MaterialTexture::MaterialTexture(TextureSemantic semantic, uint32_t uvIndex, uint32_t width, uint32_t height, uint32_t channels, const std::vector<uint8_t>& pixels, TextureFormat format, const std::string& path)
	: m_semantic(semantic), m_uv_Index(uvIndex), m_width(width), m_height(height), m_channels(channels), m_pixels(pixels), m_format(format), m_path(path)
{
}
