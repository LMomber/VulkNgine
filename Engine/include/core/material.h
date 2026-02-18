#pragma once

#include "pch.h"

//#include "rid.h"

// TODO: Add other linear and sRGB formats
// TODO: Add BCn formats
// TODO: Add HDR formats
enum class TextureFormat : uint8_t
{
	// Linear format
	RGBA8_UNORM,

	// sRGB format
	SRGBA8
};

enum class TextureSemantic
{
	Albedo,
	Normal,
	Emissive,
	MetallicRoughness,
	AO
};

struct MaterialTexture
{
	TextureSemantic m_semantic = TextureSemantic::Albedo;
	uint32_t m_uv_Index = 0;

	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_channels = 0;
	std::vector<uint8_t> m_pixels;
	TextureFormat m_format;

	std::string m_path;
};

struct MaterialProperty
{
	glm::vec4 m_baseColor = glm::vec4(1.0f);
	float m_roughness = 1.0f;
	float m_metallic = 0.0f;
	float m_normalScale = 1.0f;
	bool m_alphaBlend = false;
};

class Material
{
public:
	Material() = default;
	Material(const aiScene& scene, const aiMesh& mesh);

	const MaterialProperty& GetProperties() const { return m_properties; }
	const std::vector<MaterialTexture>& GetTextures() const { return m_textures; }

private:
	void ParseAssimpMaterial(const aiScene& scene, const aiMesh& mesh);
	void ExtractTexture(const aiScene& scene, aiMaterial* mat, const aiMesh& mesh, aiTextureType type, TextureSemantic m_semantic);

private:
	MaterialProperty m_properties{};
	std::vector<MaterialTexture> m_textures{};
};