#pragma once

#include "vkCommon.h"

#pragma warning(push)
#pragma warning(disable : 4267) // Deprecated declarations
#include <entt/entity/registry.hpp>
#pragma warning(pop)

#include <memory>
#include <cassert>

class Device;
class Renderer;
class InputHandler;
namespace Core
{
	class Input; 
	class Engine 
	{
	public:
		void Initialize();
		void Update(float);
		void Render();
		void ShutDown();

		const Device& GetDevice() const;
		const Renderer& GetRenderer() const;
		const Input& GetInput() const;
		GLFWwindow* GetWindow() const;
		entt::registry& GetRegistry();
	private:
		std::shared_ptr<Device> m_pDevice = nullptr;
		std::shared_ptr<Renderer> m_pRenderer = nullptr;
		std::shared_ptr<Input> m_pInput = nullptr;
		std::shared_ptr<InputHandler> m_pInputHandler = nullptr;

		entt::registry m_registry;
	};

	extern Engine engine;
}