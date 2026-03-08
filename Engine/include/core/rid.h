#pragma once

#include "vkCommon.h"

#include <cstddef>
#include <functional>

// From Godot:
class RID
{
public:
	RID() = default;
	explicit RID(int32_t id) noexcept : m_id(id) {} // explicit keyword to avoid implicit conversions.

	// [[nodiscard]] attribute makes sure to warn users if they do not use the return-value.
	[[nodiscard]] bool IsValid() const noexcept { return m_id != -1; }
	[[nodiscard]] bool IsNull() const noexcept { return m_id == -1; }
	[[nodiscard]] int32_t GetValue() const noexcept { return m_id; }

	bool operator==(const RID& other) const noexcept { return m_id == other.m_id; }
	bool operator!=(const RID& other) const noexcept { return m_id != other.m_id; }

	// Not useful for RIDs, force explicit behaviour
	bool operator>(const RID& other) = delete;
	bool operator<(const RID& other) = delete;
	bool operator>=(const RID& other) = delete;
	bool operator<=(const RID& other) = delete;
private:
	int32_t m_id = -1;
};
//

namespace std {
	template<>
	struct hash<RID> {
		size_t operator()(const RID& rid) const noexcept {
			return std::hash<int32_t>{}(rid.GetValue());
		}
	};
}

template<typename T>
class RID_Owner
{
public:
	RID_Owner() = default;

	RID CreateRID(const T& value);

	void InitializeRID(RID rid, const T& value);

	const T* GetOrNull(const RID rid) const;
	bool Owns(const RID rid) const;
	void Free(const RID rid);
	uint32_t GetRIDCount() const;

	template<typename Fn>
	void FreeAll(Fn destroyFunc) {
		for (auto& [rid, value] : m_rids) {
			destroyFunc(value);
		}
		m_rids.clear();
	}
private:
	std::unordered_map<RID, T> m_rids{};
	int32_t m_currentBase = -1; // Starting from -1 so that we can check p_rid == RID().
};

template<typename T>
inline RID RID_Owner<T>::CreateRID(const T& value)
{
	m_currentBase++;
	RID rid = RID(m_currentBase);

	InitializeRID(rid, value);
	return rid;
}

template<typename T>
inline void RID_Owner<T>::InitializeRID(RID rid, const T& value)
{
	m_rids[rid] = value;
}

template<typename T>
inline const T* RID_Owner<T>::GetOrNull(const RID rid) const
{
	if (Owns(rid))
	{
		return &m_rids.find(rid)->second;
	}

	return nullptr;
}

template<typename T>
inline bool RID_Owner<T>::Owns(const RID rid) const
{
	if (rid.IsNull())
	{
		return false;
	}

	return m_rids.find(rid) != m_rids.end();
}

template<typename T>
inline void RID_Owner<T>::Free(const RID rid)
{
	if (rid.IsNull() || (m_rids.find(rid) == m_rids.end()))
	{
		std::runtime_error("Tried cleaning up a non-existend RID.");
	}

	m_rids.erase(rid);
}

template<typename T>
inline uint32_t RID_Owner<T>::GetRIDCount() const
{
	return static_cast<uint32_t>(m_rids.size());
}
