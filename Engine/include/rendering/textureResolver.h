#pragma once

#include "material.h"

struct ResolvedTextureSource
{
	std::vector<uint8_t> m_pixels;
	uint32_t m_width;
	uint32_t m_height;
	uint8_t m_channels;
	CPU::TextureFormat m_format;
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
		const std::string& texturePath,
		const CPU::TextureSemantic semantic,
		const std::filesystem::path& model_dir = "0");
private:
	static ResolvedTextureSource DecodeEmbeddedTexture(const aiTexture* texture);
	static ResolvedTextureSource DecodeImageFromDisk(std::filesystem::path path);
	static CPU::TextureFormat ChooseTextureFormat(CPU::TextureSemantic m_semantic);
};