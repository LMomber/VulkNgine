#pragma once

#include "vkCommon.h"

// From Godot:
class RID
{
public:
	RID() = delete;
	explicit RID(uint64_t id) noexcept : m_id(id) {} // explicit keyword to avoid implicit conversions.

	// [[nodiscard]] attribute makes sure to warn users if they do not use the return-value.
	[[nodiscard]] bool IsValid() const noexcept { return m_id != 0; }
	[[nodiscard]] bool IsNull() const noexcept { return m_id == 0; }
	[[nodiscard]] uint64_t GetValue() const noexcept { return m_id; }

	bool operator==(const RID& other) const noexcept { return m_id == other.m_id; }
	bool operator!=(const RID& other) const noexcept { return m_id != other.m_id; }

	// Not useful for RIDs, force explicit behaviour
	bool operator>(const RID& other) = delete;
	bool operator<(const RID& other) = delete;
	bool operator>=(const RID& other) = delete;
	bool operator<=(const RID& other) = delete;
private:
	uint64_t m_id = 0;
};
//

template<typename T>
class RID_Owner
{
public:
	RID_Owner() = default;

	RID CreateRID();
	RID CreateRID(const T& value);

	void InitializeRID(RID rid);
	void InitializeRID(RID rid, const T& value);

	T* GetOrNull(const RID rid);
	bool owns(const RID rid) const;
	void free(const RID rid);
	uint32_t GetRIDCount() const;

private:
	std::unordered_map<uint64_t, T> m_rids{};
	uint64_t m_currentBase = 1; // Starting from 1 so that we can check p_rid == RID().
};