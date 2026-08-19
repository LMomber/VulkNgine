#pragma once

#include <cstdint>
#include <limits>

struct ModelID { size_t m_id = std::numeric_limits<size_t>::max(); };
struct MaterialID { size_t m_id = std::numeric_limits<size_t>::max(); };

// TODO: Add camera
enum class AssetType : uint8_t
{
	None,
	Model
};

struct AssetID
{
	AssetType m_type = AssetType::None;

	union
	{
		size_t m_raw;
		ModelID m_model;
	};

	AssetID() : m_raw(0) {}
	AssetID(size_t value) : m_raw(value) {}
};