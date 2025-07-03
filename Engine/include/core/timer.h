#pragma once

#include <chrono>

enum Unit
{
	NANO,
	MICRO,
	MILLI,
	SECONDS
};

class Timer
{
public:
	float GetDeltaTime(Unit unit);

private:
	template <typename T>
	float ReturnType(std::chrono::nanoseconds deltaTime);

	std::chrono::high_resolution_clock m_timer;
	std::chrono::nanoseconds m_lastTimePoint = std::chrono::duration<long long, std::nano>(0);
};

// Base
template<typename T>
float Timer::ReturnType(std::chrono::nanoseconds deltaTime)
{
	throw std::runtime_error("Type is not part of std::chrono");
}

// Specializations
template<>
inline float Timer::ReturnType<std::chrono::nanoseconds>(std::chrono::nanoseconds deltaTime)
{
	return static_cast<float>(deltaTime.count());
}

template<>
inline float Timer::ReturnType<std::chrono::microseconds>(std::chrono::nanoseconds deltaTime)
{
	std::chrono::microseconds value = std::chrono::duration_cast<std::chrono::microseconds>(deltaTime);
	return static_cast<float>(value.count());
}

template<>
inline float Timer::ReturnType<std::chrono::milliseconds>(std::chrono::nanoseconds deltaTime)
{
	std::chrono::milliseconds value = std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime);
	return static_cast<float>(value.count());
}

template<>
inline float Timer::ReturnType<std::chrono::seconds>(std::chrono::nanoseconds deltaTime)
{
	std::chrono::seconds value = std::chrono::duration_cast<std::chrono::seconds>(deltaTime);
	return static_cast<float>(value.count());
}
//
