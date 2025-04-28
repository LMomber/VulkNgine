#include "engine.h"

#include "vkDevice.h"
#include "vkRender.h"
#include "input.h"
#include "inputHandler.h"

Core::Engine Core::engine;

void Core::Engine::Initialize()
{
	// Macro practice
	INIT_WRAPPER("input handler", m_pInputHandler = std::make_shared<InputHandler>());
	INIT_WRAPPER("device class",
		{
			m_pDevice = std::make_shared<Device>();
			m_pDevice->Initialize();
		};);
	INIT_WRAPPER("renderer", m_pRenderer = std::make_shared<Renderer>(m_pDevice));
	INIT_WRAPPER("input class", 
		{
				std::shared_ptr<Core::Input> inputPtr(new Core::Input, Core::InputDelFunc);
				m_pInput = inputPtr;
		});
}

void Core::Engine::Update(float deltaTime)
{
	m_pInput->Update();
	m_pInputHandler->Update(deltaTime);
	m_pRenderer->Update();
}

void Core::Engine::Render()
{
	m_pRenderer->Render();
}

void Core::Engine::ShutDown()
{
	m_pInputHandler.reset(); // No dependencies
	m_pInput.reset();
	m_pRenderer.reset();
	m_pDevice->ShutDown();
	m_pDevice.reset();
}

const Device& Core::Engine::GetDevice() const
{
	assert(m_pDevice.get() && "Device is either uninitialized or deleted");
	return *m_pDevice.get();
}

const Renderer& Core::Engine::GetRenderer() const
{
	assert(m_pRenderer.get() && "Renderer is either uninitialized or deleted");
	return *m_pRenderer.get();
}

const Core::Input& Core::Engine::GetInput() const
{
	assert(m_pInput.get() && "Input is either uninitialized or deleted");
	return *m_pInput.get();
}

GLFWwindow* Core::Engine::GetWindow() const
{
	auto* window = m_pDevice->GetWindow();
	assert(window && "Window is not instantiated");
	return window;
}

entt::registry& Core::Engine::GetRegistry()
{
	return m_registry;
}
