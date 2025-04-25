#include "timer.h"

float Timer::GetDeltaTime(Unit unit)
{
	{
		std::chrono::duration<long long, std::nano> timePoint = m_timer.now().time_since_epoch();
		std::chrono::nanoseconds timeDifference = timePoint - m_lastTimePoint;
		m_lastTimePoint = timePoint;

		switch (unit)
		{
		case NANO:
			return ReturnType<std::chrono::nanoseconds>(timeDifference);
			break;
		case MICRO:
			return ReturnType<std::chrono::microseconds>(timeDifference);
			break;
		case MILLI:
			return ReturnType<std::chrono::milliseconds>(timeDifference);
			break;
		case SECONDS:
			return ReturnType<std::chrono::seconds>(timeDifference);
			break;
		}

		throw std::runtime_error("Unit is not available");
	}
}
