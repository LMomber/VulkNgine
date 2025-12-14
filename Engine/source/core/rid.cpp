#include "rid.h"

template<typename T>
inline RID RID_Owner<T>::CreateRID()
{
	RID rid = RID(m_currentBase);
	m_currentBase++;

	InitializeRID(rid);
	return rid;
}

template<typename T>
inline RID RID_Owner<T>::CreateRID(const T& value)
{
	RID rid = RID(m_currentBase);
	m_currentBase++;

	InitializeRID(rid, value);
	return rid;
}

template<typename T>
inline void RID_Owner<T>::InitializeRID(RID rid)
{
	m_rids.insert_or_assign(rid, T{});
}

template<typename T>
inline void RID_Owner<T>::InitializeRID(RID rid, const T& value)
{
	m_rids[rid] = value;
}

template<typename T>
inline T* RID_Owner<T>::GetOrNull(const RID rid)
{
	if (owns(rid))
	{
		return m_rids.find(rid);
	}

	return nullptr;
}

template<typename T>
inline bool RID_Owner<T>::owns(const RID rid) const
{
	if (rid == RID())
	{
		return false;
	}

	return m_rids.find(rid);
}

template<typename T>
inline void RID_Owner<T>::free(const RID rid)
{
	if (rid == RID() || !m_rids.find(rid))
	{
		return false;
	}

	m_rids.erase(rid);
}

template<typename T>
inline uint32_t RID_Owner<T>::GetRIDCount() const
{
	return m_currentBase - 1;
}
