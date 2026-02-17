#pragma once

#include "material.h"

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

struct ResolvedTextureSource
{
	std::vector<uint8_t> m_pixels;
	uint32_t m_width;
	uint32_t m_height;
	uint8_t m_channels;
	TextureFormat m_format;
};

class TextureResolver
{
public:
	static TextureResolver& Get()
	{
		static TextureResolver textureResolver;
		return textureResolver;
	}

	static ResolvedTextureSource ResolveTexture(const aiScene& scene,
		const MaterialTexture& texture,
		const std::filesystem::path& model_dir);
private:
	static ResolvedTextureSource DecodeEmbeddedTexture(const aiTexture* texture);
	static ResolvedTextureSource DecodeImageFromDisk(std::filesystem::path path);
	static TextureFormat ChooseTextureFormat(TextureSemantic m_semantic);
};