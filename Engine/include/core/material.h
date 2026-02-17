#pragma once

#include "pch.h"

//#include "rid.h"

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
	void ExtractTexture(aiMaterial* mat, const aiMesh& mesh, aiTextureType type, TextureSemantic m_semantic);

private:
	MaterialProperty m_properties{};
	std::vector<MaterialTexture> m_textures{};
};
//
//// Singleton
//class MaterialStorage
//{
//public:
//	static MaterialStorage& Get();
//
//	RID CreateMaterial(const aiScene& scene, const aiMesh& mesh);
//
//	MaterialStorage(const MaterialStorage&) = delete;
//	MaterialStorage operator=(const MaterialStorage&) = delete;
//	MaterialStorage(MaterialStorage&&) noexcept = delete;
//	MaterialStorage& operator=(MaterialStorage&&) noexcept = delete;
//private:
//	MaterialStorage() = default;
//	~MaterialStorage() = default;
//
//	RID_Owner<Material> m_materialOwner;
//};