#pragma once

#include "vkCommon.h"

#include <memory>
#include <cassert>

class Device;
class Renderer;
namespace Core
{
	class Engine
	{
	public:
		void Initialize();
		void Update();
		void ShutDown();

		const Device& GetDevice() const;
		const Renderer& GetRenderer() const;
	private:
		std::shared_ptr<Device> m_pDevice = nullptr;
		std::shared_ptr<Renderer> m_pRenderer = nullptr;
	};

	extern Engine engine;
}