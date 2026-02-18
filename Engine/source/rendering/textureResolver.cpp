#include "textureResolver.h"

ResolvedTextureSource TextureResolver::ResolveTexture(const aiScene& scene,
	const std::string& texturePath,
	const TextureSemantic semantic,
	const std::filesystem::path& modelDir)
{
	ResolvedTextureSource src;

	if (!texturePath.empty() && texturePath[0] == '*')
	{
		const aiTexture* tex = scene.mTextures[std::stoi(texturePath.substr(1))];
		src = DecodeEmbeddedTexture(tex);
	}
	else
	{
		src = DecodeImageFromDisk(modelDir /*/ std::filesystem::path(texture.m_path)*/);
	}

	src.m_format = ChooseTextureFormat(semantic);
	return src;
}

ResolvedTextureSource TextureResolver::DecodeEmbeddedTexture(const aiTexture* texture)
{
	assert(texture->mHeight >= 0);

	ResolvedTextureSource src;

	if (texture->mHeight == 0)
	{
		int w, h, channels;
		stbi_uc* data = stbi_load_from_memory(
			reinterpret_cast<const stbi_uc*>(texture->pcData),
			texture->mWidth,
			&w,
			&h,
			&channels,
			STBI_rgb_alpha
		);

		src.m_height = h;
		src.m_width = w;
		src.m_pixels.assign(data, data + w * h * 4);
		src.m_channels = 4;
		stbi_image_free(data);
	}

	// If texture.mHeight > 0, Assimp guarantees that aiTexel == RGBA 8-bit
	else
	{
		src.m_width = texture->mWidth;
		src.m_height = texture->mHeight;
		src.m_pixels.resize(static_cast<uint64_t>(src.m_width * src.m_height) * 4);
		src.m_channels = 4;

		memcpy(
			src.m_pixels.data(),
			texture->pcData,
			src.m_pixels.size()
		);
	}

	return src;
}

ResolvedTextureSource TextureResolver::DecodeImageFromDisk(std::filesystem::path path)
{
	int w, h, channels;
	stbi_uc* data = stbi_load(
		path.string().c_str(),
		&w,
		&h,
		&channels,
		STBI_rgb_alpha
	);

	ResolvedTextureSource src;
	src.m_height = h;
	src.m_width = w;
	src.m_pixels.assign(data, data + w * h * 4);
	src.m_channels = 4;
	stbi_image_free(data);

	return src;
}

TextureFormat TextureResolver::ChooseTextureFormat(TextureSemantic m_semantic)
{
	switch (m_semantic)
	{
	case TextureSemantic::Albedo:
	case TextureSemantic::Emissive:
	{
		return TextureFormat::SRGBA8;
	}

	case TextureSemantic::Normal:
	case TextureSemantic::MetallicRoughness:
	case TextureSemantic::AO:
	{
		return TextureFormat::RGBA8_UNORM;
	}

	default:
	{
		return TextureFormat::RGBA8_UNORM;
	}
	}
}
