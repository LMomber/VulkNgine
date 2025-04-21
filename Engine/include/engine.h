#pragma once

#include "vkCommon.h"

#include <memory>
#include <cassert>

class Device;
namespace Core
{
	class Engine
	{
	public:
		void Initialize();
		void Update();
		void ShutDown();

		const Device& GetDevice() const;
	private:
		std::shared_ptr<Device> m_pDevice = nullptr;
	};

	extern Engine engine;
}