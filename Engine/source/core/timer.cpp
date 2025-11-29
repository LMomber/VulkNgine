#include "timer.h"

#include <iostream>

long long Timer::GetDeltaTime(Unit unit)
{
	{
		std::chrono::duration<long long, std::nano> timePoint = m_timer.now().time_since_epoch();
		std::chrono::nanoseconds timeDifference = timePoint - m_lastTimePoint;

		long long t1 = 0;
		long long t2 = 0;
		long long dif = 0;

		switch (unit)
		{
		case NANO:
			t1 = ReturnType<std::chrono::nanoseconds>(timePoint);
			t2 = ReturnType<std::chrono::nanoseconds>(m_lastTimePoint);
			dif = t1 - t2;

			m_lastTimePoint = timePoint;
			return dif;
			break;
		case MICRO:
			t1 = ReturnType<std::chrono::microseconds>(timePoint);
			t2 = ReturnType<std::chrono::microseconds>(m_lastTimePoint);
			dif = t1 - t2;

			m_lastTimePoint = timePoint;
			return dif;
			break;
		case MILLI:
			t1 = ReturnType<std::chrono::milliseconds>(timePoint);
			t2 = ReturnType<std::chrono::milliseconds>(m_lastTimePoint);
			dif = t1 - t2;

			m_lastTimePoint = timePoint;
			return dif;
			break;
		case SECONDS:
			t1 = ReturnType<std::chrono::seconds>(timePoint);
			t2 = ReturnType<std::chrono::seconds>(m_lastTimePoint);
			dif = t1 - t2;

			m_lastTimePoint = timePoint;
			return dif;
			break;
		}

		throw std::runtime_error("Unit is not available");
	}
}
