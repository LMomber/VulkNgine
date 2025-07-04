#pragma once

#include <bitset>

// From the Boost library
template<class T>
inline void HashCombine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
};
