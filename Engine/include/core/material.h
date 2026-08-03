#pragma once

#include "pch.h"

namespace CPU
{
	// TODO: Add other linear and sRGB formats
	// TODO: Add BCn formats
	// TODO: Add HDR formats
	enum class TextureFormat : uint8_t
	{
		// Linear format
		RGBA8_UNORM,
		RGB8_UNORM,
		RG8_UNORM,
		R8_UNORM,

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
		MaterialTexture(TextureSemantic semantic, uint32_t uvIndex, uint32_t width, uint32_t height, uint32_t channels, const std::vector<uint8_t>& pixels, TextureFormat format, const std::string& path);

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
		Material();
		Material(const std::string& filePath, const aiScene& scene, const aiMesh& mesh);

		const MaterialProperty& GetProperties() const { return m_properties; }
		const std::shared_ptr<MaterialTexture> GetTexture(TextureSemantic semantic) const;
		void SetTexture(std::shared_ptr<MaterialTexture> texture, TextureSemantic semantic);

	private:
		void ExtractTexture(const std::string& pFile, const aiScene& scene, aiMaterial* mat, const aiMesh& mesh, aiTextureType type, TextureSemantic m_semantic);

	private:
		MaterialProperty m_properties{};

		std::shared_ptr<MaterialTexture> m_albedoTexture = nullptr;
		std::shared_ptr<MaterialTexture> m_normalTexture = nullptr;
		std::shared_ptr<MaterialTexture> m_metallicRoughnessTexture = nullptr;
		std::shared_ptr<MaterialTexture> m_occlusionTexture = nullptr;
		std::shared_ptr<MaterialTexture> m_emissiveTexture = nullptr;
	};
}